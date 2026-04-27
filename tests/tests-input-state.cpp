#include "catch.hpp"
#include <InputState.h>
#include <MatrixApplication.h>

#include <thread>
#include <vector>
#include <cmath>

// ---------------------------------------------------------------------------
// Helper: approximate float comparison (within 1 ULP)
// ---------------------------------------------------------------------------

static bool floatsEqual(float a, float b) {
    return std::fabs(a - b) < 1e-6f;
}

// ---------------------------------------------------------------------------
// 1. Default InputState — IMU snapshot is all zeros.
// ---------------------------------------------------------------------------

TEST_CASE("InputState default IMU is zero", "[input-state]") {
    InputState state;
    ImuSample sample = state.getImu();

    REQUIRE(floatsEqual(sample.ax, 0.0f));
    REQUIRE(floatsEqual(sample.ay, 0.0f));
    REQUIRE(floatsEqual(sample.az, 0.0f));
    REQUIRE(floatsEqual(sample.gx, 0.0f));
    REQUIRE(floatsEqual(sample.gy, 0.0f));
    REQUIRE(floatsEqual(sample.gz, 0.0f));
}

// ---------------------------------------------------------------------------
// 2. setImu / getImu round-trip.
// ---------------------------------------------------------------------------

TEST_CASE("InputState setImu and getImu round-trip", "[input-state]") {
    InputState state;
    ImuSample input{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    state.setImu(input);

    ImuSample output = state.getImu();
    REQUIRE(floatsEqual(output.ax, 1.0f));
    REQUIRE(floatsEqual(output.ay, 2.0f));
    REQUIRE(floatsEqual(output.az, 3.0f));
    REQUIRE(floatsEqual(output.gx, 4.0f));
    REQUIRE(floatsEqual(output.gy, 5.0f));
    REQUIRE(floatsEqual(output.gz, 6.0f));
}

// ---------------------------------------------------------------------------
// 3. setImu overwrites previous value.
// ---------------------------------------------------------------------------

TEST_CASE("InputState setImu overwrites previous value", "[input-state]") {
    InputState state;
    state.setImu(ImuSample{1, 1, 1, 1, 1, 1});
    state.setImu(ImuSample{10, 20, 30, 40, 50, 60});

    ImuSample output = state.getImu();
    REQUIRE(floatsEqual(output.ax, 10.0f));
    REQUIRE(floatsEqual(output.ay, 20.0f));
    REQUIRE(floatsEqual(output.az, 30.0f));
    REQUIRE(floatsEqual(output.gx, 40.0f));
    REQUIRE(floatsEqual(output.gy, 50.0f));
    REQUIRE(floatsEqual(output.gz, 60.0f));
}

// ---------------------------------------------------------------------------
// 4. Default audio state — volume 0, empty frequencies.
// ---------------------------------------------------------------------------

TEST_CASE("InputState default audio is zero", "[input-state]") {
    InputState state;

    REQUIRE(state.getAudioVolume() == 0);
    REQUIRE(state.getAudioFrequencies().empty());
}

// ---------------------------------------------------------------------------
// 5. setAudio / getAudio round-trip.
// ---------------------------------------------------------------------------

TEST_CASE("InputState setAudio and getAudio round-trip", "[input-state]") {
    InputState state;
    uint8_t vol = 85;
    std::vector<uint8_t> freqs = {10, 20, 30, 40};

    state.setAudio(vol, freqs);

    REQUIRE(state.getAudioVolume() == 85);
    auto got = state.getAudioFrequencies();
    REQUIRE(got.size() == freqs.size());
    for (size_t i = 0; i < freqs.size(); ++i) {
        REQUIRE(got[i] == freqs[i]);
    }
}

// ---------------------------------------------------------------------------
// 6. setAudio overwrites previous value.
// ---------------------------------------------------------------------------

TEST_CASE("InputState setAudio overwrites previous value", "[input-state]") {
    InputState state;
    state.setAudio(50, {1, 2, 3});
    state.setAudio(200, {100, 200});

    REQUIRE(state.getAudioVolume() == 200);
    auto got = state.getAudioFrequencies();
    REQUIRE(got.size() == 2);
    REQUIRE(got[0] == 100);
    REQUIRE(got[1] == 200);
}

// ---------------------------------------------------------------------------
// 7. Concurrent setters and getters — no data race, final state consistent.
// ---------------------------------------------------------------------------

TEST_CASE("InputState concurrent access is safe", "[input-state]") {
    InputState state;
    const int iterations = 1000;

    // Thread A: writes IMU values 0..iterations-1
    std::thread writer([&state, iterations]() {
        for (int i = 0; i < iterations; ++i) {
            ImuSample s;
            s.ax = static_cast<float>(i);
            s.ay = static_cast<float>(i * 2);
            s.az = static_cast<float>(i * 3);
            s.gx = static_cast<float>(i * 4);
            s.gy = static_cast<float>(i * 5);
            s.gz = static_cast<float>(i * 6);
            state.setImu(s);
        }
    });

    // Thread B: reads IMU values concurrently (no crash = pass)
    std::thread reader([&state, iterations]() {
        for (int i = 0; i < iterations; ++i) {
            state.getImu();
        }
    });

    writer.join();
    reader.join();

    // After writer finishes, final value should be iterations-1.
    ImuSample final = state.getImu();
    REQUIRE(floatsEqual(final.ax, static_cast<float>(iterations - 1)));
    REQUIRE(floatsEqual(final.ay, static_cast<float>((iterations - 1) * 2)));
    REQUIRE(floatsEqual(final.az, static_cast<float>((iterations - 1) * 3)));
}

// ---------------------------------------------------------------------------
// 8. Legacy mirror: setImu updates MatrixApplication static fields.
// ---------------------------------------------------------------------------

TEST_CASE("InputState setImu updates legacy MatrixApplication mirror", "[input-state]") {
    InputState state;
    ImuSample input{7.1f, 8.2f, 9.3f, 10.4f, 11.5f, 12.6f};
    state.setImu(input);

    // Lock the legacy mutex (same one InputState uses internally) and read.
    std::lock_guard<std::mutex> lock(MatrixApplication::simulatorImuMutex);
    REQUIRE(floatsEqual(MatrixApplication::latestSimulatorImuX, 7.1f));
    REQUIRE(floatsEqual(MatrixApplication::latestSimulatorImuY, 8.2f));
    REQUIRE(floatsEqual(MatrixApplication::latestSimulatorImuZ, 9.3f));
    REQUIRE(floatsEqual(MatrixApplication::latestSimulatorGyroX, 10.4f));
    REQUIRE(floatsEqual(MatrixApplication::latestSimulatorGyroY, 11.5f));
    REQUIRE(floatsEqual(MatrixApplication::latestSimulatorGyroZ, 12.6f));
}

// ---------------------------------------------------------------------------
// 9. Legacy mirror: setAudio updates MatrixApplication static fields.
// ---------------------------------------------------------------------------

TEST_CASE("InputState setAudio updates legacy MatrixApplication mirror", "[input-state]") {
    InputState state;
    uint8_t vol = 170;
    std::vector<uint8_t> freqs = {5, 10, 15};
    state.setAudio(vol, freqs);

    // Lock the legacy mutex and read.
    std::lock_guard<std::mutex> lock(MatrixApplication::audioDataMutex);
    REQUIRE(MatrixApplication::latestAudioVolume == 170);
    REQUIRE(MatrixApplication::latestAudioFrequencies.size() == freqs.size());
    for (size_t i = 0; i < freqs.size(); ++i) {
        REQUIRE(MatrixApplication::latestAudioFrequencies[i] == freqs[i]);
    }
}
