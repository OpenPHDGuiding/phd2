//
//  cam_ATIKOSXUniversal.cpp
//  PHD
//
//  Created by Nick Kitchener on 22/01/2015.
//  Copyright (c) 2015 Nick Kitchener. All rights reserved.
//

#include "phd.h"
#if defined (ATIK_OSX)
#include <sstream>
#include "camera.h"
#include "time.h"
#include "image_math.h"
#include "wx/stopwatch.h"

#include "cam_ATIKOSXUniversal.h"
#include "Imager100.h"
#include "ImageFrame.h"
#include "ATIKModernModels.h"
#include "ATIKLegacyModels.h"
#include "ATIKExtensions.h"

// note OpenPHD have a logger class clashing..
#include "Logger.h"

Camera_ATIKOSXUniversal::Camera_ATIKOSXUniversal()
{
    Connected = false;
    Name = _T("ATIK OSX Universal");
    FullSize = wxSize(1280,1024);
    _debugEnabled = false;
    m_hasGuideOutput = true;
    HasGainControl = false;
    Color = false;
    HSModel = false;
    HasSubframes = true;
    _userSelectedBusId=0;
    _isLegacy = false;
    _legacyFTDIMappings = std::map<std::string, uint32_t>();
    
    // need to be able to setup a legacy FTDIChipID map before connecting..
    PropertyDialogType = PROPDLG_ANY;
    
    
    loadProfile();
    //    _legacyFTDIMappings[std::string("A3001QH8")] = 0x0ee8f4c9;
    //    storeProfile();
    //    loadProfile();
    
    _driversModern = new ATIKLinuxDrivers();
    _driversModern->startSelectiveSupport();
    _driversLegacy = new ATIKLinuxLegacyDrivers();
    
    //_driversLegacy->registerFTDIChipId(0x0ee8f4c9, "A3001QH8");
    updateRegisteredFTDIChipIDs();
    
    //    _ftdiChipIdMappings[std::string("A3001QH8")] = 0x0ee8f4c9;
    
    //    Logger::globalLogger("ATIKUniversalDrivers");
    //    Logger::setDebugLogging(false);
    //    Logger::important("OpenPHD2 instantiated driver instance");
    _driversLegacy->startSelectiveSupport();
    
    _imager=NULL;
}

Camera_ATIKOSXUniversal::Camera_ATIKOSXUniversal(uint16_t busIdentity) : Camera_ATIKOSXUniversal() {
    _userSelectedBusId = busIdentity;
}


void Camera_ATIKOSXUniversal::ShowPropertyDialog() {
    cam_ATIKOSXUniversalConfigDialog dlg(pFrame);
    dlg.setDriverVersion((char*)_driversModern->version());
    dlg.setDebugEnabled(_debugEnabled);
    dlg.setPreviewModeEnabled(_previewModeEnabled);
    dlg.setTitanModeEnabled(_titanModeEnabled);
    dlg.setBinningMode(_binning);
    dlg.setFIFOModeEnabled(_fifoEnabled);
    dlg.setMapping(_legacyFTDIMappings);
    dlg.updateAgainstParameters();
    
    if (dlg.ShowModal() == wxID_OK) {
        // set the parameters from the window.
        _debugEnabled = dlg.isDebugEnabled();
        _previewModeEnabled = dlg.isPreviewModeEnabled();
        _titanModeEnabled = dlg.isTitanModeEnabled();
        _binning = dlg.binningMode();
        _fifoEnabled = dlg.isFIFOModeEnabled();
        dlg.returnMapping(_legacyFTDIMappings);
        storeProfile();
        updateRegisteredFTDIChipIDs();
        _driversModern->setEnableDebug(_debugEnabled); // as OpenPHD has a class called Logger too!
    }
}



// This needs to disconnect on deletion unless the address space is wipted by quitting.



