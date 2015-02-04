/*
 * File: ssag.cpp
 *
 * Copyright (c) 2011 Eric J. Holmes, Orion Telescopes & Binoculars
 *
 */

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#if HAVE_LIBMAGICKCORE
#   include <magick/MagickCore.h>
#endif

#include "openssag.h"

#if DEBUG
#   define DBG(...) fprintf(stderr, __VA_ARGS__)
#else
#   define DBG(...)
#endif

using namespace OpenSSAG;

void usage()
{
    printf("Usage: ssag [OPTION]...\n");
    printf("Capture images from an Orion StarShoot Autoguider.\n\n");
    
    printf("  -c, --capture [DURATION]             Capture an image from the camera. DURATION is the exposure time in ms.\n");
#if HAVE_LIBMAGICKCORE
    printf("  -f, --filename [FILENAME]            Specifiy the filename to save the image as. (eg. M42.png, M32.jpg)\n");
#endif
    printf("  -g, --gain [1-15]                    Set the gain to be used for the capture. Only accepts values between 1 and 15\n");
    printf("  -b, --boot                           Load the firmware onto the camera.\n");
    printf("  -p, --pulseguide [nswe]              Pulseguide in direction.\n");
    printf("  -d, --duration [DURATION]            Duration to pulseguide in milliseconds\n");
}

int main(int argc, char **argv)
{
    if (argc <= 1) {
        usage();
        return 0;
    }
    SSAG *camera = new SSAG();
    static int capture_flag, gain_flag, pulseguide_flag;
    static int duration = 1000;
    static int gain = 4;
    static int pulseguide_duration = 100;
    static int pulseguide_direction = guide_north;
#if HAVE_LIBMAGICKCORE
    static char filename[256] = "image.png";
#endif
    int c;
    opterr = 0;

    while (1) {
        static struct option long_options[] = {
            {"help",        no_argument,       0, 'h'}, /* Help */
            {"boot",        no_argument,       0, 'b'}, /* Load firmware */
            {"gain",        required_argument, &gain_flag, 'g'}, /* Capture an image from the camera */
            {"capture",     required_argument, &capture_flag, 'c'}, /* Capture an image from the camera */
#if HAVE_LIBMAGICKCORE
            {"filename",    required_argument, 0, 'f'},
#endif
            {"pulseguide",  required_argument, &pulseguide_flag, 'p'},
            {"duration",    required_argument, 0, 'd'},
            {0, 0, 0, 0}
        };
        
        int option_index = 0;
        c = getopt_long(argc, argv, "hbc:g:f:p:d:", long_options, &option_index);

        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                usage();
                goto done;
            case 'b':
                {
                    Loader *base = new Loader();
                    if (!base->Connect()) {
                        fprintf(stderr, "Device not found or the device already has firmware loaded\n");
                    } else {
                        base->LoadFirmware();
                        base->Disconnect();
                    }
                }
                goto done;
            case 'g':
                gain = atoi(optarg);
                if (gain < 1 || gain > 15) {
                    fprintf(stderr, "Ignoring invalid gain setting.\n");
                    gain_flag = 0;
                } else {
                    gain_flag = 1;
                }
                break;
            case 'c':
                duration = atoi(optarg);
                capture_flag = 1;
                break;
            case 'd':
                pulseguide_duration = atoi(optarg);
                break;
            case 'p':
                if (strcmp(optarg, "n") == 0) {
                    pulseguide_direction = guide_north;
                } else if (strcmp(optarg, "s") == 0) {
                    pulseguide_direction = guide_south;
                } else if (strcmp(optarg, "e") == 0) {
                    pulseguide_direction = guide_east;
                } else if (strcmp(optarg, "w") == 0) {
                    pulseguide_direction = guide_west;
                } else {
                    usage();
                    exit(-1);
                }
                pulseguide_flag = 1;
                break;
#if HAVE_LIBMAGICKCORE
            case 'f':
                strcpy(filename, optarg);
                break;
#endif
            default:
                break;
        }
    }

    if (capture_flag) {
        if (!camera->Connect()) {
            fprintf(stderr, "Camera not found or could not connect\n");
            goto done;
        }
        if (gain_flag) {
            camera->SetGain(gain);
        }
        struct raw_image *raw = camera->Expose(duration);
        if (raw) {
#if HAVE_LIBMAGICKCORE
            Image *image = NULL;
            ImageInfo *image_info = CloneImageInfo((ImageInfo *)NULL);
            image_info->compression = NoCompression;
            MagickCoreGenesis(NULL, MagickTrue);
            ExceptionInfo *exception = AcquireExceptionInfo();
            image = ConstituteImage(raw->width, raw->height, "I", CharPixel, raw->data, exception);
            strcpy(image->filename, filename);
            WriteImage(image_info, image);
            MagickCoreTerminus();
#else
            FILE *fd = fopen("image.8bit", "w");
            if (fd) {
                fwrite(raw->data, 1, raw->width * raw->height, fd);
                fclose(fd);
                // In terminal:
                // convert -size 1280x1024 -depth 8 gray:image image.jpg 
            }
#endif // HAVE_LIBMAGICKCORE
        }
    }

    if (pulseguide_flag) {
        if (!camera->IsConnected()) {
            if (!camera->Connect()) {
                fprintf(stderr, "Camera not found or could not connect\n");
                goto done;
            }
        }
        camera->Guide(pulseguide_direction, pulseguide_duration);
    }
done:
    if (camera)
        camera->Disconnect();
    return 0;
}
