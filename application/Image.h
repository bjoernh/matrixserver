#ifndef IMAGE_H
#define IMAGE_H
#include <vector>
#include <Color.h>

#ifndef _WIN32
namespace Imlib2{
#include <Imlib2.h>
}
#endif

class Image {
public:
    Image();
    bool loadImage(std::string filepath);
    Color at(int col, int row);

    int getWidth();
    int getHeight();
private:
#ifndef _WIN32
    Imlib2::Imlib_Image image;
#endif
    std::vector<Color> imageData;
    unsigned int width;
    unsigned int height;
};


#endif //IMAGE_H