// we use a local driver instance to scan for supported cameras and add them to the list
// this means we keep upto date without having to update PHD each time and it doesn't result
// in a camera attempting to be used without actually having any support for it (ie future cameras
// and old drivers etc).
void Camera_ATIKOSXUniversal::supportedCameras(wxArrayString& list) {
    //list.Add(_T("ATIK Universal, mono"));
    
    ATIKLinuxDrivers* tempMD = new ATIKLinuxDrivers();
    ATIKLinuxLegacyDrivers* tempLD = new ATIKLinuxLegacyDrivers();
    // note we don't even need to start the driver support to just check passively.
    
    PotentialDeviceList* modernList = tempMD->scanForPerspectiveServices(kATIKModelModernAnyCamera);
    if( modernList->size() >0 ) {
        for(PotentialDeviceListIterator it=modernList->begin(); it!=modernList->end(); ++it) {
            char tempString[1024];
            sprintf(tempString,"%s (usb location 0x%04x)",it->second.c_str(),it->first);
            wxString wxS = wxString(tempString);
            list.Add(wxS);
        }
    }
    
    delete modernList;
    
    PotentialDeviceList* legacyList = tempMD->scanForPerspectiveServices(kATIKModelLegacyAnyCamera);
    if( modernList->size() >0 ) {
        for(PotentialDeviceListIterator it=legacyList->begin(); it!=legacyList->end(); ++it) {
            char tempString[1024];
            sprintf(tempString,"%s (usb location 0x%04x)",it->second.c_str(),it->first);
            wxString wxS = wxString(tempString);
            list.Add(wxS);
        }
    }
    
    delete legacyList;
    
    delete tempMD;
    delete tempLD;
}

// what we do here is scan again.. but this time we pass back the unique bus id..
// a bit clunky but we don't want any persistance or global variables..
int Camera_ATIKOSXUniversal::choiceFind(const wxString& choiceFind) {
    uint16_t returnedBusIdentity=0;
    
    ATIKLinuxDrivers* tempMD = new ATIKLinuxDrivers();
    ATIKLinuxLegacyDrivers* tempLD = new ATIKLinuxLegacyDrivers();
    // note we don't even need to start the driver support to just check passively.
    
    PotentialDeviceList* modernList = tempMD->scanForPerspectiveServices(kATIKModelModernAnyCamera);
    if( modernList->size() >0 ) {
        for(PotentialDeviceListIterator it=modernList->begin(); it!=modernList->end(); ++it) {
            char tempString[1024];
            sprintf(tempString,"%s (usb location 0x%04x)",it->second.c_str(),it->first);
            wxString wxS = wxString(tempString);
            if( choiceFind == wxS ) {
                // user has chosen a modern..
                returnedBusIdentity = it->first;
            }
        }
    }
    delete modernList;
    
    if( returnedBusIdentity==0) {
        PotentialDeviceList* legacyList = tempMD->scanForPerspectiveServices(kATIKModelLegacyAnyCamera);
        if( modernList->size() >0 ) {
            for(PotentialDeviceListIterator it=legacyList->begin(); it!=legacyList->end(); ++it) {
                char tempString[1024];
                sprintf(tempString,"%s (usb location 0x%04x)",it->second.c_str(),it->first);
                wxString wxS = wxString(tempString);
                if( choiceFind == wxS ) {
                    // user has chosen a legacy..
                    returnedBusIdentity = it->first;
                }
            }
        }
        delete legacyList;
    }
    
    delete tempMD;
    delete tempLD;
    
    return returnedBusIdentity;
}

