#include "Image.h"
#include <iostream>

Image::Image() {
    width = 0;
    height = 0;
}

bool Image::loadImage(std::string filepath) {
#ifndef _WIN32
    image = Imlib2::imlib_load_image(filepath.data());
    if(image){
        Imlib2::imlib_context_set_image(image);
        Imlib2::imlib_image_set_changes_on_disk();
        width = Imlib2::imlib_image_get_width();
        height = Imlib2::imlib_image_get_height();
        imageData.resize(width * height, Color::black());

        for (unsigned int cols = 0; cols < width; cols++) {
            for (unsigned int rows = 0; rows < height; rows++) {
                Imlib2::Imlib_Color tempColor;
                Imlib2::imlib_image_query_pixel(cols, rows, &tempColor);
                imageData[rows + cols * height] = Color(tempColor.red,tempColor.green,tempColor.blue);
            }
        }
    }else{
        return false;
    }
    return true;
#else
    (void)filepath;
    return false;
#endif
}

Color Image::at(int col, int row) {
    int idx = row + col * height;
    if (idx < (int)imageData.size())
        return imageData[idx];
    else if (!imageData.empty())
        return imageData[0];
    else
        return Color::black();
}

int Image::getHeight() {
    return height;
}

int Image::getWidth() {
    return width;
}
