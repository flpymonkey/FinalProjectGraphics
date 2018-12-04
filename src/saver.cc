#include "saver.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include "stb_image_write.h"

int saveJPG(char const *filename, int w, int h, int comp, const void *data, int quality) {
    stbi_flip_vertically_on_write(1);
    return stbi_write_jpg(filename, w, h, comp, data, quality);
}