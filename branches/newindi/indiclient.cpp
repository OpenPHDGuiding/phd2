/*
 *  indiclient.cpp
 *  PHD Guiding
 *
 *  Created by Markus Wieczorek.
 *  Copyright (c) 2014 Markus Wieczorek.
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

#include "indiclient.h"
#include <libindi/baseclient.h>
#include <libindi/basedevice.h>
#include <libindi/indiproperty.h>
extern wxString INDICameraName;

MyINDIClient::MyINDIClient()
{
indi_device = NULL;
}

MyINDIClient::~MyINDIClient()
{
}

void MyINDIClient::newDevice(INDI::BaseDevice *dp)
{
if (!strcmp(dp->getDeviceName(), INDICameraName.mb_str(wxConvUTF8)))
IDLog("Receiving %s Device...\n", dp->getDeviceName());
indi_device = dp;
}

void MyINDIClient::setGain(int gain)
{
   INumberVectorProperty *ccd_gain = NULL;
   ccd_gain = indi_device->getNumber("CCD_GAIN");
   if (ccd_gain == NULL)
   {
     IDLog("Error: unable to find CCD_GAIN property...\n");
     return;
   }
   ccd_gain->np->value = gain;
   sendNewNumber(ccd_gain);
}

void MyINDIClient::setExpotime(int duration)
{
   INumberVectorProperty *ccd_expotime = NULL;
   ccd_expotime = indi_device->getNumber("CCD_EXPOSURE");
   if (ccd_expotime == NULL)
   {
     IDLog("Error: unable to find CCD_EXPOSURE property...\n");
     return;
   }
   ccd_expotime->np->value = duration/1000.0;
   sendNewNumber(ccd_expotime);
}

void MyINDIClient::newProperty(INDI::Property *property)
{
if (!strcmp(property->getDeviceName(), INDICameraName.mb_str(wxConvUTF8)) && !strcmp(property->getName(), "CONNECTION"))
{
connectDevice(INDICameraName.mb_str(wxConvUTF8));
return;
}
if (!strcmp(property->getDeviceName(), INDICameraName.mb_str(wxConvUTF8)) && !strcmp(property->getName(), "CCD_EXPOSURE"))
{
if (indi_device->isConnected())
{
IDLog("CCD is connected. Exposure time is: %f.\n", property->getNumber()->np->value);
//setTemperature();
}
return;
}
}

void MyINDIClient::newMessage(INDI::BaseDevice *dp, int messageID)
{
if (strcmp(dp->getDeviceName(), INDICameraName.mb_str(wxConvUTF8)))
return;
IDLog("Recveing message from Server:\n\n########################\n%s\n########################\n\n", dp->messageQueue(messageID));
}

void MyINDIClient::newNumber(INumberVectorProperty *nvp)
{
// Let's check if we get any new values for CCD_TEMPERATURE
  /*
if (!strcmp(nvp->name, "CCD_TEMPERATURE"))
{
IDLog("Receving new CCD Temperature: %g C\n", nvp->np[0].value);
if (nvp->np[0].value == -20)
{
IDLog("CCD temperature reached desired value!\n");
takeExposure();
}
}
*/
}

void MyINDIClient::newBLOB(IBLOB *bp)
{
// Save FITS file to disk
 /*
ofstream myfile;
myfile.open ("ccd_simulator.fits", ios::out | ios::binary);
myfile.write(static_cast<char *> (bp->blob), bp->bloblen);
myfile.close();
IDLog("Received image, saved as ccd_simulator.fits\n");
*/
}

#endif