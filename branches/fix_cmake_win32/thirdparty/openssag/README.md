OpenSSAG
============
This is a C++ library for controlling the [Orion StarShoot Autoguider](http://www.telescope.com/Astrophotography/Astrophotography-Cameras/Orion-StarShoot-AutoGuider/pc/-1/c/4/sc/58/p/52064.uts) for use on Linux and Mac OS X.

The Orion StarShoot Autoguider is a popular choice for many amature astrophotographers but unfortunately does not include support for Mac OS X and Linux operating systems. The goal of this project is to do just that. The library currently supports setting gain/exposure, capturing, and sending guide commands through the on-board ST4 port. Currently, the only supported resolution to capture at is SXGA (1280x1024).

Compiling From Source
---------------------

Prerequisites: [libusb-0.1](http://www.libusb.org)

**Linux**

```
$ ./autogen.sh
$ ./configure
$ make
$ make install
```

**Mac OS X**  
Requires Developer Tools/Xcode. You may need to specifiy the location of libusb, like so:

```
$ ./autogen.sh
$ ./configure LIBUSB_CFLAGS="<libusb cflags>" LIBUSB_LIBS="<libusb libs>"
$ make
$ make install
```

Or, just install [Homebrew](http://mxcl.github.com/homebrew/) and run:

```
$ brew install autoconf automake libtool libusb-compat
$ ./autogen.sh
$ ./configure
$ make
$ make install
```

Usage
-----
The following shows a simple example of how to use the library in a C++ application.  

```
#include <stdio.h>

#include "openssag.h"

int main()
{
    OpenSSAG::SSAG *camera = new OpenSSAG::SSAG();
    
    if (camera->Connect()) {
        struct raw_image *image = camera->Expose(1000);
        FILE *fp = fopen("image", "w");
        fwrite(image->data, 1, image->width * image->height, fp);
        fclose(fp);
        camera->Disconnect();
    }
    else {
        printf("Could not find StarShoot Autoguider\n");
    }
}
```

Compile on Linux/Mac OS X

```
$ g++ main.cpp -lusb -lopenssag
$ ./a.out
$ convert -size 1280x1024 -depth 8 gray:image image.jpg
```

Technical Information
---------------------

The Orion StarShoot Autoguider uses an MT9M001 CMOS sensor from Aptina Imaging (Micron) and is controlled through a Cypress FX2 High Speed USB Controller.

**Firmware**  

The Cypress FX2 does not contain any flash program space where firmware can be loaded, but depends on the firmware being loaded into RAM over USB when the device is connected to the host operating system. The Windows driver automatically handles this when the device is connected via ssagload.sys. Since Mac and Linux don't have functionality similar to this, the firmware is loaded when SSAG::Connect() is called.

The firmware includes two sections; a second stage loader that sets up the environment in the FX2, and the real firmware that controls the camera. The firmware hex files can be found in the `firmware` directory. The firmware was extracted from the device by sniffing USB traffic. For more information on how the FX2 handles firmware, see the [FX2 datasheet](http://www.keil.com/dd/docs/datashts/cypress/cy7c68xxx_ds.pdf).

**Camera**  

There are three main functions for capturing images from the StarShoot Autoguider; configuring registers, exposing, and reading the data from the buffer. Register configuration and exposure control is handled via control trasnfers. Data is read back from the device over bulk endpoint 2.

The registers that can be directly written to are:

0x2B Gain (Even row, even column)  
0x2C Gain (Odd row, even column)  
0x2D Gain (Even row, odd column)  
0x2E Gain (Odd row, odd column)  
0x01 Row Start  
0x02 Column Start  
0x03 Row Size (Window Height)  
0x04 Column Size (Window Width)  
0x09 Shutter Width  

For more information about these registers, see the [MT9M001 datasheet](http://download.micron.com/pdf/datasheets/imaging/mt9m001_1300_mono.pdf)
