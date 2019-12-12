; CodeVid VidCapture installer script
;--------------------------------
Name       		"CodeVis VidCapture 0.30"
OutFile    		"Build\VidCapture_Setup.exe"
InstallDir 		$PROGRAMFILES\VidCapture
DirText    		"Choose a directory to install to"
ComponentText 		"This will install CodeVis VidCapture 0.30 on your computer."
InstallColors		00ff00 000000
InstallDirRegKey        HKLM SOFTWARE\CVVidCapture "Install_Dir"
ShowInstDetails         show
ShowUninstDetails       show
LicenseData             "Project\License_Install.txt"
LicenseText             "Please read the following license agreement before installing."
Icon                    "Project\install.ico"

InstProgressFlags smooth
AutoCloseWindow false

AllowRootDirInstall false
;--------------------------------
Section "VidCapture"
  SectionIn RO

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Put files there
  File "License.txt"
  File "VidCapture_Install.nsi"

  SetOutPath $INSTDIR\Project
  File "Project\docheader.html"
  File "Project\docstyle.css"
  File "Project\install.ico"
  File "Project\uninstall.ico"
  File "Project\VidCapGuiTest.ico"
  File "Project\Doxyfile.cfg"
  File "Project\VidCapDll.dsp"
  File "Project\VidCapTest.dsp"
  File "Project\VidCapture_VC6.dsw"
  File "Project\VidCapDllTest.dsp"
  File "Project\VidCapGuiTest.dsp"
  File "Project\License_Install.txt"
  File "Project\docfooter.html"
  File "Project\VidCapLib.dsp"

  SetOutPath $INSTDIR\Source\CVCommon
  File "Source\CVCommon\CVImageStructs.h"
  File "Source\CVCommon\CVRes.h"
  File "Source\CVCommon\CVResDll.h"
  File "Source\CVCommon\CVResFile.h"
  File "Source\CVCommon\CVResImage.h"
  File "Source\CVCommon\CVResVidCap.h"

  SetOutPath $INSTDIR\Source\Example
  File "Source\Example\Example.cpp"
  File "Source\Example\resource.h"
  File "Source\Example\StdAfx.cpp"
  File "Source\Example\StdAfx.h"
  File "Source\Example\VidCapDllTest.c"
  File "Source\Example\VidCapGuiTest.cpp"
  File "Source\Example\VidCapGuiTest.h"
  File "Source\Example\VidCapGuiTest.rc"
  File "Source\Example\VidCapGuiTestDlg.cpp"
  File "Source\Example\VidCapGuiTestDlg.h"

  SetOutPath $INSTDIR\Source\VidCapDll
  File "Source\VidCapDll\VidCapDll.def"
  File "Source\VidCapDll\VidCapDll.rc"
  File "Source\VidCapDll\resource.h"
  File "Source\VidCapDll\VidCapDll.cpp"
  File "Source\VidCapDll\VidCapDll.h"

  SetOutPath $INSTDIR\Source\VidCapture
  File "Source\VidCapture\CVDShowUtil.cpp"
  File "Source\VidCapture\CVDShowUtil.h"
  File "Source\VidCapture\CVFile.cpp"
  File "Source\VidCapture\CVFile.h"
  File "Source\VidCapture\CVImage.cpp"
  File "Source\VidCapture\CVImage.h"
  File "Source\VidCapture\CVImageGrey.cpp"
  File "Source\VidCapture\CVImageGrey.h"
  File "Source\VidCapture\CVImageRGB24.cpp"
  File "Source\VidCapture\CVImageRGB24.h"
  File "Source\VidCapture\CVImageRGBFloat.cpp"
  File "Source\VidCapture\CVImageRGBFloat.h"
  File "Source\VidCapture\CVPlatform.h"
  File "Source\VidCapture\CVPlatformWin32.cpp"
  File "Source\VidCapture\CVTrace.h"
  File "Source\VidCapture\CVTraceWin32.cpp"
  File "Source\VidCapture\CVUtil.h"
  File "Source\VidCapture\CVVidCapture.cpp"
  File "Source\VidCapture\CVVidCapture.h"
  File "Source\VidCapture\CVVidCaptureDSWin32.cpp"
  File "Source\VidCapture\CVVidCaptureDSWin32.h"
  File "Source\VidCapture\VidCapture.h"
  File "Source\VidCapture\VidCapture_Docs.h"

  SetOutPath $INSTDIR\Build\VidCapLib\Lib
  File "Build\VidCapLib\Lib\VidCapLib.lib"
  File "Build\VidCapLib\Lib\VidCapLib_db.lib"

  SetOutPath $INSTDIR\Build\VidCapLib\Inc
  File "Build\VidCapLib\Inc\CVRes.h"
  File "Build\VidCapLib\Inc\CVResDll.h"
  File "Build\VidCapLib\Inc\CVResFile.h"
  File "Build\VidCapLib\Inc\CVResImage.h"
  File "Build\VidCapLib\Inc\CVResVidCap.h"
  File "Build\VidCapLib\Inc\CVImageStructs.h"
  File "Build\VidCapLib\Inc\CVPlatform.h"
  File "Build\VidCapLib\Inc\CVVidCapture.h"
  File "Build\VidCapLib\Inc\VidCapture.h"
  File "Build\VidCapLib\Inc\CVImage.h"
  File "Build\VidCapLib\Inc\CVUtil.h"
  File "Build\VidCapLib\Inc\CVTrace.h"

  SetOutPath $INSTDIR\Build\VidCapDll\Bin
  File "Build\VidCapDll\Bin\VidCapDll.dll"
  File "Build\VidCapDll\Bin\VidCapDll_db.dll"

  SetOutPath $INSTDIR\Build\VidCapDll\Inc
  File "Build\VidCapDll\Inc\VidCapDll.h"
  File "Build\VidCapDll\Inc\CVRes.h"
  File "Build\VidCapDll\Inc\CVResDll.h"
  File "Build\VidCapDll\Inc\CVResFile.h"
  File "Build\VidCapDll\Inc\CVResImage.h"
  File "Build\VidCapDll\Inc\CVResVidCap.h"
  File "Build\VidCapDll\Inc\CVImageStructs.h"

  SetOutPath $INSTDIR\Build\VidCapDll\Lib
  File "Build\VidCapDll\Lib\VidCapDll_db.lib"
  File "Build\VidCapDll\Lib\VidCapDll.lib"

  SetOutPath $INSTDIR\Build\VidCapTest
  File "Build\VidCapTest\VidCapDllTest.exe"
  File "Build\VidCapTest\VidCapDll.dll"
  File "Build\VidCapTest\VidCaptureTest.exe"
  File "Build\VidCapTest\VidCapGuiTest.exe"

  SetOutPath $INSTDIR\Build\Docs\html
   File "Build\Docs\html\annotated.html"
   File "Build\Docs\html\classCVFile-members.html"
   File "Build\Docs\html\classCVFile.html"
   File "Build\Docs\html\classCVidCapGuiTestApp-members.html"
   File "Build\Docs\html\classCVidCapGuiTestApp.html"
   File "Build\Docs\html\classCVidCapGuiTestDlg-members.html"
   File "Build\Docs\html\classCVidCapGuiTestDlg.html"
   File "Build\Docs\html\classCVImage-members.html"
   File "Build\Docs\html\classCVImage.html"
   File "Build\Docs\html\classCVImage.png"
   File "Build\Docs\html\classCVImageGrey-members.html"
   File "Build\Docs\html\classCVImageGrey.html"
   File "Build\Docs\html\classCVImageGrey.png"
   File "Build\Docs\html\classCVImageRGB24-members.html"
   File "Build\Docs\html\classCVImageRGB24.html"
   File "Build\Docs\html\classCVImageRGB24.png"
   File "Build\Docs\html\classCVImageRGBFloat-members.html"
   File "Build\Docs\html\classCVImageRGBFloat.html"
   File "Build\Docs\html\classCVImageRGBFloat.png"
   File "Build\Docs\html\classCVPlatform-members.html"
   File "Build\Docs\html\classCVPlatform.html"
   File "Build\Docs\html\classCVVidCapture-members.html"
   File "Build\Docs\html\classCVVidCapture.html"
   File "Build\Docs\html\classCVVidCapture.png"
   File "Build\Docs\html\classCVVidCaptureDSWin32-members.html"
   File "Build\Docs\html\classCVVidCaptureDSWin32.html"
   File "Build\Docs\html\classCVVidCaptureDSWin32.png"
   File "Build\Docs\html\classes.html"
   File "Build\Docs\html\CVDShowUtil_8cpp-source.html"
   File "Build\Docs\html\CVDShowUtil_8cpp.html"
   File "Build\Docs\html\CVDShowUtil_8h-source.html"
   File "Build\Docs\html\CVDShowUtil_8h.html"
   File "Build\Docs\html\CVFile_8cpp-source.html"
   File "Build\Docs\html\CVFile_8cpp.html"
   File "Build\Docs\html\CVFile_8h-source.html"
   File "Build\Docs\html\CVFile_8h.html"
   File "Build\Docs\html\CVImageGrey_8cpp-source.html"
   File "Build\Docs\html\CVImageGrey_8cpp.html"
   File "Build\Docs\html\CVImageGrey_8h-source.html"
   File "Build\Docs\html\CVImageGrey_8h.html"
   File "Build\Docs\html\CVImageRGB24_8cpp-source.html"
   File "Build\Docs\html\CVImageRGB24_8cpp.html"
   File "Build\Docs\html\CVImageRGB24_8h-source.html"
   File "Build\Docs\html\CVImageRGB24_8h.html"
   File "Build\Docs\html\CVImageRGBFloat_8cpp-source.html"
   File "Build\Docs\html\CVImageRGBFloat_8cpp.html"
   File "Build\Docs\html\CVImageRGBFloat_8h-source.html"
   File "Build\Docs\html\CVImageRGBFloat_8h.html"
   File "Build\Docs\html\CVImageStructs_8h-source.html"
   File "Build\Docs\html\CVImageStructs_8h.html"
   File "Build\Docs\html\CVImage_8cpp-source.html"
   File "Build\Docs\html\CVImage_8cpp.html"
   File "Build\Docs\html\CVImage_8h-source.html"
   File "Build\Docs\html\CVImage_8h.html"
   File "Build\Docs\html\CVPlatformWin32_8cpp-source.html"
   File "Build\Docs\html\CVPlatformWin32_8cpp.html"
   File "Build\Docs\html\CVPlatform_8h-source.html"
   File "Build\Docs\html\CVPlatform_8h.html"
   File "Build\Docs\html\CVResDll_8h-source.html"
   File "Build\Docs\html\CVResDll_8h.html"
   File "Build\Docs\html\CVResFile_8h-source.html"
   File "Build\Docs\html\CVResFile_8h.html"
   File "Build\Docs\html\CVResImage_8h-source.html"
   File "Build\Docs\html\CVResImage_8h.html"
   File "Build\Docs\html\CVResVidCap_8h-source.html"
   File "Build\Docs\html\CVResVidCap_8h.html"
   File "Build\Docs\html\CVRes_8h-source.html"
   File "Build\Docs\html\CVRes_8h.html"
   File "Build\Docs\html\CVTraceWin32_8cpp-source.html"
   File "Build\Docs\html\CVTraceWin32_8cpp.html"
   File "Build\Docs\html\CVTrace_8h-source.html"
   File "Build\Docs\html\CVTrace_8h.html"
   File "Build\Docs\html\CVUtil_8h-source.html"
   File "Build\Docs\html\CVUtil_8h.html"
   File "Build\Docs\html\CVVidCaptureDSWin32_8cpp-source.html"
   File "Build\Docs\html\CVVidCaptureDSWin32_8cpp.html"
   File "Build\Docs\html\CVVidCaptureDSWin32_8h-source.html"
   File "Build\Docs\html\CVVidCaptureDSWin32_8h.html"
   File "Build\Docs\html\CVVidCapture_8cpp-source.html"
   File "Build\Docs\html\CVVidCapture_8cpp.html"
   File "Build\Docs\html\CVVidCapture_8h-source.html"
   File "Build\Docs\html\CVVidCapture_8h.html"
   File "Build\Docs\html\docstyle.css"
   File "Build\Docs\html\doxygen.png"
   File "Build\Docs\html\Example_2resource_8h-source.html"
   File "Build\Docs\html\Example_2resource_8h.html"
   File "Build\Docs\html\Example_8cpp-source.html"
   File "Build\Docs\html\Example_8cpp.html"
   File "Build\Docs\html\files.html"
   File "Build\Docs\html\functions.html"
   File "Build\Docs\html\functions_enum.html"
   File "Build\Docs\html\functions_eval.html"
   File "Build\Docs\html\functions_func.html"
   File "Build\Docs\html\functions_type.html"
   File "Build\Docs\html\functions_vars.html"
   File "Build\Docs\html\globals.html"
   File "Build\Docs\html\globals_defs.html"
   File "Build\Docs\html\globals_enum.html"
   File "Build\Docs\html\globals_eval.html"
   File "Build\Docs\html\globals_func.html"
   File "Build\Docs\html\globals_type.html"
   File "Build\Docs\html\globals_vars.html"
   File "Build\Docs\html\hierarchy.html"
   File "Build\Docs\html\index.html"
   File "Build\Docs\html\StdAfx_8cpp-source.html"
   File "Build\Docs\html\StdAfx_8cpp.html"
   File "Build\Docs\html\StdAfx_8h-source.html"
   File "Build\Docs\html\StdAfx_8h.html"
   File "Build\Docs\html\structCVFINDDEVINFO-members.html"
   File "Build\Docs\html\structCVFINDDEVINFO.html"
   File "Build\Docs\html\structCVIMAGESTRUCT-members.html"
   File "Build\Docs\html\structCVIMAGESTRUCT.html"
   File "Build\Docs\html\structCVVidCapture_1_1VIDCAP__DEVICE-members.html"
   File "Build\Docs\html\structCVVidCapture_1_1VIDCAP__DEVICE.html"
   File "Build\Docs\html\structCVVidCapture_1_1VIDCAP__MODE-members.html"
   File "Build\Docs\html\structCVVidCapture_1_1VIDCAP__MODE.html"
   File "Build\Docs\html\structCVVidCapture_1_1VIDCAP__PROCAMP__PROPS-members.html"
   File "Build\Docs\html\structCVVidCapture_1_1VIDCAP__PROCAMP__PROPS.html"
   File "Build\Docs\html\structDLLCAPTURESTRUCT-members.html"
   File "Build\Docs\html\structDLLCAPTURESTRUCT.html"
   File "Build\Docs\html\structVIDCAP__FORMAT__CONV-members.html"
   File "Build\Docs\html\structVIDCAP__FORMAT__CONV.html"
   File "Build\Docs\html\VidCapDllTest_8c-source.html"
   File "Build\Docs\html\VidCapDllTest_8c.html"
   File "Build\Docs\html\VidCapDll_2resource_8h-source.html"
   File "Build\Docs\html\VidCapDll_2resource_8h.html"
   File "Build\Docs\html\VidCapDll_8cpp-source.html"
   File "Build\Docs\html\VidCapDll_8cpp.html"
   File "Build\Docs\html\VidCapDll_8h-source.html"
   File "Build\Docs\html\VidCapDll_8h.html"
   File "Build\Docs\html\VidCapGuiTestDlg_8cpp-source.html"
   File "Build\Docs\html\VidCapGuiTestDlg_8cpp.html"
   File "Build\Docs\html\VidCapGuiTestDlg_8h-source.html"
   File "Build\Docs\html\VidCapGuiTestDlg_8h.html"
   File "Build\Docs\html\VidCapGuiTest_8cpp-source.html"
   File "Build\Docs\html\VidCapGuiTest_8cpp.html"
   File "Build\Docs\html\VidCapGuiTest_8h-source.html"
   File "Build\Docs\html\VidCapGuiTest_8h.html"
   File "Build\Docs\html\VidCapture_8h-source.html"
   File "Build\Docs\html\VidCapture_8h.html"
   File "Build\Docs\html\VidCapture__Docs_8h-source.html"
   File "Build\Docs\html\VidCapture__Docs_8h.html"

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\CVVidCapture "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CVVidCapture" "DisplayName" "CodeVis VidCapture (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CVVidCapture" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CVVidCapture" "InstallLocation" '"$INSTDIR"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CVVidCapture" "DisplayIcon" '"$INSTDIR\Project\install.ico"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CVVidCapture" "NoRepair" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CVVidCapture" "NoModify" 1
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CVVidCapture" "URLUpdateInfo" "http://www.codevis.com"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CVVidCapture" "HelpLink" "http://www.codevis.com"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CVVidCapture" "DisplayVersion" "0.30"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CVVidCapture" "Publisher" "CodeVis"
  
  WriteUninstaller "uninstall.exe"
  
