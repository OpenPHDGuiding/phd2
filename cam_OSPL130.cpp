/*
 *  cam_OSPL130.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006, 2007, 2008, 2009, 2010 Craig Stark.
 *  All rights reserved.
 *
 */
#include "phd.h"
#include "camera.h"
#include "time.h"
#include "image_math.h"
#include "wx/stopwatch.h"

#if defined (OS_PL130)
#include "cam_OSPL130.h"
#include "cameras/OSPL130API.h"

static bool DLLExists(const wxString& DLLName)
{
    wxStandardPathsBase& StdPaths = wxStandardPaths::Get();
    if (wxFileExists(StdPaths.GetExecutablePath().BeforeLast(PATHSEPCH) + PATHSEPSTR + DLLName))
        return true;
    if (wxFileExists(StdPaths.GetExecutablePath().BeforeLast(PATHSEPCH) + PATHSEPSTR + ".." + PATHSEPSTR + DLLName))
        return true;
    if (wxFileExists(wxGetOSDirectory() + PATHSEPSTR + DLLName))
        return true;
    if (wxFileExists(wxGetOSDirectory() + PATHSEPSTR + "system32" + PATHSEPSTR + DLLName))
        return true;
    return false;
}

Camera_OpticstarPL130Class::Camera_OpticstarPL130Class()
{
    Connected = false;
    Name=_T("Opticstar PL-130M");
    FullSize = wxSize(1280,1024);
    m_hasGuideOutput = false;
    HasGainControl = false;
    Color = false;
}

wxByte Camera_OpticstarPL130Class::BitsPerPixel()
{
    return 16;
}

bool Camera_OpticstarPL130Class::Connect(const wxString& camId)
{
// returns true on error
    int retval;
    if (!DLLExists("OSPL130RT.dll")) {
        wxMessageBox(_T("Cannot find OSPL130RT.dll"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    retval = OSPL130_Initialize((int) Color, false, 0, 2);
    if (retval) {
        wxMessageBox("Cannot init camera",_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
//OSPL130_SetGain(6);
    Connected = true;
    return false;
}

bool Camera_OpticstarPL130Class::Disconnect()
{
    OSPL130_Finalize();
    Connected = false;
    return false;
}

bool Camera_OpticstarPL130Class::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    bool still_going = true;

    int mode = 3 * (int) Color;
    if (img.Init(FullSize))
    {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }
    if (OSPL130_Capture(mode,duration)) {
        pFrame->Alert(_("Cannot start exposure"));
        return true;
    }
    if (duration > 100) {
        wxMilliSleep(duration - 100); // wait until near end of exposure, nicely
        wxGetApp().Yield();
//      if (Abort) {
//          MeadeCam->AbortImage();
//          return true;
//      }
    }
    while (still_going) {  // wait for image to finish and d/l
        wxMilliSleep(20);
        OSPL130_IsExposing(&still_going);
        wxGetApp().Yield();
    }
    // Download
    OSPL130_GetRawImage(0,0,FullSize.GetWidth(),FullSize.GetHeight(), (void *) img.ImageData);
    // byte swap

    if (options & CAPTURE_SUBTRACT_DARK) SubtractDark(img);
    if (Color && (options & CAPTURE_RECON))
        QuickLRecon(img);

    return false;
}

#endif
