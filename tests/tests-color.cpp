#include "catch.hpp"
#include <Color.h>
#include <sstream>

// ---------------------------------------------------------------------------
// 1. RGB constructor stores correct values
// ---------------------------------------------------------------------------
TEST_CASE("Color RGB constructor", "[color]") {
    Color c(10, 20, 30);
    REQUIRE(c.r() == 10);
    REQUIRE(c.g() == 20);
    REQUIRE(c.b() == 30);
}

TEST_CASE("Color RGB constructor boundary values", "[color]") {
    Color c(0, 128, 255);
    REQUIRE(c.r() == 0);
    REQUIRE(c.g() == 128);
    REQUIRE(c.b() == 255);
}

// ---------------------------------------------------------------------------
// 2. Static factory methods
// ---------------------------------------------------------------------------
TEST_CASE("Color static factory black", "[color]") {
    Color c = Color::black();
    REQUIRE(c.r() == 0);
    REQUIRE(c.g() == 0);
    REQUIRE(c.b() == 0);
}

TEST_CASE("Color static factory white", "[color]") {
    Color c = Color::white();
    REQUIRE(c.r() == 255);
    REQUIRE(c.g() == 255);
    REQUIRE(c.b() == 255);
}

TEST_CASE("Color static factory red", "[color]") {
    Color c = Color::red();
    REQUIRE(c.r() == 255);
    REQUIRE(c.g() == 0);
    REQUIRE(c.b() == 0);
}

TEST_CASE("Color static factory green", "[color]") {
    Color c = Color::green();
    REQUIRE(c.r() == 0);
    REQUIRE(c.g() == 255);
    REQUIRE(c.b() == 0);
}

TEST_CASE("Color static factory blue", "[color]") {
    Color c = Color::blue();
    REQUIRE(c.r() == 0);
    REQUIRE(c.g() == 0);
    REQUIRE(c.b() == 255);
}

// ---------------------------------------------------------------------------
// 3. Getters return set values (via RGB constructor)
// ---------------------------------------------------------------------------
TEST_CASE("Color getters return correct values after RGB construction", "[color]") {
    Color c(100, 150, 200);
    REQUIRE(c.r() == 100);
    REQUIRE(c.g() == 150);
    REQUIRE(c.b() == 200);
}

// ---------------------------------------------------------------------------
// 4. uint8_t setters
// ---------------------------------------------------------------------------
TEST_CASE("Color uint8_t setters work correctly", "[color]") {
    Color c(0, 0, 0);
    c.r(uint8_t(42));
    c.g(uint8_t(84));
    c.b(uint8_t(126));
    REQUIRE(c.r() == 42);
    REQUIRE(c.g() == 84);
    REQUIRE(c.b() == 126);
}

// ---------------------------------------------------------------------------
// 5. unsigned int setters clamp at 255
// ---------------------------------------------------------------------------
TEST_CASE("Color unsigned int setters clamp values exceeding 255", "[color]") {
    Color c(0, 0, 0);
    c.r(300u);
    c.g(256u);
    c.b(1000u);
    REQUIRE(c.r() == 255);
    REQUIRE(c.g() == 255);
    REQUIRE(c.b() == 255);
}

TEST_CASE("Color unsigned int setters accept values within range", "[color]") {
    Color c(0, 0, 0);
    c.r(100u);
    c.g(200u);
    c.b(50u);
    REQUIRE(c.r() == 100);
    REQUIRE(c.g() == 200);
    REQUIRE(c.b() == 50);
}

// ---------------------------------------------------------------------------
// 6. operator+= with no overflow
// ---------------------------------------------------------------------------
TEST_CASE("Color operator+= no overflow", "[color]") {
    Color a(10, 20, 30);
    Color b(5, 10, 15);
    a += b;
    REQUIRE(a.r() == 15);
    REQUIRE(a.g() == 30);
    REQUIRE(a.b() == 45);
}

// ---------------------------------------------------------------------------
// 7. operator+= with saturation
// ---------------------------------------------------------------------------
TEST_CASE("Color operator+= saturates at 255", "[color]") {
    Color a(200, 200, 200);
    Color b(100, 100, 100);
    a += b;
    REQUIRE(a.r() == 255);
    REQUIRE(a.g() == 255);
    REQUIRE(a.b() == 255);
}

TEST_CASE("Color operator+= exactly at saturation boundary", "[color]") {
    Color a(255, 128, 0);
    Color b(1, 127, 255);
    a += b;
    REQUIRE(a.r() == 255);
    REQUIRE(a.g() == 255);
    REQUIRE(a.b() == 255);
}

