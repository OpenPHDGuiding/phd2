Welcome to PHD2 Alpha 3 testing. 

PHD2 is based on the original PHD1 code, but much of the code has been reorganized.  Like most things, software is easiest to work on if its organization is decided in advance, but when a program evolves as much as PHD has, it is almost inevitable that it will eventually need to be reorganized. The work on PHD2 is the result of an attempt to add Adaptive Optics support to PHD, and the things that needed to be changed to have two mounts active at the same time kept bumping up against limitations in the original code (which was written before AO devices existed), and at some point it became apparent that it would be easier to reorganize the code and then AO support than to simply add it.

PHD1 was about 20,000 lines of code, with approximately 5,000 lines for telescope support, 10,000 lines for camera support and 5,000 lines for the actual guiding routines.  The 15,000 lines for supporting telescopes and cameras is basically unchanged in PHD2, while most of the guiding coding being either reorganized or rewritten.  

The goal of the Alpha releases is to figure out which things basically still work, and which things accidentally got broken in the reorganization. While I have guided for a couple nights with an earlier version of PHD2, you should not expect that PHD2 is ready for anything besides testing.

Because of the large number of devices that PHD supports, it is not possible for the PHD developers to own or test all of them. For Alpha 1, what we would like you to is to install the software, connect to your devices and calibrate your mount and report back to us the if that worked or not. 

After you have tried this experiment, please report success/failure to the google group.

If you have any other issues, please post them here.