bool Camera_ATIKOSXUniversal::Connect()
{
    // returns true on error
    if (_imager!=NULL) {
        wxMessageBox(_("Already connected"));
        return false;  // Already connected
    }
    
    // if the _userSelectedBusId > 0 then we have been told what to connect...
    
    //if( _userSelectedBusId==0 ){
    
    // build list of modern and legacy cameras
    // how the FTDIChipID would work here - that's something for later..
    // for the moment we're only going to ask for guider models..
    
    
    
    // Find available cameras
    // build a list of potential ATIK devices without connecting to them...
    
    int i=-1, index=0;
    wxArrayString usbATIKDevices;
    uint16_t addresses[1024];
    
    // we know the camera we're looking for is a modern..
    PotentialDeviceList* modernList = _driversModern->scanForPerspectiveServices(kATIKModelModernAnyCamera);
    if( modernList->size() >0 ) {
        for(PotentialDeviceListIterator it=modernList->begin(); it!=modernList->end(); ++it) {
            // because c++ string streams are ugly (although printf isn't particularly safe..)
            addresses[index]=it->first;
            char tempString[1024];
            sprintf(tempString,"%s (usb location 0x%04x)",it->second.c_str(),it->first);
            wxString wxS = wxString(tempString);
            usbATIKDevices.Add(tempString);
            index++;
        }
    }
    int legacyIndex = index;
    PotentialDeviceList* legacyList=_driversLegacy->scanForPerspectiveServices(kATIKModelLegacyAnyCamera);
    for(PotentialDeviceListIterator it=legacyList->begin(); it!=legacyList->end(); ++it) {
        // because c++ string streams are ugly (although printf isn't particularly safe..)
        addresses[index]=it->first;
        char tempString[1024];
        sprintf(tempString,"%s (usb location 0x%04x)",it->second.c_str(),it->first);
        wxString wxS = wxString(tempString);
        usbATIKDevices.Add(tempString);
        index++;
    }
    
    if(index==1)
        i=0; // one camera connected
    else if (index==0)
        return true; // no cameras.
    else {
        i = wxGetSingleChoiceIndex(_("Select camera"),_("Camera name"), usbATIKDevices);
        if (i == -1) {
            Disconnect();
            return true;
        }
    }
    
    _isLegacy = i>=legacyIndex;
    _userSelectedBusId = addresses[i];
    
    //}
    
    // add and claim camera
    ServiceInterface* service=NULL;
    
    if(!_isLegacy) {
        _driversModern->supportCameraIdentifiedByBusAddress(_userSelectedBusId);
        Services* serviceMgmt = _driversModern->serviceManagement();
        ServiceIdentityList* available = _driversModern->availableServices();
        // Check for modern camera
        if( available->size() != 0) {  // May have found a legacy device
            for(ServiceIdentityListIterator it=available->begin(); it!=available->end(); ++it) {
                std::string deviceIdentity = it->first;
                std::string protocol = it->second;
                if( protocol == "Imager100" ) { // in theory this should be Imager101 but for now this is 100.
                    //bIsLegacy = false;
                    service = serviceMgmt->claimService(deviceIdentity);
                    break;
                }
            }
        }
    } else {
        // ensure the chip ids stored in the profile are updated before we
        // attempt to connect!
        //  updateRegisteredFTDIChipIDs();
        
        _driversLegacy->supportCameraIdentifiedByBusAddress(_userSelectedBusId);
        Services* serviceMgmt = _driversLegacy->serviceManagement();
        ServiceIdentityList* available = _driversLegacy->availableServices();
        // Check for modern camera
        if( available->size() != 0) {  // May have found a legacy device
            for(ServiceIdentityListIterator it=available->begin(); it!=available->end(); ++it) {
                std::string deviceIdentity = it->first;
                std::string protocol = it->second;
                if( protocol == "Imager100" ) { // in theory this should be Imager101 but for now this is 100.
                    //bIsLegacy = false;
                    service = serviceMgmt->claimService(deviceIdentity);
                    break;
                }
            }
        }
    }
    
    
    if(service==NULL) {
        wxMessageBox(wxString::Format("Filed to connect to ATIK Camera (Driver version %s)",_driversModern->version()));
        return true;
    }
    
    //
    _imager=dynamic_cast<Imager100*>(service);
    
    // now have our camera all ready, update properties
    
    Name = _imager->uniqueIdentity();
    FullSize = wxSize(_imager->xPixels(), _imager->yPixels());
    PixelSize = _imager->yPixelSize();     // PHD assumes square pixels
    HasPortNum = false;
    HasDelayParam = false;
    HasGainControl = false; // could use this to engage preview?
    HasShutter = false;
    HasSubframes = true;
    Cooling100* cooling = _imager->cooling();
    
    _imager->setExtension(kArtemisExtensionKeyFIFO, _fifoEnabled?kArtemisExtensionValueYes:kArtemisExtensionValueNo);
    
    Connected = true;
    return false;
}