SectionEnd ; end the section
;--------------------------------
; optional section (can be disabled by the user)
Section "Start Menu Shortcuts"
  CreateDirectory "$SMPROGRAMS\CodeVis VidCapture"
  CreateShortCut "$SMPROGRAMS\CodeVis VidCapture\CodeVis VidCapture Docs.lnk" "$INSTDIR\Build\Docs\html\index.html" "" "$INSTDIR\Build\Docs\html\index.html" 0
  CreateShortCut "$SMPROGRAMS\CodeVis VidCapture\VidCapture Test App.lnk" "$INSTDIR\Build\VidCapTest\VidCapGuiTest.exe" "" "$INSTDIR\Build\VidCapTest\VidCapGuiTest.exe" 0
  CreateShortCut "$SMPROGRAMS\CodeVis VidCapture\VidCapture Projects.lnk" "$INSTDIR\Project\" "" "$INSTDIR\Project\" 0
  CreateShortCut "$SMPROGRAMS\CodeVis VidCapture\VidCapture Source.lnk" "$INSTDIR\Source\" "" "$INSTDIR\Source\" 0
  CreateShortCut "$SMPROGRAMS\CodeVis VidCapture\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd
;--------------------------------

; Uninstaller

UninstallText "This will uninstall CodeVis VidCapture. Hit next to continue."
UninstallIcon "Project\uninstall.ico"
; Uninstall section

Section "Uninstall"
  
  ; remove registry keys
  DeleteRegKey HKLM "SOFTWARE\CVVidCapture"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CVVidCapture"

  ; remove files and uninstaller
  Delete "$INSTDIR\*.*"

  ; remove shortcuts, if any
  Delete "$SMPROGRAMS\CodeVis VidCapture\*.*"

  ; remove directories used
  RMDir /r "$SMPROGRAMS\CodeVis VidCapture"
  RMDir /r "$INSTDIR"

SectionEnd