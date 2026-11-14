#ifndef JPEG2PNG_JPEG_H
#define JPEG2PNG_JPEG_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "jpeg2png.h"

struct jpeg {
        unsigned c;
        unsigned h;
        unsigned w;
        struct coef coefs[4];
        // 4 components only
        bool ycck;     // components are Y, Cb, Cr, K instead of C, M, Y, K
        bool inverted; // Adobe style: 255 means no ink
};

// number of components of a JPEG file, rewinds the file
unsigned peek_jpeg_components(FILE *in);

void read_jpeg(FILE *in, struct jpeg *jpeg);
void decode_coefficients(struct coef *coef);
#endif