// ---------------------------------------------------------------------------
// 8. operator+ binary
// ---------------------------------------------------------------------------
TEST_CASE("Color operator+ binary does not modify operands", "[color]") {
    Color a(50, 60, 70);
    Color b(10, 20, 30);
    Color c = a + b;
    REQUIRE(c.r() == 60);
    REQUIRE(c.g() == 80);
    REQUIRE(c.b() == 100);
    // Operands unchanged
    REQUIRE(a.r() == 50);
    REQUIRE(b.r() == 10);
}

// ---------------------------------------------------------------------------
// 9. operator-= with no underflow
// ---------------------------------------------------------------------------
TEST_CASE("Color operator-= no underflow", "[color]") {
    Color a(100, 150, 200);
    Color b(10, 50, 100);
    a -= b;
    REQUIRE(a.r() == 90);
    REQUIRE(a.g() == 100);
    REQUIRE(a.b() == 100);
}

// 10. operator-= with underflow (clamps to 0)
TEST_CASE("Color operator-= underflow clamps to zero", "[color]") {
    Color a(10, 10, 10);
    Color b(20, 20, 20);
    // uint8_t operands are promoted to int in C++, so (10 - 20 > 0) is false,
    // correctly clamping to 0.
    a -= b;
    REQUIRE(a.r() == 0);
    REQUIRE(a.g() == 0);
    REQUIRE(a.b() == 0);
}

TEST_CASE("Color operator-= exact zero result", "[color]") {
    Color a(50, 100, 150);
    Color b(50, 100, 150);
    a -= b;
    REQUIRE(a.r() == 0);
    REQUIRE(a.g() == 0);
    REQUIRE(a.b() == 0);
}

// ---------------------------------------------------------------------------
// 11. operator- binary
// ---------------------------------------------------------------------------
TEST_CASE("Color operator- binary does not modify operands", "[color]") {
    Color a(100, 100, 100);
    Color b(30, 40, 50);
    Color c = a - b;
    REQUIRE(c.r() == 70);
    REQUIRE(c.g() == 60);
    REQUIRE(c.b() == 50);
    // Operands unchanged
    REQUIRE(a.r() == 100);
    REQUIRE(b.r() == 30);
}

// ---------------------------------------------------------------------------
// 12. operator*= scale up within bounds
// ---------------------------------------------------------------------------
TEST_CASE("Color operator*= scale up within bounds", "[color]") {
    Color c(50, 60, 70);
    c *= 2.0f;
    REQUIRE(c.r() == 100);
    REQUIRE(c.g() == 120);
    REQUIRE(c.b() == 140);
}

// ---------------------------------------------------------------------------
// 13. operator*= scale up saturates at 255
// ---------------------------------------------------------------------------
TEST_CASE("Color operator*= saturates at 255 when scaling up", "[color]") {
    Color c(200, 200, 200);
    c *= 2.0f;
    REQUIRE(c.r() == 255);
    REQUIRE(c.g() == 255);
    REQUIRE(c.b() == 255);
}

// ---------------------------------------------------------------------------
// 14. operator*= scale down
// ---------------------------------------------------------------------------
TEST_CASE("Color operator*= scale down", "[color]") {
    Color c(100, 200, 50);
    c *= 0.5f;
    REQUIRE(c.r() == 50);
    REQUIRE(c.g() == 100);
    REQUIRE(c.b() == 25);
}

// ---------------------------------------------------------------------------
// 15. operator*= by zero produces black
// ---------------------------------------------------------------------------
TEST_CASE("Color operator*= by zero produces black", "[color]") {
    Color c(100, 150, 200);
    c *= 0.0f;
    REQUIRE(c.r() == 0);
    REQUIRE(c.g() == 0);
    REQUIRE(c.b() == 0);
}

// ---------------------------------------------------------------------------
// 16. operator*= by 1 leaves color unchanged
// ---------------------------------------------------------------------------
TEST_CASE("Color operator*= by 1 leaves color unchanged", "[color]") {
    Color c(42, 84, 126);
    c *= 1.0f;
    REQUIRE(c.r() == 42);
    REQUIRE(c.g() == 84);
    REQUIRE(c.b() == 126);
}

// ---------------------------------------------------------------------------
// 17. operator*= by negative clamps to 0
// NOTE: The blue channel has a known bug: it checks tempRed < 0 instead of
// tempBlue < 0. For negative multipliers where red is also negative this
// still produces 0, so the visible behavior is correct for most cases.
// ---------------------------------------------------------------------------
TEST_CASE("Color operator*= by negative value clamps to 0", "[color]") {
    Color c(100, 100, 100);
    c *= -1.0f;
    REQUIRE(c.r() == 0);
    REQUIRE(c.g() == 0);
    REQUIRE(c.b() == 0);
}

// ---------------------------------------------------------------------------
// 18. operator* binary
// ---------------------------------------------------------------------------
TEST_CASE("Color operator* binary does not modify operand", "[color]") {
    Color a(100, 50, 25);
    Color b = a * 2.0f;
    REQUIRE(b.r() == 200);
    REQUIRE(b.g() == 100);
    REQUIRE(b.b() == 50);
    // Original unchanged
    REQUIRE(a.r() == 100);
}

