#include "image.h"

int image_load(unsigned char *src, size_t size, Image *img) {
  int width, height, channels;
  stbi_uc *pixels = stbi_load_from_memory(
    src,
    (int)size,
    &width,
    &height,
    &channels,
    4
  );

  if (pixels == NULL)
    return 0;

  img->data = pixels;

  img->cols = width;
  img->rows = height;
  img->channels = 4;

  return 1;
}

static inline void set_pixel(Image *img, size_t x, size_t y, const unsigned char *rgba) {
  size_t i = (y * img->cols + x) * img->channels;
  for (size_t j = 0; j < img->channels; ++j) 
    img->data[i + j] = rgba[j];
  
}

void generate_default_cover(Image *img) {
  img->cols = 15;
  img->rows = 15;
  img->alpha = 255;
  img->channels = 4;
  size_t size = img->cols * img->rows * img->channels;
  void *data = malloc(size);
  if (data == NULL)
    return;
  img->data = data;
  for (size_t y = 0; y < img->rows; ++y) {
    for (size_t x = 0; x < img->cols; ++x) {
        set_pixel(img, x, y, (unsigned char[]){255, 255, 255, img->alpha});
    }
  }
}
