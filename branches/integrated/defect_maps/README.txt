This branch represents the main development branch of PHD2 Guiding, maintained by Craig
Stark, the original author of PHD Guiding, Bret McKee, the authour of the majority of
the PHD2 changes, and a growing set of developers.

Rather than being geared around the Linux builds (found in the branchs/openphd1), 
this code has the OS X and Windows versions that have considerably more hardware 
supported (but aren't setup to support INDI and V4L).

As Craig uses XCode and VC++, you'll find projects setup for those in here.  There are
detailed instructions for building with MSVS in README-building-phd2-for-windows.txt.

Hopefully a similar file for OS X will be created, but at present you are on your own.

One other bit of note is that some hardware makers don't want me to expose their APIs. 
So, you'll find that some bits are still supported on the main releases of PHD that aren't
here.  Sorry, but there's nothing I can do about that on my end.  The vast majority of
bits are in here though.
