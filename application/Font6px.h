#ifndef __FONT6PX_H__
#define __FONT6PX_H__

#include <vector>
#include <string>
#include <Eigen/Dense>
#include <Eigen/StdVector>
using namespace Eigen;

typedef std::vector<Vector2i> Bitmap1bpp;

namespace CharacterBitmaps {
static const int fontWidth = 4;
static const int fontHeight = 6;
enum FontAlignment { left = 0, centered = -1, right = -2 };

int getStringWidth(std::string str);
Bitmap1bpp getBitmapFromChar(char character);

} // namespace CharacterBitmaps

#endif
