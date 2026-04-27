#include "catch.hpp"
#include <Screen.h>
#include <chrono>

using namespace std::chrono;

// ---------------------------------------------------------------------------
// Functional tests
// All tests use a 16x16 screen unless a smaller size is more appropriate.
// ---------------------------------------------------------------------------

TEST_CASE("Screen construction properties", "[screen]") {
    Screen screen(16, 16, 42);
    CHECK(screen.getWidth() == 16);
    CHECK(screen.getHeight() == 16);
    CHECK(screen.getScreenId() == 42);
}

TEST_CASE("Screen data size", "[screen]") {
    Screen screen(16, 16, 0);
    CHECK(screen.getScreenDataSize() == 16 * 16);
}

TEST_CASE("Screen initial state is black", "[screen]") {
    Screen screen(16, 16, 0);
    for (int x = 0; x < screen.getWidth(); ++x) {
        for (int y = 0; y < screen.getHeight(); ++y) {
            Color px = screen.getPixel(x, y);
            CHECK(px.r() == 0);
            CHECK(px.g() == 0);
            CHECK(px.b() == 0);
        }
    }
}

TEST_CASE("Screen setPixel and getPixel", "[screen]") {
    Screen screen(16, 16, 0);

    // Top-left corner
    screen.setPixel(0, 0, Color(10, 20, 30));
    CHECK(screen.getPixel(0, 0) == Color(10, 20, 30));

    // Arbitrary interior pixel
    screen.setPixel(7, 5, Color(100, 150, 200));
    CHECK(screen.getPixel(7, 5) == Color(100, 150, 200));

    // Bottom-right corner
    screen.setPixel(15, 15, Color(255, 128, 64));
    CHECK(screen.getPixel(15, 15) == Color(255, 128, 64));
}

TEST_CASE("Screen setPixel RGB overload", "[screen]") {
    Screen screen(16, 16, 0);
    screen.setPixel(3, 7, 11, 22, 33);
    Color px = screen.getPixel(3, 7);
    CHECK(px.r() == 11);
    CHECK(px.g() == 22);
    CHECK(px.b() == 33);
}

TEST_CASE("Screen out-of-bounds setPixel", "[screen]") {
    Screen screen(16, 16, 0);
    // These must not crash and must not modify any in-bounds pixel
    screen.setPixel(-1, 0, Color::red());
    screen.setPixel(0, -1, Color::red());
    screen.setPixel(16, 0, Color::red());
    screen.setPixel(0, 16, Color::red());
    screen.setPixel(16, 16, Color::red());
    screen.setPixel(-5, -5, Color::red());

    // Verify (0,0) is untouched
    CHECK(screen.getPixel(0, 0) == Color(0, 0, 0));
}

TEST_CASE("Screen out-of-bounds getPixel", "[screen]") {
    Screen screen(16, 16, 0);
    screen.fill(Color::red()); // Make all in-bounds pixels non-black

    CHECK(screen.getPixel(-1, 0) == Color(0, 0, 0));
    CHECK(screen.getPixel(0, -1) == Color(0, 0, 0));
    CHECK(screen.getPixel(16, 0) == Color(0, 0, 0));
    CHECK(screen.getPixel(0, 16) == Color(0, 0, 0));
}

TEST_CASE("Screen corner pixels", "[screen]") {
    Screen screen(16, 16, 0);
    int w = screen.getWidth();
    int h = screen.getHeight();

    screen.setPixel(0, 0, Color(1, 2, 3));
    screen.setPixel(w - 1, 0, Color(4, 5, 6));
    screen.setPixel(0, h - 1, Color(7, 8, 9));
    screen.setPixel(w - 1, h - 1, Color(10, 11, 12));

    CHECK(screen.getPixel(0, 0) == Color(1, 2, 3));
    CHECK(screen.getPixel(w - 1, 0) == Color(4, 5, 6));
    CHECK(screen.getPixel(0, h - 1) == Color(7, 8, 9));
    CHECK(screen.getPixel(w - 1, h - 1) == Color(10, 11, 12));
}

TEST_CASE("Screen additive blending", "[screen]") {
    Screen screen(16, 16, 0);

    screen.setPixel(4, 4, Color(100, 50, 25));
    // Additive blend: result should be clamped to [0, 255] per channel
    screen.setPixel(4, 4, Color(50, 50, 50), /*add=*/true);

    Color px = screen.getPixel(4, 4);
    CHECK(px.r() == 150);
    CHECK(px.g() == 100);
    CHECK(px.b() == 75);

    // Test saturation: adding to a value already at 255 must not overflow
    screen.setPixel(5, 5, Color(200, 200, 200));
    screen.setPixel(5, 5, Color(100, 100, 100), /*add=*/true);
    Color px2 = screen.getPixel(5, 5);
    CHECK(px2.r() == 255);
    CHECK(px2.g() == 255);
    CHECK(px2.b() == 255);
}

TEST_CASE("Screen array index is column-major", "[screen]") {
    Screen screen(16, 16, 0);
    int h = screen.getHeight();

    CHECK(screen.getArrayIndex(0, 0) == 0);
    CHECK(screen.getArrayIndex(1, 0) == h); // next column
    CHECK(screen.getArrayIndex(0, 1) == 1); // next row in same column
    CHECK(screen.getArrayIndex(2, 3) == 3 + 2 * h);
}

