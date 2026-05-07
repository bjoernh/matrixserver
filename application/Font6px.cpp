#include "Font6px.h"

namespace CharacterBitmaps {
// ---------------------------------------------------------------------------
// Glyph data — defined once here; not in the header.
// Internal linkage via anonymous namespace avoids polluting the linker
// namespace while still being accessible to the functions below.
// ---------------------------------------------------------------------------
namespace {
const Bitmap1bpp char_0 = {
    Vector2i(0, 0), Vector2i(1, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(0, 2),
    Vector2i(2, 2), Vector2i(0, 3), Vector2i(2, 3), Vector2i(0, 4), Vector2i(1, 4), Vector2i(2, 4),
};

const Bitmap1bpp char_1 = {
    Vector2i(1, 0), Vector2i(1, 1), Vector2i(1, 2), Vector2i(1, 3), Vector2i(1, 4),
};

const Bitmap1bpp char_2 = {
    Vector2i(0, 0), Vector2i(1, 0), Vector2i(2, 0), Vector2i(2, 1), Vector2i(2, 2), Vector2i(1, 2),
    Vector2i(0, 2), Vector2i(0, 3), Vector2i(0, 4), Vector2i(1, 4), Vector2i(2, 4),
};

const Bitmap1bpp char_3 = {
    Vector2i(0, 0), Vector2i(1, 0), Vector2i(2, 0), Vector2i(2, 1), Vector2i(2, 2),
    Vector2i(1, 2), Vector2i(2, 3), Vector2i(0, 4), Vector2i(1, 4), Vector2i(2, 4),
};

const Bitmap1bpp char_4 = {
    Vector2i(0, 0), Vector2i(0, 1), Vector2i(0, 2), Vector2i(1, 2), Vector2i(2, 0), Vector2i(2, 1), Vector2i(2, 2), Vector2i(2, 3), Vector2i(2, 4),
};

const Bitmap1bpp char_5 = {
    Vector2i(0, 0), Vector2i(1, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(2, 2), Vector2i(1, 2),
    Vector2i(0, 2), Vector2i(2, 3), Vector2i(0, 4), Vector2i(1, 4), Vector2i(2, 4),
};

const Bitmap1bpp char_6 = {
    Vector2i(0, 0), Vector2i(1, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(1, 2), Vector2i(0, 2),
    Vector2i(2, 2), Vector2i(0, 3), Vector2i(2, 3), Vector2i(0, 4), Vector2i(1, 4), Vector2i(2, 4),
};

const Bitmap1bpp char_7 = {
    Vector2i(0, 0), Vector2i(1, 0), Vector2i(2, 0), Vector2i(2, 1), Vector2i(2, 2), Vector2i(2, 3), Vector2i(2, 4),
};

const Bitmap1bpp char_8 = {
    Vector2i(0, 0), Vector2i(1, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(0, 2), Vector2i(2, 2),
    Vector2i(0, 3), Vector2i(2, 3), Vector2i(0, 4), Vector2i(1, 4), Vector2i(2, 4), Vector2i(1, 2),
};

const Bitmap1bpp char_9 = {
    Vector2i(0, 0), Vector2i(1, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(0, 2),
    Vector2i(2, 2), Vector2i(2, 3), Vector2i(0, 4), Vector2i(1, 4), Vector2i(2, 4), Vector2i(1, 2),
};

const Bitmap1bpp char_A = {
    Vector2i(1, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(0, 2), Vector2i(2, 2),
    Vector2i(0, 3), Vector2i(2, 3), Vector2i(0, 4), Vector2i(2, 4), Vector2i(1, 2),
};

const Bitmap1bpp char_B = {
    Vector2i(0, 0), Vector2i(1, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(0, 2), Vector2i(2, 2),
    Vector2i(0, 3), Vector2i(2, 3), Vector2i(0, 4), Vector2i(1, 4), Vector2i(1, 2),
};

const Bitmap1bpp char_C = {
    Vector2i(1, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(0, 2), Vector2i(0, 3), Vector2i(1, 4), Vector2i(2, 4),
};

const Bitmap1bpp char_D = {
    Vector2i(0, 0), Vector2i(1, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(0, 2),
    Vector2i(2, 2), Vector2i(0, 3), Vector2i(2, 3), Vector2i(0, 4), Vector2i(1, 4),
};

const Bitmap1bpp char_E = {
    Vector2i(0, 0), Vector2i(1, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(0, 2), Vector2i(2, 2),
    Vector2i(0, 3), Vector2i(0, 4), Vector2i(1, 4), Vector2i(2, 4), Vector2i(1, 2),
};

const Bitmap1bpp char_F = {
    Vector2i(0, 0), Vector2i(1, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(0, 2), Vector2i(2, 2), Vector2i(0, 3), Vector2i(0, 4), Vector2i(1, 2),
};

const Bitmap1bpp char_G = {
    Vector2i(1, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(1, 2), Vector2i(0, 2),
    Vector2i(2, 2), Vector2i(0, 3), Vector2i(2, 3), Vector2i(1, 4), Vector2i(2, 4),
};

const Bitmap1bpp char_H = {
    Vector2i(0, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(0, 2), Vector2i(2, 2),
    Vector2i(0, 3), Vector2i(2, 3), Vector2i(0, 4), Vector2i(2, 4), Vector2i(1, 2),
};

const Bitmap1bpp char_I = {
    Vector2i(0, 0), Vector2i(2, 0), Vector2i(1, 0), Vector2i(1, 1), Vector2i(1, 2), Vector2i(1, 3), Vector2i(1, 4), Vector2i(0, 4), Vector2i(2, 4),
};

const Bitmap1bpp char_J = {
    Vector2i(2, 0), Vector2i(2, 1), Vector2i(2, 2), Vector2i(2, 3), Vector2i(1, 4), Vector2i(0, 4),
};

const Bitmap1bpp char_K = {
    Vector2i(0, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(0, 2),
    Vector2i(0, 3), Vector2i(2, 3), Vector2i(0, 4), Vector2i(2, 4), Vector2i(1, 2),
};

const Bitmap1bpp char_L = {
    Vector2i(0, 0), Vector2i(0, 1), Vector2i(0, 2), Vector2i(0, 3), Vector2i(0, 4), Vector2i(1, 4), Vector2i(2, 4),
};

const Bitmap1bpp char_M = {
    Vector2i(1, 1), Vector2i(0, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(0, 2),
    Vector2i(2, 2), Vector2i(0, 3), Vector2i(2, 3), Vector2i(0, 4), Vector2i(2, 4), Vector2i(1, 2),
};

const Bitmap1bpp char_N = {
    Vector2i(1, 1), Vector2i(1, 3), Vector2i(0, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(0, 2),
    Vector2i(2, 2), Vector2i(0, 3), Vector2i(2, 3), Vector2i(0, 4), Vector2i(2, 4), Vector2i(1, 2),
};

const Bitmap1bpp char_O = {
    Vector2i(1, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(0, 2), Vector2i(2, 2), Vector2i(0, 3), Vector2i(2, 3), Vector2i(1, 4),
};

const Bitmap1bpp char_P = {
    Vector2i(0, 0), Vector2i(1, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(0, 2), Vector2i(0, 3), Vector2i(0, 4), Vector2i(1, 2),
};

const Bitmap1bpp char_Q = {
    Vector2i(1, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(0, 2), Vector2i(2, 2),
    Vector2i(0, 3), Vector2i(2, 3), Vector2i(1, 4), Vector2i(2, 4), Vector2i(1, 3),
};

const Bitmap1bpp char_R = {
    Vector2i(0, 0), Vector2i(1, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(0, 2), Vector2i(0, 3),
    Vector2i(0, 4), Vector2i(1, 2), Vector2i(1, 3), Vector2i(2, 4), Vector2i(2, 2),
};

const Bitmap1bpp char_S = {
    Vector2i(1, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(1, 2), Vector2i(2, 3), Vector2i(0, 4), Vector2i(1, 4),
};

const Bitmap1bpp char_T = {
    Vector2i(0, 0), Vector2i(2, 0), Vector2i(1, 0), Vector2i(1, 1), Vector2i(1, 2), Vector2i(1, 3), Vector2i(1, 4),
};

const Bitmap1bpp char_U = {
    Vector2i(0, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(0, 2),
    Vector2i(2, 2), Vector2i(0, 3), Vector2i(2, 3), Vector2i(1, 4), Vector2i(2, 4),
};

const Bitmap1bpp char_V = {
    Vector2i(0, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(0, 2), Vector2i(2, 2), Vector2i(1, 3), Vector2i(1, 4),
};

const Bitmap1bpp char_W = {
    Vector2i(0, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(0, 2), Vector2i(2, 2),
    Vector2i(0, 3), Vector2i(2, 3), Vector2i(0, 4), Vector2i(2, 4), Vector2i(1, 2), Vector2i(1, 3),
};

const Bitmap1bpp char_X = {
    Vector2i(0, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(0, 3),
    Vector2i(2, 3), Vector2i(0, 4), Vector2i(2, 4), Vector2i(1, 2), Vector2i(1, 3),
};

const Bitmap1bpp char_Y = {
    Vector2i(0, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(2, 1), Vector2i(1, 2), Vector2i(1, 3), Vector2i(1, 4),
};

const Bitmap1bpp char_Z = {
    Vector2i(0, 0), Vector2i(1, 0), Vector2i(2, 0), Vector2i(0, 4), Vector2i(1, 4), Vector2i(2, 4), Vector2i(2, 1), Vector2i(1, 2), Vector2i(0, 3),
};

const Bitmap1bpp char_colon = {
    Vector2i(1, 1),
    Vector2i(1, 3),
};

const Bitmap1bpp char_dot = {
    Vector2i(1, 4),
};

const Bitmap1bpp char_comma = {
    Vector2i(1, 3),
    Vector2i(0, 4),
};

const Bitmap1bpp char_minus = {
    Vector2i(0, 2),
    Vector2i(1, 2),
    Vector2i(2, 2),
};

const Bitmap1bpp char_plus = {
    Vector2i(1, 1), Vector2i(0, 2), Vector2i(1, 2), Vector2i(2, 2), Vector2i(1, 3),
};

const Bitmap1bpp char_underscore = {
    Vector2i(0, 5),
    Vector2i(1, 5),
    Vector2i(2, 5),
};

const Bitmap1bpp char_exclamation = {
    Vector2i(1, 0),
    Vector2i(1, 1),
    Vector2i(1, 2),
    Vector2i(1, 4),
};

const Bitmap1bpp char_question = {
    Vector2i(0, 0), Vector2i(1, 0), Vector2i(2, 0), Vector2i(2, 1), Vector2i(1, 2), Vector2i(1, 4),
};

const Bitmap1bpp char_matrix = {
    Vector2i(0, 0), Vector2i(2, 0), Vector2i(1, 1), Vector2i(3, 1), Vector2i(0, 2), Vector2i(2, 2),
    Vector2i(1, 3), Vector2i(3, 3), Vector2i(0, 4), Vector2i(2, 4), Vector2i(1, 5), Vector2i(3, 5),
};
} // anonymous namespace

// ---------------------------------------------------------------------------
// Public function implementations
// ---------------------------------------------------------------------------

int getStringWidth(std::string str) { return static_cast<int>(str.length()) * fontWidth; }

Bitmap1bpp getBitmapFromChar(char character) {
    Bitmap1bpp bitmap = char_matrix;
    switch (character) {
    case 'A':
    case 'a':
        bitmap = char_A;
        break;
    case 'B':
    case 'b':
        bitmap = char_B;
        break;
    case 'C':
    case 'c':
        bitmap = char_C;
        break;
    case 'D':
    case 'd':
        bitmap = char_D;
        break;
    case 'E':
    case 'e':
        bitmap = char_E;
        break;
    case 'F':
    case 'f':
        bitmap = char_F;
        break;
    case 'G':
    case 'g':
        bitmap = char_G;
        break;
    case 'H':
    case 'h':
        bitmap = char_H;
        break;
    case 'I':
    case 'i':
        bitmap = char_I;
        break;
    case 'J':
    case 'j':
        bitmap = char_J;
        break;
    case 'K':
    case 'k':
        bitmap = char_K;
        break;
    case 'L':
    case 'l':
        bitmap = char_L;
        break;
    case 'M':
    case 'm':
        bitmap = char_M;
        break;
    case 'N':
    case 'n':
        bitmap = char_N;
        break;
    case 'O':
    case 'o':
        bitmap = char_O;
        break;
    case 'P':
    case 'p':
        bitmap = char_P;
        break;
    case 'Q':
    case 'q':
        bitmap = char_Q;
        break;
    case 'R':
    case 'r':
        bitmap = char_R;
        break;
    case 'S':
    case 's':
        bitmap = char_S;
        break;
    case 'T':
    case 't':
        bitmap = char_T;
        break;
    case 'U':
    case 'u':
        bitmap = char_U;
        break;
    case 'V':
    case 'v':
        bitmap = char_V;
        break;
    case 'W':
    case 'w':
        bitmap = char_W;
        break;
    case 'X':
    case 'x':
        bitmap = char_X;
        break;
    case 'Y':
    case 'y':
        bitmap = char_Y;
        break;
    case 'Z':
    case 'z':
        bitmap = char_Z;
        break;
    case '0':
        bitmap = char_0;
        break;
    case '1':
        bitmap = char_1;
        break;
    case '2':
        bitmap = char_2;
        break;
    case '3':
        bitmap = char_3;
        break;
    case '4':
        bitmap = char_4;
        break;
    case '5':
        bitmap = char_5;
        break;
    case '6':
        bitmap = char_6;
        break;
    case '7':
        bitmap = char_7;
        break;
    case '8':
        bitmap = char_8;
        break;
    case '9':
        bitmap = char_9;
        break;
    case ':':
        bitmap = char_colon;
        break;
    case '.':
        bitmap = char_dot;
        break;
    case ',':
        bitmap = char_comma;
        break;
    case '-':
        bitmap = char_minus;
        break;
    case '+':
        bitmap = char_plus;
        break;
    case '_':
        bitmap = char_underscore;
        break;
    case '!':
        bitmap = char_exclamation;
        break;
    case '?':
        bitmap = char_question;
        break;
    case ' ':
        bitmap.clear();
        break;
    }
    return bitmap;
}

} // namespace CharacterBitmaps
