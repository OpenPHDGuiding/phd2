This branch represents the main development branch of PHD Guiding, maintained by Craig
Stark, the original author of PHD Guiding.  Rather than being geared around the Linux
builds (found in the trunk), this code has the OS X and Windows versions that have
considerably more hardware supported (but aren't setup to support INDI and V4L).

As Craig uses XCode and VC++, you'll find projects setup for those in here.  Feel free to
use your own environment, but don't expect me to shift over.  Grab the .cpp and .h files
as you needed them and play to your heart's content.

To get this to actually build, you'll need a few dependencies (and again, no, I'm not
setting this up here to automagically download and build them).  Specifically, you'll
need: 
- CFITSIO (although Mac .a and Windows .DLL files are in here you may need to get
these yourself) 
- wxWidgets 2.9 or greater

If you're having trouble getting some bit of hardware to compile from a missing library,
feel free to remove the definition to it in cameras.h or scopes.h.

One other bit of note is that some hardware makers don't want me to expose their APIs. 
So, you'll find that some bits are still supported on the main releases of PHD that aren't
here.  Sorry, but there's nothing I can do about that on my end.  The vast majority of
bits are in here though.

