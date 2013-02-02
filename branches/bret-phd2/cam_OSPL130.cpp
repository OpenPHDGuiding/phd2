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

extern void TestGuide();

#if defined (OS_PL130)
#include "cam_OSPL130.h"
#include "cameras/OSPL130API.h"

Camera_OpticstarPL130Class::Camera_OpticstarPL130Class() {
    Connected = FALSE;
//  HaveBPMap = FALSE;
//  NBadPixels=-1;
    Name=_T("Opticstar PL-130M");
    FullSize = wxSize(1280,1024);
    HasGuiderOutput = false;
    HasGainControl = false;
    Color = false;
}



bool Camera_OpticstarPL130Class::Connect() {
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
    RawData = new unsigned char [2621440];  //
//OSPL130_SetGain(6);
    Connected = true;
    return false;
}


bool Camera_OpticstarPL130Class::Disconnect() {
    OSPL130_Finalize();
    Connected = false;
    if (RawData) delete [] RawData;
    RawData = NULL;
    return false;
}

bool Camera_OpticstarPL130Class::Capture(int duration, usImage& img, wxRect subframe, bool recon) {
//  bool retval;
    bool still_going = true;

    int mode = 3 * (int) Color;
    if (img.NPixels != (FullSize.GetWidth()*FullSize.GetHeight())) {
        if (img.Init(FullSize.GetWidth(),FullSize.GetHeight())) {
            wxMessageBox(_T("Memory allocation error during capture"),_("Error"),wxOK | wxICON_ERROR);
            Disconnect();
            return true;
        }
    }
    if (OSPL130_Capture(mode,duration)) {
        wxMessageBox(_T("Cannot start exposure"), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    if (duration > 100) {
        wxMilliSleep(duration - 100); // wait until near end of exposure, nicely
        wxTheApp->Yield();
//      if (Abort) {
//          MeadeCam->AbortImage();
//          return true;
//      }
    }
    while (still_going) {  // wait for image to finish and d/l
        wxMilliSleep(20);
        OSPL130_IsExposing(&still_going);
        wxTheApp->Yield();
    }
    // Download
//  rawptr = RawData;
    OSPL130_GetRawImage(0,0,FullSize.GetWidth(),FullSize.GetHeight(), (void *) img.ImageData);
    unsigned short *dataptr;
    dataptr = img.ImageData;
    // byte swap


    if (HaveDark && recon) Subtract(img,CurrentDarkFrame);
    if (Color)
        QuickLRecon(img);

    if (recon) {
        ;
    }


    return false;
}
#endif
