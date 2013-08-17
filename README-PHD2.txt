Welcome to the PHD2 Beta Release 1 (v2.1.0).

We have decided to ship this version to Beta 1 (instead of Alpha 6) because PHD2 is
now "feature complete". That means we beleive all the functionality needed to ship
is implemented.

The developers and a number of Alpha testers have been using PHD2 for all night guiding
sessions, so it is getting to be "ready for prim time". That does not mean it is bug free 
or totally stable (and it never will be), but it is getting to be quite usable.:w

Please try to use use, and send feedback on issues that you find or with enhancements that
you think would improve the program.

We use the Google Code Issue tracker to keep track of these items. Please submit
any problems you find at:

https://code.google.com/p/open-phd-guiding/issues/list


The changes from Alpha 5 are:
    Fix misleading tooltip text (Fixes Issue 116)
    fix failing assert in GuideCamera::InitCapture (Fixes Issue 115)
    make sure all user-visible strings in the equipment selection dialog are localizable
    add missing bitmap files
    Connect Gear dialog improvements  - change the dialog title to "Connect Equipment" (fixes Issue 104)
    remove the Done button (fixes Issue 105)
    replace the placholder text with real instructions (fixes Issue 106)
    update main window status items as equiment is connected or disconnected (fixes Issue 107)
    Connect/disconnect buttons are now toggle buttons with a graphic label (fixes Issue 108)
    Add some Tool Tips
    fix stepguider simulator to handle recent centering changes
    change some stepguider method names to (hopefully) make clearer what they are doing
    Remove obsolete scope_none
    Fix Stepguider Centering
    graph color selection fixes (Fixes issue 109)
    update RA/Dec label colors to match seleted colors on graph
    give the color selection dialog boxes meaningful titles
    persist color choices across sessions
    do toggle RA/DEC-dx/dy after color selection!
    guide log should be opened in text mode, not binary mode  so we get the right line terminators
    fix windows installer problem
    Fix debug log causing crashes in release mode
    Updated French translation.
    fix spelling error in tool tip text
    fix logic error checking for connected mount when setting manual lock position
    minor code cleanup: remove redundant null check
    A massive restructuring about how gear connects
    new Inno Setup script to create a PHD2 Windows installer that does not depend on of PHD1 (Fixes Issue 93)
    - bump version to 2.0.5d consistently display the full version number, including the sub-version string
    - update about dialog to include all contributors (based on the commit log) and all copyright notices in the source tree
    - remove duplicate Windows DLLs enable server by default (Fixes Issue 102)
    fix socket server switch statement missing break
    fix crash when socket server command MSG_LOOP is received while already looping (Fixes Issue 101)
    fix typo in installer message
    Changes to make sure socket server behavior is compatible with PHD1 (Fixes issue 57)
    Fix issues when Dec Guide Mode is other than Auto (Fixes issue 94)
    Removing "cam_KWIQGuider"
    Added directory "cam_KWIQGuider"
    Polar alignment enhancement: draw polar alignment circle when trendlines are enabled on the graph.
    Do not abort calibration if star is lost (Fixes issue 97)  just keep trying.  Also, add some more debug logging around calibration moves.
    camera simulator enhancements:
    - allow simulating meridian flip
    - camera/mount can be flipped with new socket server command (23)
    - allow varying noise level to allow simulating noisy images
    fix broken on-camera mount (Fixes issue 92)
    fix debug log output and typo in comment
    whitespace changes (fix line terminators)
    fix broken socket server commands (Fixes issue 96)
    Adding logging to move
    Provide a command line option to reset all settings
    Display polar alignment error estimate on graph when trendlines are enabled.
    fix a missing POSSIBLY_UNUSED
    Rename scope_onboard to scope_oncamera
    Fix typos in licenses pointed out during code review
    improvements to guiding status message updates (Fixes issue 77)
    - display messages for at least 1/2 second (no
    - fixes a bug where status messages could be queued faster than they could be displayed resulting in a growing backlog of status messages
    - Minor edit to cam_firewire_OSX to use new recon / dark subtraction method
    - Added new files to OS X project file
    bumping version to 2.0.5c
    Adding some more debugging to the parallel LE cam
    Adding some logging to the config class
    remove an unused variable
    Changes for Alpha 5b

109 files changed, 6397 insertions(+), 4090 deletions(-)

Thanks,

The PHD2 team
