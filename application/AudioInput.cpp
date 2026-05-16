#include "AudioInput.h"

#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <thread>
#include <vector>

namespace {

constexpr int    kSampleRate       = 48000;
constexpr int    kChannels         = 1;
constexpr int    kBufferSamples    = 1024;   ///< ~23 ms per callback at 44.1 kHz
constexpr int    kEnergyHistoryLen = 43;     ///< ~1 s of chunks for beat avg

// Smoothing factors per chunk (α in y = α·x + (1-α)·y).
constexpr float kVolumeSmoothing = 0.30f;
constexpr float kPeakSmoothing   = 0.40f;
constexpr float kBandSmoothing   = 0.35f;

// One-pole filter coefficients for band split (computed once for kSampleRate).
//   y[n] = α·x[n] + (1-α)·y[n-1],   α = dt / (RC + dt),  RC = 1/(2π·fc)
inline float lpCoef(float fc, float fs) {
    const float dt = 1.0f / fs;
    const float rc = 1.0f / (2.0f * float(M_PI) * fc);
    return dt / (rc + dt);
}

inline std::int64_t monoMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

} // namespace

AudioInput::AudioInput()
    : deviceId_(0)
    , outputDeviceId_(0)
    , obtainedFormat_(0)
    , available_(false)
    , enabled_(false)
    , gain_(1.0f)
    , beatThreshold_(1.4f)
    , beatRefractoryMs_(180)
    , volume_(0.0f)
    , peak_(0.0f)
    , bass_(0.0f)
    , mid_(0.0f)
    , treble_(0.0f)
    , beatFlag_(false)
    , beatLatch_(false)
    , energyHistory_(kEnergyHistoryLen, 0.0f)
    , energyHistoryIdx_(0)
    , lastBeatMonoMs_(0)
    , dcBlockX_(0.0f)
    , dcBlockY_(0.0f)
    , bassLp_(0.0f)
    , midLp_(0.0f)
    , midHp_(0.0f)
    , trebleHp_(0.0f)
    , sampleRate_(kSampleRate)
    , channels_(kChannels) {}

AudioInput::~AudioInput() {
    stop();
    if (deviceId_ != 0) {
        SDL_CloseAudioDevice(deviceId_);
        deviceId_ = 0;
    }
}

