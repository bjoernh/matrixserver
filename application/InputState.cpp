#include "InputState.h"
#include "MatrixApplication.h"

// ---------------------------------------------------------------------------
// setImu
//
// Dual-write: update the internal ImuSample snapshot AND the legacy
// MatrixApplication public-static mirror fields.  Both are written under
// MatrixApplication::simulatorImuMutex so that example apps that lock that
// mutex before reading the legacy fields see a consistent snapshot.
// ---------------------------------------------------------------------------

void InputState::setImu(const ImuSample& sample) {
    std::lock_guard<std::mutex> legacyLock(MatrixApplication::simulatorImuMutex);

    // Update legacy mirror fields (read by example apps via MatrixApplication::).
    MatrixApplication::latestSimulatorImuX = sample.ax;
    MatrixApplication::latestSimulatorImuY = sample.ay;
    MatrixApplication::latestSimulatorImuZ = sample.az;
    MatrixApplication::latestSimulatorGyroX = sample.gx;
    MatrixApplication::latestSimulatorGyroY = sample.gy;
    MatrixApplication::latestSimulatorGyroZ = sample.gz;

    // Update internal snapshot under its own mutex (held while legacyLock is
    // already held — consistent ordering: always legacyLock first, then
    // imuMutex_; never the reverse).
    std::lock_guard<std::mutex> internalLock(imuMutex_);
    imuSample_ = sample;
}

// ---------------------------------------------------------------------------
// setAudio
//
// Dual-write: update the internal audio snapshot AND the legacy
// MatrixApplication public-static mirror fields under audioDataMutex.
// ---------------------------------------------------------------------------

void InputState::setAudio(uint8_t volume, const std::vector<uint8_t>& frequencies) {
    std::lock_guard<std::mutex> legacyLock(MatrixApplication::audioDataMutex);

    // Update legacy mirror fields.
    MatrixApplication::latestAudioVolume = volume;
    MatrixApplication::latestAudioFrequencies = frequencies;

    // Update internal snapshot.
    std::lock_guard<std::mutex> internalLock(audioMutex_);
    audioVolume_ = volume;
    audioFrequencies_ = frequencies;
}

// ---------------------------------------------------------------------------
// Getters — return by-value snapshots; fully thread-safe.
// ---------------------------------------------------------------------------

ImuSample InputState::getImu() const {
    std::lock_guard<std::mutex> lock(imuMutex_);
    return imuSample_;
}

uint8_t InputState::getAudioVolume() const {
    std::lock_guard<std::mutex> lock(audioMutex_);
    return audioVolume_;
}

std::vector<uint8_t> InputState::getAudioFrequencies() const {
    std::lock_guard<std::mutex> lock(audioMutex_);
    return audioFrequencies_;
}