bool Camera_ATIKOSXUniversal::ST4PulseGuideScope(int direction, int duration)
{
    int axis;
    //wxStopWatch swatch;
    
    // Output pins are NC, Com, RA+(W), Dec+(N), Dec-(S), RA-(E) ??
    switch (direction) {
            /*      case WEST: axis = ARTEMIS_GUIDE_WEST; break;    // 0111 0000
             case NORTH: axis = ARTEMIS_GUIDE_NORTH; break;  // 1011 0000
             case SOUTH: axis = ARTEMIS_GUIDE_SOUTH; break;  // 1101 0000
             case EAST: axis = ARTEMIS_GUIDE_EAST;   break;  // 1110 0000*/
        case WEST: axis = 2; break; // 0111 0000
        case NORTH: axis = 0; break;    // 1011 0000
        case SOUTH: axis = 1; break;    // 1101 0000
        case EAST: axis = 3;    break;  // 1110 0000
        default: return true; // bad direction passed in
    }
    
    _imager->guidePort()->pulseGuide(axis, duration);
    
    //swatch.Start();
    //    ArtemisPulseGuide(Cam_Handle,axis,duration);  // returns after pulse
    //long t1 = swatch.Time();
    //wxMessageBox(wxString::Format("%ld",t1));
    /*  ArtemisGuide(Cam_Handle,axis);
     wxMilliSleep(duration);
     ArtemisStopGuiding(Cam_Handle);*/
    //if (duration > 50) wxMilliSleep(duration - 50);  // wait until it's mostly done
    //wxMilliSleep(duration + 10);
    return false;
}

void Camera_ATIKOSXUniversal::ClearGuidePort()
{
    _imager->guidePort()->stopGuiding();
}

bool Camera_ATIKOSXUniversal::Disconnect() {
    
    if(_imager!=NULL) {
        if(!_isLegacy) {
            _driversModern->releaseService(_imager);
            _driversModern->removeSupportCameraIdentifiedByBusAddress(_userSelectedBusId);
        } else {
            _driversLegacy->releaseService(_imager);
            _driversLegacy->removeSupportCameraIdentifiedByBusAddress(_userSelectedBusId);
        }
        _imager=NULL;
    }
    
    // if this is a shutdown of the class instance then the drivers will be shut down here.
    
    wxMilliSleep(100);
    Connected = false;
    _userSelectedBusId=0;
    
    _driversModern->endSupport();
    _driversLegacy->endSupport();
    
    return false;
}

static bool StopCapture()
{
    Debug.AddLine("Camera_ATIKOSXUniversal: cancel exposure");
    //_imager->abortCapture();
    return true;
}

