Welcome to the PHD2 Beta Release 3 (v2.1.3a).

For Windows, this would be Release Candidate 1 if it were not for some late breaking
changes to the LE webcam code.  For IOS, this is the first Beta to contain the new
GUI changes designed to fix all the little wxWidgets issues that were discovered. Other
than the GUI changes, the IOS version is probably at Release Candidate quality as well.

At this point we are concentrating on fixing any remaining serious defects, and are likely
to not work on enhacement requests until PHD2.2 (i.e. until we have a "Release" version of 
PHD2, which of reasons I won't go into will come out as 2.1.x).

Please try to use use, and send feedback on issues that you find or with enhancements that
you think would improve the program.

We use the Google Code Issue tracker to keep track of these items. Please submit
any problems you find at:

https://code.google.com/p/open-phd-guiding/issues/list

Thanks,

The PHD2 team

Changes from Beta 2:

    Changes for Beta 3
    Refactor cam_wdm and move LE cams to wdm 
    Fix linux newlines in cam_QHY5LII.cpp 
    Whitespace cleanup - converted Unix EOF characters. 
    Added declination adjustment to stepsize calculator; replaced most text controls with spinner controls in stepsize calculator in order to improve user-friendliness; replaced camera pixel size text control with spinner control in guider.cpp 
    Save/restore user settings for graph mode and graph units; Resolves issue 206 
    add a few more event server commands 
    update event server guide/dither to run on Windows 
    whitespace cleanup 
    add new files to Windows project file 
    event server commands to start guiding and to dither, with notification of guider settling 
    add event server command to set lock position 
    fix bug with socket server command causing a message box to pop up. socket server events now do not send button push events, but invoke common code shared with the button push handlers. 
    Flag Starfish guide cam with HasNonGuiCapture, ST4HasNonGuiMove = true. 
    fix Linux compile error 
    add option for fast recenter of star after calibration 
    fix spelling error in log message 
    add server commands to select equipment profile and connect and disconnect equipment 
    Fix bogus "no star found message" when looping starts; clean up spacing in Cam_simulator controls 
    Fix misleading messages in Brain when scope, mount, or AO is not selected. The messages say, for example, "No Camera Connected" when in fact a camera need only be selected, not necessarily Connected. Also, change the name of the tab from "Scope" to "Mount" to be more consistent with the Connect Equipment dialog. In connect equipment we say Connect Mount, but in the brain we were saying "No Scope Connected", so now call it a Mount in both places. 
    fix incorrect information in tooltip 
    whitespace change 
    Add tooltips explaining the difference between "default curve" and "custom curve" in the Camera Simulator dialog. Also, fix how some of the labels were showing "label::", now show "label:" 
    fix compile error when Bruce's SIMDEBUG conditional code is enabled 
    fix Linux compile errors 
    fix Mac compile error 
    add new source files to Mac xcode project file 
    add ability to get/set camera duration via server 
    revert recently added simulator code modifying mount state; supply alternate solution rendering simulated stars at sub-pixel offsets also, fix a couple bugs in new simulator cam dialog: - fix the clouds checkbox so clouds can be turned off - disable the reset button when capture is active - relax some of the parameter limits 
    UI enhancements in CamSimulator based on Andy's requests 
    Fixed bug in range checking of dec_drift_rate 
    Fixed bugs in new CamSimulator dialog 
    fix Linux compile errors 
    Enhancements to camera simulator; miscellaneous white-space clean-up     Simulator changes:         Better support for end-user learning and experimentation         Reasonable behavior across a range of image scales         UI rework to support image-scale-independent parameters         Improved realism for seeing deflections, including fractional pixel moves         Basic UI for PE simulation     Whitespace clean-up in several unrelated project files 
    Fix Linux compile errors and warnings. 
    - Rearranging wxBoxSize on the graph window sot that the mini brain does overlap control button. Tested on 12'' eeePC. Update Issue 185. - Rearranging of some control on the brain dialog to reduce its height under 600px. Fix issue 180. - Sticky Lock Position is now saved in PHD config. Fix issue 202. - Removing translation on strings in debug log file. 
    Fix useless message box asking to restart PHD guiding after exiting the brain dialog without changing the language. 
    fix compile error on Mac and add new files to Mac xcode project file 
    Manual calibration field in one window with french translation 
    Update french translation 
    Make some strings translatable and make strings used for logging not translatable. 
    Removing wxFloatingPointValidator which cause conflict between wxlocal and setlocale 
    add option to automatically load calibration data 
    Mac: fix bookmark key 
    Mac tweaks to fix missing/broken UI elements 
    fix typo in debug log output 
    Mac: fix missing labels on controls on graph 
    MAC: remove absolute paths from xcode project file There were several hard-coded paths for dependent libraries. Now we look for the dependencies relative to the source tree 
    fix crash when app starting on Mac 
    mini-brain guider controls got broken in r545 earlier today, fix 
    update Mac xcode project file for recently added/renamed files 
    bump guide log version number 
    Guide log changes to better support PHDLab:  - remove of redundant direction columns 
    Update french translation 
    Adding circle to the target window. Fixes issue 76. 
    Fix profile initialization with localization 
    Fix string formatting for translation 
    Initialize graph y-axis scale to match PHD1's 4-pixel scale for users starting PHD2 for the first time. This should help minimize one of the primary sources of confusion we are hearing from PHD1 users trying out PHD2. 
    fix display of guide corrections on graph. recent changes broke the display, causing all corrections to be displayed in one direction 
    fix spelling error in debug log 
    automatically flip calibration after pier flip for ASCOM mounts 
    drift align tool enhancements: display a bitmap to show which adjustment is being done; add a place to store notes about alt/az adjustment 
    fix problem displaying bookmarks when image is scaled; add ability to bookmark the lock position with a keyboard shortcut 
    send guide step info that was recently added to guiding log to event server as well 
    Minor changes in logging code to resolve issues 167, 178, 195 Whitespace repairs The default logging location now specifies a 'PHD2' sub-directory 
    fix a mem leak at app shutdown when socket server clients are still connected 
    Drift alignment tool 
    Add a check-box in the brain guider tab to explictly enable/disable star mass change detection, rather than trying to explain to users that setting the value to 100 disables star mass change detection. 
    fix incorrect calibration dec compensation after meridian flip 
    Report star error code in guide log in the same way that PHD1 did. 
    improved socket server distance reporting 
    whitespace changes 
    Content fixes to logging, make guiding & calibration setting header lines consistent, add missing calibration for stepguiders, fix typos.  These fixes will allow the upcoming 0.5 rev of PHDLab to handle PHD2 logs. 
    ignore case when sorting cameras and scopes for equipment dialog drop-down lists 
    re-add updated version 
    deleteing messages.mo temporarily, I'm about to add back a new version 
    update messages files 
    since we now allow switching directly from guiding to looping, allow this via the socket server MSG_LOOP command too. 
    Allow a star to be selected when not looping. 
    simulator: add an option to simulate clouds To simulate clouds, open the Cam Dialog, and check "clouds". Also, fix layout of contols in the simulator Cam Dialog to be in two columns. 
    Allow alternating between Looping ang Guiding without hitting Stop 
    Make status bar widths dynamic 
    Make the Mac build not use OpenCV 
    Check for pGuider before dereferencing it 
    Log version numbers 
    minor cleanups: remove redundant parent window pointer, use wxWindow::GetParent
    Check for pFrame before dereferencing 
    Fix PHD crash when opening calibration step calculator when ASCOM mount is not connected. Also, add checks in all ASCOM scope methods to prevent similar future crashes. 

    103 files changed, 11495 insertions(+), 5515 deletions(-)