TEST_CASE("Screen clear", "[screen]") {
    Screen screen(16, 16, 0);
    screen.fill(Color(50, 100, 150));
    screen.clear();

    for (int x = 0; x < screen.getWidth(); ++x) {
        for (int y = 0; y < screen.getHeight(); ++y) {
            CHECK(screen.getPixel(x, y) == Color(0, 0, 0));
        }
    }
}

TEST_CASE("Screen fill Color", "[screen]") {
    Screen screen(16, 16, 0);
    Color target(30, 60, 90);
    screen.fill(target);

    for (int x = 0; x < screen.getWidth(); ++x) {
        for (int y = 0; y < screen.getHeight(); ++y) {
            CHECK(screen.getPixel(x, y) == target);
        }
    }
}

TEST_CASE("Screen fill RGB", "[screen]") {
    Screen screen(16, 16, 0);
    screen.fill(10, 20, 30);

    for (int x = 0; x < screen.getWidth(); ++x) {
        for (int y = 0; y < screen.getHeight(); ++y) {
            Color px = screen.getPixel(x, y);
            CHECK(px.r() == 10);
            CHECK(px.g() == 20);
            CHECK(px.b() == 30);
        }
    }
}

TEST_CASE("Screen fade half", "[screen]") {
    Screen screen(16, 16, 0);
    screen.fill(Color(200, 100, 50));
    screen.fade(0.5f);

    // Each channel should be roughly half; allow ±1 for integer rounding
    for (int x = 0; x < screen.getWidth(); ++x) {
        for (int y = 0; y < screen.getHeight(); ++y) {
            Color px = screen.getPixel(x, y);
            CHECK(px.r() >= 99);
            CHECK(px.r() <= 101);
            CHECK(px.g() >= 49);
            CHECK(px.g() <= 51);
            CHECK(px.b() >= 24);
            CHECK(px.b() <= 26);
        }
    }
}

TEST_CASE("Screen fade boundary", "[screen]") {
    Screen screen(16, 16, 0);
    screen.fill(Color(128, 128, 128));

    // fade(0.0) must be a no-op (guard: factor must be > 0)
    screen.fade(0.0f);
    CHECK(screen.getPixel(0, 0) == Color(128, 128, 128));

    // fade(1.0) must be a no-op (guard: factor must be < 1)
    screen.fade(1.0f);
    CHECK(screen.getPixel(0, 0) == Color(128, 128, 128));
}

TEST_CASE("Screen rotation metadata", "[screen]") {
    Screen screen(16, 16, 0);

    screen.setRotation(Rotation::rot0);
    CHECK(screen.getRotation() == Rotation::rot0);

    screen.setRotation(Rotation::rot90);
    CHECK(screen.getRotation() == Rotation::rot90);

    screen.setRotation(Rotation::rot180);
    CHECK(screen.getRotation() == Rotation::rot180);

    screen.setRotation(Rotation::rot270);
    CHECK(screen.getRotation() == Rotation::rot270);
}

TEST_CASE("Screen offset metadata", "[screen]") {
    Screen screen(16, 16, 0);

    screen.setOffsetX(7);
    CHECK(screen.getOffsetX() == 7);

    screen.setOffsetY(13);
    CHECK(screen.getOffsetY() == 13);

    screen.setOffsetX(-3);
    CHECK(screen.getOffsetX() == -3);
}

TEST_CASE("Screen setScreenId", "[screen]") {
    Screen screen(16, 16, 0);
    screen.setScreenId(99);
    CHECK(screen.getScreenId() == 99);
}

TEST_CASE("Screen setScreenData pointer", "[screen]") {
    Screen screen(16, 16, 0);
    int size = screen.getScreenDataSize();

    // Build a replacement buffer filled with a known color
    std::vector<Color> buf(size, Color(11, 22, 33));
    screen.setScreenData(buf.data());

    // Every pixel should now reflect the new data
    for (int x = 0; x < screen.getWidth(); ++x) {
        for (int y = 0; y < screen.getHeight(); ++y) {
            CHECK(screen.getPixel(x, y) == Color(11, 22, 33));
        }
    }
}

TEST_CASE("Screen setScreenData vector", "[screen]") {
    Screen screen(16, 16, 0);
    int size = screen.getScreenDataSize();

    std::vector<Color> buf(size, Color(44, 55, 66));
    screen.setScreenData(buf);

    for (int x = 0; x < screen.getWidth(); ++x) {
        for (int y = 0; y < screen.getHeight(); ++y) {
            CHECK(screen.getPixel(x, y) == Color(44, 55, 66));
        }
    }
}

// ---------------------------------------------------------------------------
// Performance benchmark (original test, preserved)
// ---------------------------------------------------------------------------

TEST_CASE("Screen clear speed test", "[screen]") {
    const int numberOfClears = 10000;
    Screen screen0(64, 64, 0);
    auto msStart = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    for (int i = 0; i < numberOfClears; i++) {
        screen0.fill(Color::red());
        screen0.clear();
    }
    auto msTotal = duration_cast<milliseconds>(system_clock::now().time_since_epoch()) - msStart;
    WARN("Total Time for " << numberOfClears << " fill&clear events: " << msTotal.count() << " ms");
    CHECK(msTotal.count() < 500);
}