bool Camera_ATIKOSXUniversal::Capture(int duration, usImage& img, wxRect subframe, bool recon) {
    bool TakeSubframe = UseSubframes;
    
    if (subframe.width <= 0 || subframe.height <= 0) {
        TakeSubframe = false;
    }
    
    ImageFrame* image = new ImageFrame(0, NULL);
    
    if(TakeSubframe) {
        image->setCameraFrame(subframe.x,subframe.y,subframe.width,subframe.height);
        img.Subframe = subframe;
    } else {
        img.Subframe = wxRect(0, 0, 0, 0);
    }
    
    image->setDuration(((float)(duration))/1000.0f); // duration as a float..
    image->setIsPreview(_previewModeEnabled);
    image->setBinning(_binning, _binning);

    try {
    
    _imager->prepare(image);
    image->selfAllocate(); // we could make this more efficient by using the 'img' buffer directly.
    
        _imager->snapShot(image, true); // blocking.. until exposure complete (or abort called by seperate thread)
        
    } catch(...) {
        // device could have detached.
        DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
        return false;
    }
    
    _imager->postProcess(image);
    
    // image ready.. it would have been subframed if needed..
    
    
    // int data_x,data_y,data_w,data_h,data_binx,data_biny;
    //    ArtemisGetImageData(Cam_Handle, &data_x, &data_y, &data_w, &data_h, &data_binx, &data_biny);
    
    uint32_t data_x,data_y,data_w,data_h,data_binx,data_biny;
    
    image->cameraFrame(&data_x, &data_y, &data_w, &data_h);
    image->binning(&data_binx, &data_biny);
    
    if (img.Init(FullSize)) {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }
    
    
    
    //    if (TakeSubframe) {
    //        unsigned short *dptr = img.ImageData;
    //        img.Subframe = subframe;
    //        for (int i=0; i<img.NPixels; i++, dptr++)
    //            *dptr = 0;
    //        //unsigned short *rawptr = (unsigned short *) ArtemisImageBuffer(Cam_Handle);
    //        unsigned short *rawptr = (unsigned short*) image->imagebuffer();
    //        for (int y=0; y<subframe.height; y++) {
    //            dptr = img.ImageData + (y+subframe.y)*FullSize.GetWidth() + subframe.x;
    //            for (int x=0; x<subframe.width; x++, dptr++, rawptr++)
    //                *dptr = *rawptr;
    //        }
    //    }
    //    else {
    uint16_t *dptr = (uint16_t*) img.ImageData;
    
    // use memset to blank as it doesn't loose time to repeat security checks.
    memset(dptr, 0, img.NPixels);
    
    printf("PHD thinks the image size is %i pixels, image is %i pixels\n", img.NPixels, data_w*data_h);
    printf("PHD thinks the image size is %ix%i pixels, image is %ix%i pixels\n", img.Size.GetWidth(),img.Size.GetHeight(), data_w,data_h);
    
    // OpenPHD assumes that the driver will give it a X by Y image even if the binning is not wholy divisable.
    //
    
    // now we use the data_w data_h to drive the image copy
    // note that binning will return a single value for the N binned data,

    int blankW = img.Size.GetWidth()-data_w;
    uint16_t *rawptr = (uint16_t*) image->imagebuffer();
    for(int h=0;h<data_h;h++) {
        for(int w=0;w<data_w;w++, dptr++, rawptr++) {
            *dptr = *rawptr;
        }
        // if there's any gap in the row we need to add to the destination
        dptr += blankW;
    }
    
//    
//    
//    if( img.Size.GetHeight() > data_h ) {
//        uint16_t *rawptr = (uint16_t*) image->imagebuffer();
//        for (int i=0; i<data_h; i++,dptr++, rawptr++) {
//            *dptr = *rawptr;
//        }
//
//    } else {
//        //unsigned short *rawptr = (unsigned short *) ArtemisImageBuffer(Cam_Handle);
//        uint16_t *rawptr = (uint16_t*) image->imagebuffer();
//        for (int i=0; i<img.NPixels; i++,dptr++, rawptr++) {
//            *dptr = *rawptr;
//        }
//    }
    
    delete image;
    
    // Do quick L recon to remove bayer array
    if (recon) SubtractDark(img);
    if (Color && recon) QuickLRecon(img);
    
    return false;
}

// Now the more efficient version..

