#ifndef JPEG2PNG_TIFF_H
#define JPEG2PNG_TIFF_H

#include <stdio.h>
#include "jpeg.h"

// write a 4 component jpeg as CMYK tiff, bits is 8 or 16
void write_tiff(FILE *out, unsigned bits, struct jpeg *jpeg);

#endif
