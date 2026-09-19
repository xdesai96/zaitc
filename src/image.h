#ifndef ZAITC_IMAGE
#define ZAITC_IMAGE

#include "config.h"
#include "../thirdparty/stb_image.h"
#include <math.h>

typedef struct Image {
  bool can_load;
  size_t alpha;
  size_t channels;
  size_t rows, cols;
  double scale;
  unsigned char *data;
} Image;

int image_load(unsigned char *src, size_t size, Image *img);
void generate_default_cover(Image *img);

#endif // ZAITC_IMAGE