//bool Camera_ATIKOSXUniversal::Capture(int duration, usImage& img, wxRect subframe, bool recon) {
//    bool TakeSubframe = UseSubframes;
//
//    if (subframe.width <= 0 || subframe.height <= 0) {
//        TakeSubframe = false;
//    }
//
//    ImageFrame* image = new ImageFrame(0, NULL);
//
//    if(TakeSubframe) {
//        image->setCameraFrame(subframe.x,subframe.y,subframe.width,subframe.height);
//        img.Subframe = subframe;
//    } else {
//        img.Subframe = wxRect(0, 0, 0, 0);
//    }
//
//    image->setDuration(((float)(duration))/1000.0f); // duration as a float..
//
//    _imager->prepare(image);
//
//    if (img.Init(FullSize)) {
//        DisconnectWithAlert(CAPT_FAIL_MEMORY);
//        return true;
//    }
//
//    printf("PHD thinks the image size is %i pixels, image is %i pixels (image size required by driver is %i bytes)\n", img.NPixels, img.Size.GetWidth()*img.Size.GetHeight(), image->imagebufferSize());
//
//    // in this we use the image data location immediately
//    image->setExternalBuffer((uint8_t*)img.ImageData);
//
//
//    //image->selfAllocate(); // we could make this more efficient by using the 'img' buffer directly.
//
//    _imager->snapShot(image, true); // blocking.. until exposure complete (or abort called by seperate thread)
//
//
//    //    if (ArtemisStartExposure(Cam_Handle,(float) duration / 1000.0))  {
//    //        pFrame->Alert(_("Couldn't start exposure - aborting"));
//    //        return true;
//    //    }
//
//    //    CameraWatchdog watchdog(duration, GetTimeoutMs());
//
//    // but if active proxy used we can use this instead...
//    //    while (ArtemisCameraState(Cam_Handle) > CAMERA_IDLE)
//    //    {
//    //        if (duration > 100)
//    //            wxMilliSleep(100);
//    //        else
//    //            wxMilliSleep(30);
//    //        if (WorkerThread::InterruptRequested() &&
//    //            (WorkerThread::TerminateRequested() || StopCapture(Cam_Handle)))
//    //        {
//    //            return true;
//    //        }
//    //        if (watchdog.Expired())
//    //        {
//    //            DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
//    //            return true;
//    //        }
//    //    }
//
//    _imager->postProcess(image);
//
//    // image ready.. it would have been subframed if needed..
//
//
//    // int data_x,data_y,data_w,data_h,data_binx,data_biny;
//    //    ArtemisGetImageData(Cam_Handle, &data_x, &data_y, &data_w, &data_h, &data_binx, &data_biny);
//
//    //uint32_t data_x,data_y,data_w,data_h,data_binx,data_biny;
//
//    //image->cameraFrame(&data_x, &data_y, &data_w, &data_h);
//   // image->binning(&data_binx, &data_biny);
//
//
//
//
//    //    if (TakeSubframe) {
//    //        unsigned short *dptr = img.ImageData;
//    //        img.Subframe = subframe;
//    //        for (int i=0; i<img.NPixels; i++, dptr++)
//    //            *dptr = 0;
//    //        //unsigned short *rawptr = (unsigned short *) ArtemisImageBuffer(Cam_Handle);
//    //        unsigned short *rawptr = (unsigned short*) image->imagebuffer();
//    //        for (int y=0; y<subframe.height; y++) {
//    //            dptr = img.ImageData + (y+subframe.y)*FullSize.GetWidth() + subframe.x;
//    //            for (int x=0; x<subframe.width; x++, dptr++, rawptr++)
//    //                *dptr = *rawptr;
//    //        }
//    //    }
//    //    else {
//   // uint16_t *dptr = (uint16_t*) img.ImageData;
//
////    //unsigned short *rawptr = (unsigned short *) ArtemisImageBuffer(Cam_Handle);
////    uint16_t *rawptr = (uint16_t*) image->imagebuffer();
////    for (int i=0; i<img.NPixels; i++,dptr++, rawptr++) {
////        *dptr = *rawptr;
////    }
//
//    delete image;
//
//    // Do quick L recon to remove bayer array
//    if (recon) SubtractDark(img);
//    if (Color && recon) QuickLRecon(img);
//
//    return false;
//}