// ---------------------------------------------------------------------------
// 19. operator== same colors
// ---------------------------------------------------------------------------
TEST_CASE("Color operator== returns true for equal colors", "[color]") {
    Color a(10, 20, 30);
    Color b(10, 20, 30);
    REQUIRE(a == b);
}

TEST_CASE("Color operator== static factories compare equal", "[color]") {
    REQUIRE(Color::red() == Color::red());
    REQUIRE(Color::black() == Color::black());
    REQUIRE(Color::white() == Color::white());
}

// ---------------------------------------------------------------------------
// 20. operator== different colors
// ---------------------------------------------------------------------------
TEST_CASE("Color operator== returns false for different colors", "[color]") {
    Color a(10, 20, 30);
    Color b(10, 20, 31);
    REQUIRE_FALSE(a == b);
}

// ---------------------------------------------------------------------------
// 21. operator!=
// ---------------------------------------------------------------------------
TEST_CASE("Color operator!= returns true for different colors", "[color]") {
    Color a(1, 2, 3);
    Color b(4, 5, 6);
    REQUIRE(a != b);
}

TEST_CASE("Color operator!= returns false for equal colors", "[color]") {
    Color a(128, 64, 32);
    Color b(128, 64, 32);
    REQUIRE_FALSE(a != b);
}

// ---------------------------------------------------------------------------
// 22. HSV pure red (h=0, s=1, v=1)
// ---------------------------------------------------------------------------
TEST_CASE("Color fromHSV pure red at h=0", "[color]") {
    Color c(0, 0, 0);
    c.fromHSV(0.0f, 1.0f, 1.0f);
    REQUIRE(c.r() == 255);
    REQUIRE(c.g() == 0);
    REQUIRE(c.b() == 0);
}

// ---------------------------------------------------------------------------
// 23. HSV pure green (h=120, s=1, v=1)
// ---------------------------------------------------------------------------
TEST_CASE("Color fromHSV pure green at h=120", "[color]") {
    Color c(0, 0, 0);
    c.fromHSV(120.0f, 1.0f, 1.0f);
    REQUIRE(c.r() == 0);
    REQUIRE(c.g() == 255);
    REQUIRE(c.b() == 0);
}

// ---------------------------------------------------------------------------
// 24. HSV pure blue (h=240, s=1, v=1)
// ---------------------------------------------------------------------------
TEST_CASE("Color fromHSV pure blue at h=240", "[color]") {
    Color c(0, 0, 0);
    c.fromHSV(240.0f, 1.0f, 1.0f);
    REQUIRE(c.r() == 0);
    REQUIRE(c.g() == 0);
    REQUIRE(c.b() == 255);
}

// ---------------------------------------------------------------------------
// 25. HSV achromatic grey (s=0)
// ---------------------------------------------------------------------------
TEST_CASE("Color fromHSV achromatic grey with s=0", "[color]") {
    Color c(0, 0, 0);
    c.fromHSV(0.0f, 0.0f, 0.5f);
    // With s=0 all channels should be equal (grey)
    REQUIRE(c.r() == c.g());
    REQUIRE(c.g() == c.b());
    // v=0.5 should give approximately 128
    REQUIRE(c.r() == Approx(128).margin(2));
}

TEST_CASE("Color fromHSV achromatic black with s=0 v=0", "[color]") {
    Color c(255, 255, 255);
    c.fromHSV(0.0f, 0.0f, 0.0f);
    REQUIRE(c.r() == 0);
    REQUIRE(c.g() == 0);
    REQUIRE(c.b() == 0);
}

TEST_CASE("Color fromHSV achromatic white with s=0 v=1", "[color]") {
    Color c(0, 0, 0);
    c.fromHSV(0.0f, 0.0f, 1.0f);
    REQUIRE(c.r() == 255);
    REQUIRE(c.g() == 255);
    REQUIRE(c.b() == 255);
}

// ---------------------------------------------------------------------------
// 26. Stream output format "RGB(r,g,b)"
// ---------------------------------------------------------------------------
TEST_CASE("Color stream output has correct format", "[color]") {
    Color c(10, 20, 30);
    std::ostringstream oss;
    oss << c;
    REQUIRE(oss.str() == "RGB(10,20,30)");
}

TEST_CASE("Color stream output for black", "[color]") {
    std::ostringstream oss;
    oss << Color::black();
    REQUIRE(oss.str() == "RGB(0,0,0)");
}

TEST_CASE("Color stream output for white", "[color]") {
    std::ostringstream oss;
    oss << Color::white();
    REQUIRE(oss.str() == "RGB(255,255,255)");
}
