#ifndef MATRIXSERVER_AUDIOINPUT_H
#define MATRIXSERVER_AUDIOINPUT_H

#include <Eigen/Dense>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

/**
 * Mic capture + lightweight feature extraction for audio-reactive animations.
 *
 * Uses SDL2 audio capture in a background callback thread. Computes:
 *   - smoothed RMS volume (0..1, post-gain)
 *   - smoothed peak (0..1, post-gain)
 *   - energy-based beat / onset detection (latched, consumable)
 *   - bass/mid/treble band energies via one-pole IIR filters (0..1)
 *
 * No FFT yet — the band split uses cheap IIR filters so the API is in place
 * for animations to consume bass/mid/treble without paying for a real FFT.
 *
 * Thread-safe: callback writes feature snapshot under a mutex; getters read
 * under the same mutex. Tuning setters use atomics.
 *
 * Construction never throws. init() returns false if SDL audio can't open;
 * isAvailable() reports the result. When unavailable, getters return zeros /
 * false so callers can fall back to non-audio behaviour without branching.
 */
class AudioInput {
public:
    AudioInput();
    ~AudioInput();

    AudioInput(const AudioInput&) = delete;
    AudioInput& operator=(const AudioInput&) = delete;

    /// Open the default capture device. Auto-starts capture on success.
    /// Safe to call multiple times — re-init closes the previous device.
    /// preferredDevice: empty string = SDL default; otherwise the device name.
    bool init(const std::string& preferredDevice = "");

    bool isAvailable() const { return available_; }

    void start();
    void stop();

    /// When false, feature getters return non-audio defaults (0 / false) and
    /// beat latches are cleared. Capture keeps running so re-enable is instant.
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_.load(); }

    void setGain(float gain) { gain_.store(gain); }
    float getGain() const { return gain_.load(); }

    /// Beat trigger threshold as a ratio of instant-to-average energy.
    /// Typical: 1.3 (sensitive) .. 1.8 (selective). Default 1.4.
    void setBeatThreshold(float t) { beatThreshold_.store(t); }
    float getBeatThreshold() const { return beatThreshold_.load(); }

    /// Minimum gap between beats in milliseconds. Default 180.
    void setBeatRefractoryMs(int ms) { beatRefractoryMs_.store(ms); }

    /// Smoothed RMS volume in [0, 1] after gain.
    float getVolume() const;

    /// Smoothed peak amplitude in [0, 1] after gain.
    float getPeak() const;

    /// True if a beat was detected in the most recent chunk.
    /// Non-consuming — multiple readers can observe the same beat.
    bool isBeat() const;

    /// Returns true exactly once per detected beat (latched then cleared).
    /// Use this for one-shot triggers (color flip, spawn burst).
    bool consumeBeat();

    /// Band energies, all in [0, 1]: (bass, mid, treble).
    /// Bass < ~250 Hz, mid ~250–2000 Hz, treble > ~2000 Hz.
    Eigen::Vector3f getBands() const;

    /// Generate and play a sine-wave beep on the default output device.
    /// Non-blocking — audio plays in a background SDL audio queue.
    /// freqHz: tone pitch (default 440 Hz A4)
    /// durationMs: length in milliseconds (default 1000)
    /// amplitude: linear amplitude 0..1 (default 0.3)
    void playBeep(float freqHz = 440.0f, float durationMs = 1000.0f, float amplitude = 0.3f);

private:
    static void audioCallback(void* userdata, std::uint8_t* stream, int len);
    void processSamples(const float* samples, int count);

    std::uint32_t deviceId_;        ///< SDL capture device id; 0 = invalid.
    std::uint32_t outputDeviceId_;  ///< SDL output device id for playBeep; 0 = invalid.
    std::uint16_t obtainedFormat_;  ///< SDL_AudioFormat actually opened (may differ from F32).
    bool available_;
    std::atomic<bool> enabled_;

    std::atomic<float> gain_;
    std::atomic<float> beatThreshold_;
    std::atomic<int>   beatRefractoryMs_;

    // Feature snapshot guarded by featureMutex_.
    mutable std::mutex featureMutex_;
    float volume_;
    float peak_;
    float bass_;
    float mid_;
    float treble_;
    bool  beatFlag_;
    mutable bool beatLatch_;  ///< cleared by consumeBeat()

    // Beat-detection rolling energy history (callback thread only).
    std::vector<float> energyHistory_;
    std::size_t energyHistoryIdx_;
    std::int64_t lastBeatMonoMs_;

    // IIR filter states (callback thread only).
    float dcBlockX_;   ///< previous raw input sample (DC blocker)
    float dcBlockY_;   ///< previous DC-blocked output sample
    float bassLp_;
    float midLp_;
    float midHp_;
    float trebleHp_;

    int sampleRate_;
    int channels_;
};

#endif // MATRIXSERVER_AUDIOINPUT_H
