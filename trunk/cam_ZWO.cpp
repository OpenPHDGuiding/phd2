/*
*  cam_ZWO.cpp
*  PHD Guiding
*
*  Created by Robin Glover.
*  Copyright (c) 2014 Robin Glover.
*  All rights reserved.
*
*  This source code is distributed under the following "BSD" license
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*    Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*    Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*    Neither the name of Craig Stark, Stark Labs nor the names of its
*     contributors may be used to endorse or promote products derived from
*     this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
*/
#include "phd.h"

#ifdef ZWO_ASI

#include "cam_ZWO.h"
#include "cameras/ASICamera.h"



Camera_ZWO::Camera_ZWO()
{
    Connected = false;
    m_hasGuideOutput = true;
    HasGainControl = true; // really ought to ask the open camera, but all known ZWO cameras have gain
}

Camera_ZWO::~Camera_ZWO()
{
}

bool Camera_ZWO::Connect()
{

    // Find available cameras
    wxArrayString USBNames;
    int iCameras = getNumberOfConnectedCameras();
    if (iCameras == 0)
    {
        wxMessageBox(_T("No ZWO cameras detected."), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    for (int i = 0; i < iCameras; i++)
    {
        char* name = getCameraModel(i);
        USBNames.Add(name);
    }

    int iSelected = 0;

    if (USBNames.Count() > 1)
    {
        iSelected = wxGetSingleChoiceIndex(_("Select camera"), _("Camera name"), USBNames);
        if (iSelected == -1) 
        {
            Disconnect(); return true; 
        }
    }

    if (!openCamera(iSelected))
    {
        wxMessageBox(_T("Failed to open ZWO ASI Camera."), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    if (!initCamera())
    {
        wxMessageBox(_T("Failed to initialize ZWO ASI Camera."), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    FullSize.x = getMaxWidth();
    FullSize.y = getMaxHeight();

    PixelSize = getPixelSize();

    if (HasGainControl)
    {
        GuideCameraGain = (getMax(CONTROL_GAIN) + getMin(CONTROL_GAIN)) / 2;
    }

    Connected = true;

    if (isAvailable(CONTROL_BANDWIDTHOVERLOAD))
        setValue(CONTROL_BANDWIDTHOVERLOAD, getMin(CONTROL_BANDWIDTHOVERLOAD), false);

    setStartPos(0, 0);

    setImageFormat(FullSize.x, FullSize.y, 1, IMG_Y8);
    startCapture();

    return false;
}

bool Camera_ZWO::Disconnect()
{
    stopCapture(); 
    closeCamera();

    Connected = false;
    return false;
}


bool Camera_ZWO::Capture(int duration, usImage& img, wxRect subframe, bool recon)
{
    int exposureUS = duration * 1000;

    int xsize = getMaxWidth();
    int ysize = getMaxHeight();

    if (img.NPixels != (xsize*ysize)) {
        if (img.Init(xsize, ysize)) {
            pFrame->Alert(_("Memory allocation error during capture"));
            Disconnect();
            return true;
        }
    }

    setValue(CONTROL_EXPOSURE, exposureUS, false);
    setValue(CONTROL_GAIN, GuideCameraGain, false);

    int bufSize = xsize * ysize;
    unsigned char* buffer = new unsigned char[bufSize];

    bool gotFrame = getImageData(buffer, bufSize, duration * 2 + 1000);

    if (!gotFrame)
        return true;

    unsigned char* src = buffer;
    unsigned short* dest = img.ImageData;
    for (int y = 0; y < ysize; y++)
    {
        for (int x = 0; x < xsize; x++, src++, dest++)
        {
            *dest = (unsigned short)*src;
        }
    }

    delete[] buffer;

    if (recon) SubtractDark(img);

    return false;
}

GuideDirections GetDirection(int direction)
{
    switch (direction)
    {
    default:
    case NORTH:
        return guideNorth;
    case EAST:
        return guideEast;
    case WEST:
        return guideWest;
    case SOUTH:
        return guideSouth;

    }

}

bool  Camera_ZWO::ST4PulseGuideScope(int direction, int duration)
{
    pulseGuide(GetDirection(direction), duration);

    return false;
}

void  Camera_ZWO::ClearGuidePort()
{
    pulseGuide(guideNorth, 0);
}

#endif // ZWO_ASI
