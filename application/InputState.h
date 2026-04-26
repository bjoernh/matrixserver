#ifndef MATRIXSERVER_INPUTSTATE_H
#define MATRIXSERVER_INPUTSTATE_H

#include <cstdint>
#include <mutex>
#include <vector>

// ---------------------------------------------------------------------------
// ImuSample — snapshot of one IMU reading
// ---------------------------------------------------------------------------

struct ImuSample {
    float ax = 0.0f;
    float ay = 0.0f;
    float az = 0.0f;
    float gx = 0.0f;
    float gy = 0.0f;
    float gz = 0.0f;
};

// ---------------------------------------------------------------------------
// InputState
//
// Centralises the write-side of the IMU and audio state that arrives via the
// matrixserver message dispatcher.  The legacy public-static mirror fields in
// MatrixApplication (latestSimulatorImu*, latestAudioVolume, …) remain the
// public read API consumed by example apps.  InputState::setImu() and
// setAudio() update *both* the internal snapshot and those legacy fields under
// the same mutexes, keeping the two in lock-step.
//
// New code should prefer the getter pair (getImu / getAudio) which return
// by-value snapshots and are fully thread-safe.
// ---------------------------------------------------------------------------

class MatrixApplication; // forward declaration for the dual-write helpers

class InputState {
public:
    // setImu — called by the message-dispatcher handler.
    // Writes the internal snapshot AND the legacy MatrixApplication static
    // fields under MatrixApplication::simulatorImuMutex.
    void setImu(const ImuSample& sample);

    // setAudio — called by the message-dispatcher handler.
    // Writes internal snapshot AND legacy MatrixApplication static fields
    // under MatrixApplication::audioDataMutex.
    void setAudio(uint8_t volume, const std::vector<uint8_t>& frequencies);

    // Thread-safe getters — return a by-value copy.
    ImuSample getImu() const;

    uint8_t getAudioVolume() const;
    std::vector<uint8_t> getAudioFrequencies() const;

private:
    // Internal mirror (separate from the legacy statics so that InputState
    // can be used independently in tests or non-MatrixApplication contexts).
    mutable std::mutex imuMutex_;
    ImuSample imuSample_;

    mutable std::mutex audioMutex_;
    uint8_t audioVolume_ = 0;
    std::vector<uint8_t> audioFrequencies_;
};

#endif // MATRIXSERVER_INPUTSTATE_H
