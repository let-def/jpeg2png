#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>

#include <fftw3.h>

#include "gopt/gopt.h"

#include "jpeg2png.h"
#include "utils.h"
#include "jpeg.h"
#include "png.h"
#include "box.h"
#include "upsample.h"
#include "compute.h"
#include "logger.h"

static const float default_weight = 0.3;
static const int default_iterations = 100;

noreturn static void usage() {
        printf(
                "usage: jpeg2png in.jpg out.png [-w weight] [-i iterations] [-c csv_log]\n"
                "\n"
                "-w weight\n"
                "--second-order-weight weight\n"
                "\tweight is a floating point number between 0.0 and 1.0 for TVG weight alpha_1\n"
                "\thigher values give smoother transitions with less staircasing\n"
                "\ta value of 1.0 means equivalent weight to the first order weight\n"
                "\ta value of 0.0 means plain Total Variation, and gives a speed boost\n"
                "\tdefault value: %g\n"
                "\n"
                "-i iterations\n"
                "--iterations iterations\n"
                "\titerations is an integer for the number of optimization steps\n"
                "\thigher values give better results but take more time\n"
                "\tdefault value: %d\n"
                "\n"
                "-c csv_log\n"
                "--csv_log csv_log\n"
                "\tcsv_log is a file name for the optimization log\n"
                "\tdefault: none\n"
                , default_weight, default_iterations);
        exit(EXIT_FAILURE);
}

int main(int argc, const char **argv) {
        void *options = gopt_sort(&argc, argv, gopt_start(
                gopt_option('h', GOPT_NOARG, gopt_shorts( 'h', '?' ), gopt_longs("help")),
                gopt_option('c', GOPT_ARG, gopt_shorts('c'), gopt_longs("csv-log")),
                gopt_option('i', GOPT_ARG, gopt_shorts('i'), gopt_longs("iterations")),
                gopt_option('w', GOPT_ARG, gopt_shorts('w'), gopt_longs("second-order-weight"))));
        if(argc != 3 || gopt(options, 'h')) {
                usage();
        }
        const char *arg_string;
        float weight = default_weight;
        if(gopt_arg(options, 'w', &arg_string)) {
                if(sscanf(arg_string, "%f", &weight) != 1) {
                        die("invalid weight");
                }
        }
        int iterations = default_iterations;
        if(gopt_arg(options, 'i', &arg_string)) {
                if(sscanf(arg_string, "%d", &iterations) != 1 || iterations < 0) {
                        die("invalid number of iterations");
                }
        }

        FILE *in = fopen(argv[1], "rb");
        if(!in) { die_perror("could not open input file `%s`", argv[1]); }
        FILE *out = fopen(argv[2], "wb");
        if(!out) { die_perror("could not open output file `%s`", argv[2]); }

        FILE *csv_log = NULL;
        if(gopt_arg(options, 'c', &arg_string)) {
                csv_log = fopen(arg_string, "wb");
                if(!csv_log) { die_perror("could not open csv log `%s`", csv_log); }
        }

        struct jpeg jpeg;
        read_jpeg(in, &jpeg);
        fclose(in);

        for(int c = 0; c < 3; c++) {
                struct coef *coef = &jpeg.coefs[c];
                decode_coefficients(coef, jpeg.quant_table[c]);
        }

        for(int i = 0; i < 3; i++) {
                struct coef *coef = &jpeg.coefs[i];
                float *temp = fftwf_alloc_real(coef->h * coef->w);
                if(!temp) { die("allocation error"); }

                unbox(coef->fdata, temp, coef->w, coef->h);

                fftwf_free(coef->fdata);
                coef->fdata = temp;
        }

        START_TIMER(computing);
        struct logger log;
        logger_start(&log, csv_log);
        for(int i = 0; i < 3; i++) {
                log.channel = i;
                START_TIMER(compute_1);
                struct coef *coef = &jpeg.coefs[i];
                uint16_t *quant_table = jpeg.quant_table[i];
                compute(coef, &log, quant_table, weight, iterations);
                STOP_TIMER(compute_1);
        }
        STOP_TIMER(computing);
        if(csv_log) {
                fclose(csv_log);
        }

        struct coef *coef = &jpeg.coefs[0];
        for(int i = 0; i < coef->h * coef->w; i++) {
                coef->fdata[i] += 128.;
        }

        START_TIMER(upsampling);
        for(int i = 0; i < 3; i++) {
                upsample(&jpeg.coefs[i], jpeg.w, jpeg.h);
        }
        STOP_TIMER(upsampling);

        write_png(out, jpeg.w, jpeg.h, &jpeg.coefs[0], &jpeg.coefs[1], &jpeg.coefs[2]);
        fclose(out);

        for(int i = 0; i < 3; i++) {
                fftwf_free(jpeg.coefs[i].fdata);
                free(jpeg.coefs[i].data);
        }
}
