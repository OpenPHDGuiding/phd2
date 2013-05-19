Welcome to the PHD2 Alpha 4 test release

The major change from Alpha 3 is an improved user interface with
resizable, docking windows, improved AO support, repair of some
broken socket server functions, an AO graph window and a PHD target window.

PHD2 is based on the original PHD1 code, but much of the code has been reorganized or rewritten.  Like most things, software is easiest to work on if its organization is decided in advance, but when a program evolves as much as PHD has, it is almost inevitable that it will eventually need to be reorganized. The work on PHD2 is the result of an attempt to add Adaptive Optics support to PHD, and the things that needed to be changed to have two mounts active at the same time kept bumping up against limitations in the original code (which was written before AO devices existed), and at some point it became apparent that it would be easier to reorganize the code and then AO support than to simply add it.

PHD1 was about 20,000 lines of code, with approximately 5,000 lines for telescope support, 10,000 lines for camera support and 5,000 lines for the actual guiding routines.  The 15,000 lines for supporting telescopes and cameras is basically unchanged in PHD2, while the 5,00 lines of guiding coding is now more than 10,000 lines of guiding code.

A few users are reporting success with long guiding sessions, but you should not expect that PHD2 is ready for anything besides basic testing.

Because of the large number of devices that PHD supports, it is not possible for the PHD developers to own or test all of them. We would like you to install the software, connect to your devices and calibrate your mount and report back to us the if that worked or not. If it works, let it guide for a few minutes, but please keep an eye on it (it has been known to go in the wrong direction).

After you have tried this experiment, or if you have any other issues, 
please report success/failure to the google group.