bool AudioInput::init(const std::string& preferredDevice) {
    // Force ALSA on Linux so I2S devices are reachable regardless of how the
    // process was launched (systemd service, MainMenu, manual shell, etc.).
    // Only sets the variable if not already overridden by the environment.
    if (!getenv("SDL_AUDIODRIVER"))
        setenv("SDL_AUDIODRIVER", "alsa", 0);

    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
            std::cerr << "AudioInput: SDL_InitSubSystem failed: " << SDL_GetError() << std::endl;
            available_ = false;
            return false;
        }
    }

    if (deviceId_ != 0) {
        SDL_CloseAudioDevice(deviceId_);
        deviceId_ = 0;
    }

    SDL_AudioSpec desired{};
    desired.freq     = kSampleRate;
    desired.format   = AUDIO_F32SYS;
    desired.channels = kChannels;
    desired.samples  = kBufferSamples;
    desired.callback = &AudioInput::audioCallback;
    desired.userdata = this;

    // Enumerate SDL capture devices for diagnostics.
    const char* driverName = SDL_GetCurrentAudioDriver();
    std::cerr << "AudioInput: SDL driver: " << (driverName ? driverName : "none") << std::endl;
    int numDev = SDL_GetNumAudioDevices(/*iscapture=*/1);
    std::cerr << "AudioInput: " << numDev << " capture device(s):" << std::endl;
    for (int i = 0; i < numDev; ++i) {
        const char* n = SDL_GetAudioDeviceName(i, 1);
        std::cerr << "  [" << i << "] " << (n ? n : "?") << std::endl;
    }

    SDL_AudioSpec obtained{};
    const char* dev = preferredDevice.empty() ? nullptr : preferredDevice.c_str();
    // Allow format, channel, and frequency changes so the ICS43432 (S32 stereo,
    // 48 kHz, hardware-locked) can be opened. The callback converts to float.
    constexpr int kAllowChanges = SDL_AUDIO_ALLOW_FORMAT_CHANGE
                                | SDL_AUDIO_ALLOW_CHANNELS_CHANGE
                                | SDL_AUDIO_ALLOW_FREQUENCY_CHANGE;

    deviceId_ = SDL_OpenAudioDevice(dev, /*iscapture=*/1, &desired, &obtained, kAllowChanges);

    // The ALSA "default" PCM resolves to dmix (playback-only) on many Pi setups.
    // Try common capture-specific device names before giving up.
    if (deviceId_ == 0 && preferredDevice.empty()) {
        // SDL_GetAudioDeviceName() returns the ALSA hint NAME string, which is
        // the correct argument for SDL_OpenAudioDevice() — try each enumerated
        // capture device before giving up.
        int numDev = SDL_GetNumAudioDevices(/*iscapture=*/1);
        for (int i = 0; i < numDev && deviceId_ == 0; ++i) {
            const char* devName = SDL_GetAudioDeviceName(i, 1);
            if (!devName) continue;
            std::cerr << "AudioInput: retrying with enumerated device [" << i << "]: " << devName << std::endl;
            SDL_AudioSpec ob2{};
            deviceId_ = SDL_OpenAudioDevice(devName, 1, &desired, &ob2, kAllowChanges);
            if (deviceId_ != 0) obtained = ob2;
        }
    }

    if (deviceId_ == 0) {
        std::cerr << "AudioInput: SDL_OpenAudioDevice failed: " << SDL_GetError() << std::endl;
        available_ = false;
        return false;
    }

    sampleRate_     = obtained.freq;
    channels_       = obtained.channels;
    obtainedFormat_ = obtained.format;
    available_      = true;
    enabled_.store(true);

    std::cerr << "AudioInput: opened id=" << deviceId_
              << " freq=" << obtained.freq
              << " ch=" << int(obtained.channels)
              << " fmt=0x" << std::hex << obtained.format << std::dec << std::endl;

    SDL_PauseAudioDevice(deviceId_, 0);
    return true;
}

void AudioInput::start() {
    if (deviceId_ != 0) SDL_PauseAudioDevice(deviceId_, 0);
}

void AudioInput::stop() {
    if (deviceId_ != 0) SDL_PauseAudioDevice(deviceId_, 1);
}

void AudioInput::setEnabled(bool enabled) {
    enabled_.store(enabled);
    if (!enabled) {
        std::lock_guard<std::mutex> lk(featureMutex_);
        volume_    = 0.0f;
        peak_      = 0.0f;
        bass_      = 0.0f;
        mid_       = 0.0f;
        treble_    = 0.0f;
        beatFlag_  = false;
        beatLatch_ = false;
    }
}

float AudioInput::getVolume() const {
    if (!enabled_.load()) return 0.0f;
    std::lock_guard<std::mutex> lk(featureMutex_);
    return volume_;
}

float AudioInput::getPeak() const {
    if (!enabled_.load()) return 0.0f;
    std::lock_guard<std::mutex> lk(featureMutex_);
    return peak_;
}

bool AudioInput::isBeat() const {
    if (!enabled_.load()) return false;
    std::lock_guard<std::mutex> lk(featureMutex_);
    return beatFlag_;
}

bool AudioInput::consumeBeat() {
    if (!enabled_.load()) return false;
    std::lock_guard<std::mutex> lk(featureMutex_);
    if (beatLatch_) {
        beatLatch_ = false;
        return true;
    }
    return false;
}

Eigen::Vector3f AudioInput::getBands() const {
    if (!enabled_.load()) return Eigen::Vector3f::Zero();
    std::lock_guard<std::mutex> lk(featureMutex_);
    return Eigen::Vector3f(bass_, mid_, treble_);
}

