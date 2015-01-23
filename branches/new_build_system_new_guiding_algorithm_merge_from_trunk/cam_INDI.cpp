/*
 *  cam_INDI.cpp
 *  PHD Guiding
 *
 *  Created by Geoffrey Hausheer.
 *  Copyright (c) 2009 Geoffrey Hausheer.
 *  All rights reserved.
 * 
 *  Redraw for libindi/baseclient by Patrick Chevalley
 *  Copyright (c) 2014 Patrick Chevalley
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

#ifdef INDI_CAMERA

#include <iostream>
#include <fstream>

#include "config_INDI.h"
#include "camera.h"
#include "time.h"
#include "image_math.h"
#include "cam_INDI.h"

Camera_INDIClass::Camera_INDIClass() 
{
    ClearStatus();
    // load the values from the current profile
    INDIhost = pConfig->Profile.GetString("/indi/INDIhost", _T("localhost"));
    INDIport = pConfig->Profile.GetLong("/indi/INDIport", 7624);
    INDICameraName = pConfig->Profile.GetString("/indi/INDIcam", _T("INDI Camera"));
    INDICameraCCD = pConfig->Profile.GetLong("/indi/INDIcam_ccd", 0);
    INDICameraPort = pConfig->Profile.GetString("/indi/INDIcam_port",_T(""));
    Name = INDICameraName;
    SetCCDdevice();
    PropertyDialogType = PROPDLG_ANY;
    FullSize = wxSize(640,480);
}

Camera_INDIClass::~Camera_INDIClass() 
{
  disconnectServer();    
}

void Camera_INDIClass::ClearStatus()
{
    // reset properties pointer
    expose_prop = NULL;
    frame_prop = NULL;
    frame_type_prop = NULL;
    binning_prop = NULL;
    video_prop = NULL;
    camera_port = NULL;
    camera_device = NULL;
    pulseGuideNS_prop = NULL;
    pulseGuideEW_prop = NULL;
    // gui self destroy on lost connection
    gui = NULL;
    // reset connection status
    has_blob = false;
    Connected = false;
    ready = false;
    m_hasGuideOutput = false;
}

void Camera_INDIClass::CheckState() 
{
    // Check if the device has all the required properties for our usage.
    if(has_blob && Connected && (expose_prop || video_prop)) {
        if (! ready) {
            //printf("Camera is ready\n");
            ready = true;
            if (modal) {
                modal = false;
            }
        }
    }
}

void Camera_INDIClass::newDevice(INDI::BaseDevice *dp)
{
  if (strcmp(dp->getDeviceName(), INDICameraName.mb_str(wxConvUTF8)) == 0) {
      // The camera object, maybe this can be useful in the future
      camera_device = dp;
  }
}

void Camera_INDIClass::newSwitch(ISwitchVectorProperty *svp)
{
    // we go here every time a Switch state change
    //printf("Camera Receving Switch: %s = %i\n", svp->name, svp->sp->s);
    if (strcmp(svp->name, "CONNECTION") == 0) {
	ISwitch *connectswitch = IUFindSwitch(svp,"CONNECT");
	Connected = (connectswitch->s == ISS_ON);
    }
}

void Camera_INDIClass::newMessage(INDI::BaseDevice *dp, int messageID)
{
    // we go here every time the camera driver send a message
    //printf("Camera Receving message: %s\n", dp->messageQueue(messageID));
}

void Camera_INDIClass::newNumber(INumberVectorProperty *nvp)
{
    // we go here every time a Number value change
    //printf("Camera Receving Number: %s = %g\n", nvp->name, nvp->np->value);
}

void Camera_INDIClass::newText(ITextVectorProperty *tvp)
{
    // we go here every time a Text value change
    //printf("Camera Receving Text: %s = %s\n", tvp->name, tvp->tp->text);
}

void  Camera_INDIClass::newBLOB(IBLOB *bp)
{
    // we go here every time a new blob is available
    // this is normally the image from the camera
    //printf("Got camera blob %s \n",bp->name);
    if (expose_prop) {
	if (strcmp(bp->name,INDICameraBlobName)==0){
	cam_bp = bp;
	modal = false;
	}
    }
    else if (video_prop){
	cam_bp = bp;
	// TODO : cumulate the frames received during exposure
    }
}

void Camera_INDIClass::newProperty(INDI::Property *property) 
{
    // Here we receive a list of all the properties after the connection
    // Updated values are not received here but in the newTYPE() functions above.
    // We keep the vector for each interesting property to send some data later.
    //const char* DeviName = property->getDeviceName();
    const char* PropName = property->getName();
    INDI_TYPE Proptype = property->getType();
    //printf("Camera Property: %s\n",PropName);
    
    if (Proptype == INDI_BLOB) {
        //printf("Found BLOB property for %s %s\n", DeviName, PropName);
        has_blob = 1;
    }
    else if ((strcmp(PropName, INDICameraCCDCmd+"EXPOSURE") == 0) && Proptype == INDI_NUMBER) {
	//printf("Found CCD_EXPOSURE for %s %s\n", DeviName, PropName);
	expose_prop = property->getNumber();
    }
    else if ((strcmp(PropName, INDICameraCCDCmd+"FRAME") == 0) && Proptype == INDI_NUMBER) {
	//printf("Found CCD_FRAME for %s %s\n", DeviName, PropName);
	frame_prop = property->getNumber();
    }
    else if ((strcmp(PropName, INDICameraCCDCmd+"FRAME_TYPE") == 0) && Proptype == INDI_SWITCH) {
	//printf("Found CCD_FRAME_TYPE for %s %s\n", DeviName, PropName);
	frame_type_prop = property->getSwitch();
    }
    else if ((strcmp(PropName, INDICameraCCDCmd+"BINNING") == 0) && Proptype == INDI_NUMBER) {
	//printf("Found CCD_BINNING for %s %s\n",DeviName, PropName);
	binning_prop = property->getNumber();
    }
    else if ((strcmp(PropName, "VIDEO_STREAM") == 0) && Proptype == INDI_SWITCH) {
	//printf("Found Video %s %s\n",DeviName, PropName);
	video_prop = property->getSwitch();
    }
    else if (strcmp(PropName, "DEVICE_PORT") == 0 && Proptype == INDI_TEXT) {
	//printf("Found device port for %s \n",DeviName);
	camera_port = property->getText();
    }
    else if (strcmp(PropName, "CONNECTION") == 0 && Proptype == INDI_SWITCH) {
	//printf("Found CONNECTION for %s %s\n",DeviName, PropName);
	// Check the value here in case the device is already connected
	ISwitch *connectswitch = IUFindSwitch(property->getSwitch(),"CONNECT");
	Connected = (connectswitch->s == ISS_ON);
    }
    else if ((strcmp(PropName, "TELESCOPE_TIMED_GUIDE_NS") == 0) && Proptype == INDI_NUMBER){
	pulseGuideNS_prop = property->getNumber();
	pulseN_prop = IUFindNumber(pulseGuideNS_prop,"TIMED_GUIDE_N");
	pulseS_prop = IUFindNumber(pulseGuideNS_prop,"TIMED_GUIDE_S");
    }
    else if ((strcmp(PropName, "TELESCOPE_TIMED_GUIDE_WE") == 0) && Proptype == INDI_NUMBER){
	pulseGuideEW_prop = property->getNumber();
	pulseW_prop = IUFindNumber(pulseGuideEW_prop,"TIMED_GUIDE_W");
	pulseE_prop = IUFindNumber(pulseGuideEW_prop,"TIMED_GUIDE_E");
    }
    else if (strcmp(PropName, "CCD_INFO") == 0 && Proptype == INDI_NUMBER) {
        PixelSize = IUFindNumber(property->getNumber(),"CCD_PIXEL_SIZE")->value;
	FullSize = wxSize(IUFindNumber(property->getNumber(),"CCD_MAX_Y")->value,IUFindNumber(property->getNumber(),"CCD_MAX_X")->value);
    }
    
    CheckState();
}

bool Camera_INDIClass::Connect() 
{
    // If not configured open the setup dialog
    if (strcmp(INDICameraName,"INDI Camera")==0) CameraSetup();
    // define server to connect to.
    setServer(INDIhost.mb_str(wxConvUTF8), INDIport);
    // Receive messages only for our camera.
    watchDevice(INDICameraName.mb_str(wxConvUTF8));
    // Connect to server.
    if (connectServer()) {
       return !ready;
    }
    else {
       // last chance to fix the setup
       CameraSetup();
       setServer(INDIhost.mb_str(wxConvUTF8), INDIport);
       watchDevice(INDICameraName.mb_str(wxConvUTF8));
       if (connectServer()) {
	  return !ready;
       }
       else {
	  return true;
      }
    }
}

bool Camera_INDIClass::Disconnect() 
{
    // Disconnect from server
    if (disconnectServer()){
	return false;
    }
    else return true;
}

void Camera_INDIClass::serverConnected()
{
    // After connection to the server
    // set option to receive blob and messages for the selected CCD
    setBLOBMode(B_ALSO, INDICameraName.mb_str(wxConvUTF8), INDICameraBlobName.mb_str(wxConvUTF8));
    modal = true; 
    // wait for the device port property
    wxLongLong msec;
    msec = wxGetUTCTimeMillis();
    while ((!camera_port) && wxGetUTCTimeMillis() - msec < 1 * 1000) {
	::wxSafeYield();
    }
    // Set the port, this must be done before to try to connect the device
    if (camera_port && INDICameraPort.Length()) {  // the camera port is not mandatory
	char* porttext = (const_cast<char*>((const char*)INDICameraPort.mb_str()));
	camera_port->tp->text = porttext;
	sendNewText(camera_port); 
    }
    // Connect the camera device
    connectDevice(INDICameraName.mb_str(wxConvUTF8));
    
    msec = wxGetUTCTimeMillis();
    while (modal && wxGetUTCTimeMillis() - msec < 5 * 1000) {
	::wxSafeYield();
    }
    modal = false;
    // In case we not get all the required properties or connection to the device failed
    if(ready) 
    {
	Connected = true;
	m_hasGuideOutput = (pulseGuideNS_prop && pulseGuideEW_prop);
    }
    else {
	pFrame->Alert(_("Cannot connect to camera ")+INDICameraName);
	Connected = false;
	Disconnect();
    }
}

void Camera_INDIClass::serverDisconnected(int exit_code)
{
   // in case the connection lost we must reset the client socket
   Disconnect();
   // after disconnection we reset the connection status and the properties pointers
   ClearStatus();
}

void Camera_INDIClass::ShowPropertyDialog() 
{
    if (Connected) {
	// show the devices INDI dialog
	CameraDialog();
    }
    else {
	// show the server and device configuration
	CameraSetup();
    }
}

void Camera_INDIClass::CameraDialog() 
{
   if (gui) {
      gui->Show();
   }
   else {
      gui = new IndiGui();
      gui->child_window = true;
      gui->allow_connect_disconnect = false;
      gui->ConnectServer(INDIhost, INDIport);
      gui->Show();
   }
}

void Camera_INDIClass::CameraSetup() 
{
    // show the server and device configuration
    INDIConfig *indiDlg = new INDIConfig(wxGetActiveWindow(),TYPE_CAMERA);
    indiDlg->INDIhost = INDIhost;
    indiDlg->INDIport = INDIport;
    indiDlg->INDIDevName = INDICameraName;
    indiDlg->INDIDevCCD = INDICameraCCD; 
    indiDlg->INDIDevPort = INDICameraPort;
    // initialize with actual values
    indiDlg->SetSettings();
    // try to connect to server
    indiDlg->Connect();
    if (indiDlg->ShowModal() == wxID_OK) {
	// if OK save the values to the current profile
	indiDlg->SaveSettings();
	INDIhost = indiDlg->INDIhost;
	INDIport = indiDlg->INDIport;
	INDICameraName = indiDlg->INDIDevName;
	INDICameraCCD = indiDlg->INDIDevCCD;
	INDICameraPort = indiDlg->INDIDevPort;
	pConfig->Profile.SetString("/indi/INDIhost", INDIhost);
	pConfig->Profile.SetLong("/indi/INDIport", INDIport);
	pConfig->Profile.SetString("/indi/INDIcam", INDICameraName);
	pConfig->Profile.SetLong("/indi/INDIcam_ccd",INDICameraCCD);
	pConfig->Profile.SetString("/indi/INDIcam_port",INDICameraPort);
	Name = INDICameraName;
	SetCCDdevice();
    }
    indiDlg->Disconnect();
    indiDlg->Destroy();
    delete indiDlg;
}

void  Camera_INDIClass::SetCCDdevice()
{
    if (INDICameraCCD == 0) {
	INDICameraBlobName = "CCD1";
	INDICameraCCDCmd = "CCD_";
    } 
    else {
	INDICameraBlobName = "CCD2";
	INDICameraCCDCmd = "GUIDER_";
    } 
}

bool Camera_INDIClass::ReadFITS(usImage& img) 
{
    int xsize, ysize;
    fitsfile *fptr;  // FITS file pointer
    int status = 0;  // CFITSIO status value MUST be initialized to zero!
    int hdutype, naxis;
    int nhdus=0;
    long fits_size[2];
    long fpixel[3] = {1,1,1};
    size_t bsize = static_cast<size_t>(cam_bp->bloblen);
    
    // load blob to CFITSIO
    if (fits_open_memfile(&fptr,
            "",
            READONLY,
            &(cam_bp->blob),
            &bsize,
            0,
            NULL,
            &status) )
    {
        pFrame->Alert(_("Unsupported type or read error loading FITS file"));
        return true;
    }
    if (fits_get_hdu_type(fptr, &hdutype, &status) || hdutype != IMAGE_HDU) {
        pFrame->Alert(_("FITS file is not of an image"));
        return true;
    }

    // Get HDUs and size
    fits_get_img_dim(fptr, &naxis, &status);
    fits_get_img_size(fptr, 2, fits_size, &status);
    xsize = (int) fits_size[0];
    ysize = (int) fits_size[1];
    fits_get_num_hdus(fptr,&nhdus,&status);
    if ((nhdus != 1) || (naxis != 2)) {
        pFrame->Alert(_("Unsupported type or read error loading FITS file"));
        return true;
    }
    if (img.Init(xsize,ysize)) {
        pFrame->Alert(_("Memory allocation error"));
        return true;
    }
    // Read image
    if (fits_read_pix(fptr, TUSHORT, fpixel, xsize*ysize, NULL, img.ImageData, NULL, &status) ) { 
        pFrame->Alert(_("Error reading data"));
        return true;
    }
    fits_close_file(fptr,&status);
    return false;
}

bool Camera_INDIClass::ReadStream(usImage& img) 
{
    int xsize, ysize;
    unsigned char *inptr;
    unsigned short *outptr;

    if (! frame_prop) {
        pFrame->Alert(_("No CCD_FRAME property, failed to determine image dimensions"));
        return true;
    }
    
    INumber *f_num = IUFindNumber(frame_prop,"WIDTH");
 
    if (! (f_num)) {
        pFrame->Alert(_("No WIDTH value, failed to determine image dimensions"));
        return true;
    }
    xsize = f_num->value;

    f_num = IUFindNumber(frame_prop,"HEIGHT");
    
    if (! (f_num)) {
        pFrame->Alert(_("No HEIGHT value, failed to determine image dimensions"));
        return true;
    }
    ysize = f_num->value;
    
    // allocate image
    if (img.Init(xsize,ysize)) {
        pFrame->Alert(_("CCD stream: memory allocation error"));
        return true;
    }
    // copy image
    outptr = img.ImageData;
    inptr = (unsigned char *) cam_bp->blob;
    for (int i = 0; i < xsize * ysize; i++)
        *outptr ++ = *inptr++;
    return false;
}

bool Camera_INDIClass::Capture(int duration, usImage& img, wxRect subframe, bool recon)
{
  if (Connected) {
      // we can set the exposure time directly in the camera
      if (expose_prop) {
	  //printf("Exposing for %d(ms)\n", duration);
	  
	  // set the exposure time, this immediately start the exposure
	  expose_prop->np->value = (double)duration/1000;
	  sendNewNumber(expose_prop);
	  
	  modal = true;  // will be reset when the image blob is received
	  
	  unsigned long loopwait = duration > 100 ? 10 : 1;
	  
	  CameraWatchdog watchdog(duration, GetTimeoutMs());
	  
	  while (modal) {
	     wxMilliSleep(loopwait);
	     if (WorkerThread::TerminateRequested())
		return true;
	     if (watchdog.Expired())
	     {
		DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
		return true;
	     }
	  }
      }
      // for video camera without exposure time setting
      else if (video_prop){
	  //printf("Enabling video capture\n");
	  ISwitch *v_on = IUFindSwitch(video_prop,"ON");
	  ISwitch *v_off = IUFindSwitch(video_prop,"OFF");
	  v_on->s = ISS_ON;
	  v_off->s = ISS_OFF;
	  // start capture, every video frame is received as a blob
	  sendNewSwitch(video_prop);
	  
	  // wait the required time
	  wxMilliSleep(duration); // TODO : add the frames received during exposure

	  //printf("Stop video capture\n");
	  v_on->s = ISS_OFF;
	  v_off->s = ISS_ON;
	  sendNewSwitch(video_prop);
      }
      else {
	  return true;
      }

      //printf("Exposure end\n");
      
      if (strcmp(cam_bp->format, ".fits") == 0) {
	 //printf("Processing fits file\n");
	 // for CCD camera
	 if ( ! ReadFITS(img) ) {
	    if ( recon ) {
	       //printf("Subtracting dark\n");
	       SubtractDark(img);
	    }
	    return false;
	 } else {
	    return true;
	 }
      } else if (strcmp(cam_bp->format, ".stream") == 0) {
	 //printf("Processing stream file\n");
	 // for video camera
	 return ReadStream(img);
      } else {
	 pFrame->Alert(_("Unknown image format: ") + wxString::FromAscii(cam_bp->format));
	 return true;
      }

  }
  else {
      // in case the camera is not connected
      return true;
  }
  // we must never go here
  return true;
}

bool Camera_INDIClass::HasNonGuiCapture(void) 
{
    return true;
}

// Camera ST4 port

bool Camera_INDIClass::ST4HasNonGuiMove(void)
{
    return true;
}

bool Camera_INDIClass::ST4PulseGuideScope(int direction, int duration)
{
    if (pulseGuideNS_prop && pulseGuideEW_prop) {
	switch (direction) {
	    case EAST:
		pulseE_prop->value = duration;
		pulseW_prop->value = 0;
		sendNewNumber(pulseGuideEW_prop);
		break;
	    case WEST:
		pulseE_prop->value = 0;
		pulseW_prop->value = duration;
		sendNewNumber(pulseGuideEW_prop);
		break;
	    case NORTH:
		pulseN_prop->value = duration;
		pulseS_prop->value = 0;
		sendNewNumber(pulseGuideNS_prop);
		break;
	    case SOUTH:
		pulseN_prop->value = 0;
		pulseS_prop->value = duration;
		sendNewNumber(pulseGuideNS_prop);
		break;
	    case NONE:
		printf("error CameraINDI::Guide NONE\n");
		break;
	}
	wxMilliSleep(duration);
	return false;
    }
    else return true;
}
   
    
#endif
