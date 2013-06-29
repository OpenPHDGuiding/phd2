Welcome to the PHD2 Alpha 5 test release.

The major changes from Alpha 4 are:
- Added tabs to the brain menu
- changed to use OpenCV for control of web cameras, and rewritten handling for LE modified webcams.
- added management of dark frames
- added the ability to adjust declination calibration for declination
- added keyboard shortcuts for all the big buttons
- added support for outputing in arc-seconds
- added an event framework
- modified AO mount bump
- enhanded manual lock position
- a bunch more things are saved between sessions, and the ability to clear them all
- added an option to always strech the displayed image
- increased the gamma range
- add an option to display trend lines on the graph (to assist with polar alignment)
- some performance improvements
    - moved more devices to background threads
    - optimized window repaints
    - sped up median noise reduction
- fixed a number of A4 bugs:
    - could not connect to AO on port > COM9
    - fixed hangs with ASCOM cameras
    - fixed problems with aborting sometimes blowing assertions
    - fixed AO max step calculations
    - disable brain button when looping/guiding
    - a problem with calibration flipping with AO attached

Please try the features out and feedback on issues that you find or with enhancements that 
you think would improve the program.

We use the Google Code Issue tracker to keep track of these items. Please submit
any problems you find at:

https://code.google.com/p/open-phd-guiding/issues/list

Thanks,

The PHD2 team