void AudioInput::playBeep(float freqHz, float durationMs, float amplitude) {
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
            std::cerr << "AudioInput::playBeep: SDL_InitSubSystem failed: " << SDL_GetError() << std::endl;
            return;
        }
    }

    // Close any previous output device.
    if (outputDeviceId_ != 0) {
        SDL_CloseAudioDevice(outputDeviceId_);
        outputDeviceId_ = 0;
    }

    SDL_AudioSpec desired{};
    desired.freq     = kSampleRate;
    desired.format   = AUDIO_F32SYS;
    desired.channels = 1;
    desired.samples  = kBufferSamples;
    desired.callback = nullptr;  // push mode via SDL_QueueAudio

    SDL_AudioSpec obtained{};
    outputDeviceId_ = SDL_OpenAudioDevice(nullptr, /*iscapture=*/0, &desired, &obtained, 0);
    if (outputDeviceId_ == 0) {
        std::cerr << "AudioInput::playBeep: SDL_OpenAudioDevice (output) failed: " << SDL_GetError() << std::endl;
        return;
    }

    const int numSamples = static_cast<int>(durationMs / 1000.0f * float(kSampleRate));
    std::vector<float> buf(numSamples);
    const float twoPiF = 2.0f * float(M_PI) * freqHz;
    for (int i = 0; i < numSamples; ++i) {
        float t = float(i) / float(kSampleRate);
        // Fade in/out over 10 ms to avoid clicks.
        float env = 1.0f;
        const int fadeSamples = kSampleRate / 100;
        if (i < fadeSamples)       env = float(i) / float(fadeSamples);
        if (i > numSamples - fadeSamples) env = float(numSamples - i) / float(fadeSamples);
        buf[i] = amplitude * env * std::sin(twoPiF * t);
    }

    SDL_QueueAudio(outputDeviceId_, buf.data(), static_cast<std::uint32_t>(buf.size() * sizeof(float)));
    SDL_PauseAudioDevice(outputDeviceId_, 0);

    // Close the device after playback finishes (detached — non-blocking).
    SDL_AudioDeviceID devToClose = outputDeviceId_;
    std::thread([devToClose, durationMs]() {
        SDL_Delay(static_cast<std::uint32_t>(durationMs) + 50);
        SDL_CloseAudioDevice(devToClose);
    }).detach();
}

void AudioInput::audioCallback(void* userdata, std::uint8_t* stream, int len) {
    auto* self = static_cast<AudioInput*>(userdata);

    if (self->obtainedFormat_ == AUDIO_F32SYS || self->obtainedFormat_ == AUDIO_F32LSB || self->obtainedFormat_ == AUDIO_F32MSB) {
        const int numSamples = len / int(sizeof(float));
        self->processSamples(reinterpret_cast<const float*>(stream), numSamples);
    } else if (self->obtainedFormat_ == AUDIO_S32SYS || self->obtainedFormat_ == AUDIO_S32LSB || self->obtainedFormat_ == AUDIO_S32MSB) {
        // ICS43432 and other I2S mics deliver S32. Convert to float [-1,1].
        const int numSamples = len / int(sizeof(std::int32_t));
        std::vector<float> buf(numSamples);
        const auto* s32 = reinterpret_cast<const std::int32_t*>(stream);
        constexpr float kS32Scale = 1.0f / float(std::numeric_limits<std::int32_t>::max());
        for (int i = 0; i < numSamples; ++i)
            buf[i] = float(s32[i]) * kS32Scale;
        self->processSamples(buf.data(), numSamples);
    } else if (self->obtainedFormat_ == AUDIO_S16SYS || self->obtainedFormat_ == AUDIO_S16LSB || self->obtainedFormat_ == AUDIO_S16MSB) {
        const int numSamples = len / int(sizeof(std::int16_t));
        std::vector<float> buf(numSamples);
        const auto* s16 = reinterpret_cast<const std::int16_t*>(stream);
        constexpr float kS16Scale = 1.0f / float(std::numeric_limits<std::int16_t>::max());
        for (int i = 0; i < numSamples; ++i)
            buf[i] = float(s16[i]) * kS16Scale;
        self->processSamples(buf.data(), numSamples);
    }
    // Unknown formats are silently dropped — volume stays 0.
}

