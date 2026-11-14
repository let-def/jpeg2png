#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <tiffio.h>

#include "tiff.h"
#include "utils.h"

// clamp to sample range
static float clamp(float x) {
        return CLAMP(x, 0., 255.);
}

// write CMYK image to TIFF file, 0 means no ink
void write_tiff(FILE *out, unsigned bits, struct jpeg *jpeg) {
        ASSUME(bits == 8 || bits == 16);
        ASSUME(jpeg->c == 4);
        unsigned w = jpeg->w;
        unsigned h = jpeg->h;
        TIFF *tif = TIFFFdOpen(fileno(out), "output", "w");
        if(!tif) { die("could not initialize TIFF write struct"); }
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, w);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, h);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bits);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 4);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_SEPARATED);
        TIFFSetField(tif, TIFFTAG_INKSET, INKSET_CMYK);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        if(TIFFIsCODECConfigured(COMPRESSION_ADOBE_DEFLATE)) {
                TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE);
                TIFFSetField(tif, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
        }
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, 0));

        size_t depth = bits / 8;
        uint8_t *line = malloc(w * 4 * depth);
        if(!line) { die("could not allocate image line"); }
        float bitfactor = (1 << bits) / 256.;

        struct coef *c = jpeg->coefs;
        for(unsigned i = 0; i < h; i++) {
                for(unsigned j = 0; j < w; j++) {
                        float v[4];
                        for(unsigned k = 0; k < 4; k++) {
                                v[k] = *p(c[k].fdata, j, i, c[k].w, c[k].h);
                        }
                        if(jpeg->ycck) {
                                // YCC -> RGB, then as libjpeg does RGB -> CMY
                                float r = v[0] + 1.402 * v[2];
                                float g = v[0] - 0.34414 * v[1] - 0.71414 * v[2];
                                float b = v[0] + 1.772 * v[1];
                                v[0] = 255. - clamp(r);
                                v[1] = 255. - clamp(g);
                                v[2] = 255. - clamp(b);
                        }
                        for(unsigned k = 0; k < 4; k++) {
                                float ink = clamp(v[k]);
                                if(jpeg->inverted) { ink = 255. - ink; }
                                unsigned s = ink * bitfactor;
                                uint8_t *here = &line[((size_t)j * 4 + k) * depth];
                                if(bits == 8) {
                                        here[0] = s & 0xFF;
                                } else {
                                        // libtiff expects native endianness
                                        *(uint16_t *)(void *)here = s;
                                }
                        }
                }
                if(TIFFWriteScanline(tif, line, i, 0) < 0) { die("libtiff error: could not write scanline"); }
        }
        free(line);
        TIFFClose(tif);
}