bool Camera_ATIKOSXUniversal::HasNonGuiCapture(void) {
    return true;
}

bool Camera_ATIKOSXUniversal::ST4HasNonGuiMove(void) {
    return true;
}


// Profile is defined as:
//  /ATIK/<CameraModel>/Default/Settings/<key>

// loads the FTDIChipIds out of the profile and registers them aith the
// legacy driver
void Camera_ATIKOSXUniversal::updateRegisteredFTDIChipIDs() {
    for(std::map<std::string, uint32_t>::iterator it=_legacyFTDIMappings.begin(); it!=_legacyFTDIMappings.end(); it++) {
        _driversLegacy->registerFTDIChipId(it->second, (char*) it->first.c_str());
    }
}

// [x] Preview Mode
// [x] Titan autodark plantary Mode (Titan only)
//
// Camera Binning
// [n] x [n] Binning
//
// Legacy Camera
// [x] FIFO Enabled
// Legacy FTDIChipMappings
// [ [n]->[n] ]
// [ [n]->[n] ]

void Camera_ATIKOSXUniversal::loadProfile() {
    
    // modern parameters
    _previewModeEnabled = pConfig->Profile.GetBoolean("PreviewMode", false);
    _titanModeEnabled = pConfig->Profile.GetBoolean("TitanMode", false);
    _binning = 1; // pConfig->Profile.GetInt("Binning", 1);
    
    // legacy parameters
    _fifoEnabled = pConfig->Profile.GetBoolean("FIFOEnabled", true);
    // legacy ftdi mappings table
    // note we use a system that has (N -> "serialnum=chipid")
    uint8_t i=0;
    wxString value;
    do{
        char arrayKey[12];
        sprintf(arrayKey,"%i", i);
        value = pConfig->Global.GetString(arrayKey, _("ARRAYEND=ARRAYEND"));
        if( value!="ARRAYEND=ARRAYEND" ) {
            std::string valueString = std::string(value);
            std::string serialNumberString = valueString.substr(0,value.find('='));
            std::string mappingNumberString = valueString.substr(value.find('=')+1, value.length());
            uint32_t mappingNumber=0;
            try {
                std::istringstream(mappingNumberString) >> std::hex >> mappingNumber;
                _legacyFTDIMappings[serialNumberString] = mappingNumber;
            } catch(...) {
                // failed to convert..
            }
            i++;
        }
    } while( value!="ARRAYEND=ARRAYEND" );
    
    // if it's empty we add Nicks as this seems to be a common identity.
    if( _legacyFTDIMappings.size()==0) {
        _legacyFTDIMappings[std::string("A3001QH8")] = 0x0ee8f4c9;
    }
    
}

void Camera_ATIKOSXUniversal::storeProfile() {
    
    // modern parameters
    pConfig->Profile.SetBoolean("PreviewMode", _previewModeEnabled);
    pConfig->Profile.SetBoolean("TitanMode", _titanModeEnabled);
    pConfig->Profile.SetInt("Binning", 1); // _binning
    
    // legacy parameters
    pConfig->Profile.SetBoolean("FIFOEnabled", _fifoEnabled);
    // legacy ftdi mappings table
    // note we use a system that has (N -> "serialnum=chipid")
    uint8_t i=0;
    char key[64], value[64];
    char arrayKey[12];
    for(std::map<std::string, uint32_t>::iterator it=_legacyFTDIMappings.begin(); it!=_legacyFTDIMappings.end(); it++) {
        sprintf(arrayKey,"%i", i);
        sprintf(value, "%s=%x", it->first.c_str(), it->second);
        pConfig->Global.SetString(arrayKey, _(value));
        i++;
    }
    sprintf(arrayKey,"%i", i);
    pConfig->Global.SetString(arrayKey, _("ARRAYEND=ARRAYEND"));
    
}


#endif