void AudioInput::processSamples(const float* samples, int count) {
    if (count <= 0) return;

    const float gain = gain_.load();

    // Coefficients (recomputed each call is fine — cheap, supports rate changes).
    const float aBassLp   = lpCoef(250.0f,  float(sampleRate_));
    const float aMidLp    = lpCoef(2000.0f, float(sampleRate_));
    const float aTrebleLp = aMidLp; // mirror split point

    // Per-chunk accumulators.
    double sumSq      = 0.0;
    float  peak       = 0.0f;
    double bassSumSq  = 0.0;
    double midSumSq   = 0.0;
    double trebSumSq  = 0.0;

    // De-interleave to mono if multi-channel device was opened (downmix).
    const int channels = std::max(1, channels_);
    const int frames   = count / channels;

    // DC-blocking filter: y[n] = x[n] - x[n-1] + α·y[n-1]
    // α ≈ 1 − 2π·fc/fs, fc = 20 Hz. Removes the ICS43432/SPH0645 hardware
    // DC bias that would otherwise saturate the bass band at full scale.
    constexpr float kDcAlpha = 0.9974f; // 1 - 2π·20/48000

    for (int i = 0; i < frames; ++i) {
        // I2S MEMS mics (ICS43432, SPH0645LM4H) have SEL tied to GND →
        // left-channel-only output; right channel is silent. Averaging both
        // channels halves the amplitude. Take ch0 only.
        float raw = samples[i * channels];
        // DC block before gain so the bias doesn't clip post-gain.
        float dc  = raw - dcBlockX_ + kDcAlpha * dcBlockY_;
        dcBlockX_ = raw;
        dcBlockY_ = dc;
        float s   = dc * gain;

        // Band split. bassLp_ = low-pass output; trebleHp_ = high above 2kHz.
        bassLp_   += aBassLp   * (s - bassLp_);
        midLp_    += aMidLp    * (s - midLp_);
        const float midBand   = midLp_ - bassLp_;          // 250 .. 2000 Hz
        const float trebBand  = s - midLp_;                // > 2000 Hz

        sumSq      += double(s)       * double(s);
        bassSumSq  += double(bassLp_) * double(bassLp_);
        midSumSq   += double(midBand) * double(midBand);
        trebSumSq  += double(trebBand)* double(trebBand);
        peak        = std::max(peak, std::fabs(s));
    }

    const float instRms     = std::sqrt(float(sumSq      / std::max(1, frames)));
    const float instBassRms = std::sqrt(float(bassSumSq  / std::max(1, frames)));
    const float instMidRms  = std::sqrt(float(midSumSq   / std::max(1, frames)));
    const float instTrebRms = std::sqrt(float(trebSumSq  / std::max(1, frames)));
    const float instEnergy  = instRms * instRms;

    // Beat detection: compare instant energy to rolling average.
    float avgEnergy = 0.0f;
    for (float e : energyHistory_) avgEnergy += e;
    avgEnergy /= float(energyHistory_.size());

    const std::int64_t now      = monoMs();
    const float        thresh   = beatThreshold_.load();
    const int          refrac   = beatRefractoryMs_.load();
    const float        noiseFloor = 1e-4f;

    bool beat = false;
    if (avgEnergy > noiseFloor &&
        instEnergy > thresh * avgEnergy &&
        (now - lastBeatMonoMs_) > refrac) {
        beat = true;
        lastBeatMonoMs_ = now;
    }

    energyHistory_[energyHistoryIdx_] = instEnergy;
    energyHistoryIdx_ = (energyHistoryIdx_ + 1) % energyHistory_.size();

    // Publish snapshot to readers.
    {
        std::lock_guard<std::mutex> lk(featureMutex_);
        volume_ = kVolumeSmoothing * std::min(1.0f, instRms)     + (1.0f - kVolumeSmoothing) * volume_;
        peak_   = kPeakSmoothing   * std::min(1.0f, peak)        + (1.0f - kPeakSmoothing)   * peak_;
        bass_   = kBandSmoothing   * std::min(1.0f, instBassRms) + (1.0f - kBandSmoothing)   * bass_;
        mid_    = kBandSmoothing   * std::min(1.0f, instMidRms)  + (1.0f - kBandSmoothing)   * mid_;
        treble_ = kBandSmoothing   * std::min(1.0f, instTrebRms) + (1.0f - kBandSmoothing)   * treble_;
        beatFlag_ = beat;
        if (beat) beatLatch_ = true;
    }

    (void)aTrebleLp; // currently treble derived from s - midLp; aTrebleLp reserved for future split
}
