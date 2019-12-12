/*
// CVResVidCap - Result codes for video capture devices 
// Written by Michael Ellison
//-------------------------------------------------------------------------
//                      CodeVis's Free License
//                         www.codevis.com
//
// Copyright (c) 2003 by Michael Ellison (mike@codevis.com)
// All rights reserved.
//
// You may use this software in source and/or binary form, with or without
// modification, for commercial or non-commercial purposes, provided that 
// you comply with the following conditions:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer. 
//
// * Redistributions of modified source must be clearly marked as modified,
//   and due notice must be placed in the modified source indicating the
//   type of modification(s) and the name(s) of the person(s) performing
//   said modification(s).
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//---------------------------------------------------------------------------
// Modifications:
//
//---------------------------------------------------------------------------
/// \file CVResVidCap.h 
/// \brief Result codes for video capture subsystem.
///
/// Prefer to include into .cpp files that need these codes, and include
/// CVRes.h into headers to speed compile times and reduce recompilation
/// when changing result codes.
/// \sa CVRes.h
///
/// $RCSfile: CVResVidCap.h,v $
/// $Date: 2004/02/18 21:36:21 $
/// $Revision: 1.3 $
/// $Author: mikeellison $
*/

#ifndef _CVRESVIDCAP_H_
#define _CVRESVIDCAP_H_

#include "CVRes.h"

enum CVRES_VIDCAP_ENUM
{
   /*------------------------------------------------------------------------
   // Video capture status codes
   */
    
   /*! Video capture is running and active */
   CVRES_VIDCAP_RUNNING = CVRES_VIDCAP_STATUS + 1,
   /*! Video capture already initialized. */
   CVRES_VIDCAP_ALREADY_INITIALIZED,
   /*! Video capture dev already connected. */
   CVRES_VIDCAP_ALREADY_CONNECTED,    
   /*! Video capture not initialized. */
   CVRES_VIDCAP_NOT_INITIALIZED,      
   /*! Video capture not connected. */
   CVRES_VIDCAP_NOT_CONNECTED,        
   /*! Video capture already stopped. */
   CVRES_VIDCAP_ALREADY_STOPPED,      
   
   /*------------------------------------------------------------------------
   // Video capture errors
   */

   /*! COM error - CoInitializeEx() failed. */
   CVRES_VIDCAP_COM_ERR = CVRES_VIDCAP_ERROR + 1,
   /*! Enumeration callback must be specified. */
   CVRES_VIDCAP_ENUM_NO_CALLBACK,   
   /*! Error enumerating devices. */
   CVRES_VIDCAP_ENUM_ERR,           
   /*! Could not create DS device enumerator. */
   CVRES_VIDCAP_NO_ENUMERATOR,   
   /*! You must initialize the video capture first. */
   CVRES_VIDCAP_MUST_INITIALIZE_ERR,                                      
   /*! You must call Connect() to connect to a video capture device first. */
   CVRES_VIDCAP_MUST_CONNECT_ERR,
   /*! Stop() called, but no capture active. */
   CVRES_VIDCAP_STOP_BEFORE_GRABS_ERR, 
   /*! No filter graph available for capture. */
   CVRES_VIDCAP_NO_FILTER_GRAPH,                                               
   /*! Failed getting event for capture. */
   CVRES_VIDCAP_NO_CAPTURE_EVENT,     
   /*! Failed getting control for capture. */
   CVRES_VIDCAP_NO_CAPTURE_CONTROL,   
   /*! ISampleGrabber not available. */
   CVRES_VIDCAP_NO_SAMPLE_GRABBER,    
   /*! Could not add grabber for vidcap. */
   CVRES_VIDCAP_ADD_GRABBER_ERR,      
   /*! NullRenderer not available. */
   CVRES_VIDCAP_NO_NULL_RENDERER,     
   /*! VideoRenderer not available. */
   CVRES_VIDCAP_NO_VIDEO_RENDERER,     
   /*! Could not add renderer for vidcap. */
   CVRES_VIDCAP_ADD_RENDER_ERR,       
   /*! Could not retrieve interface for ISampleGrabber. */
   CVRES_VIDCAP_NO_ISAMPLEGRABBER,                                          
   /*! Could not set media type for vidcap. */
   CVRES_VIDCAP_MEDIATYPE_SET_ERR,    
   /*! Error connected to device. */
   CVRES_VIDCAP_CONNECT_ERR,          
   /*! Not Connected to device. */
   CVRES_VIDCAP_NOT_CONNECTED_ERR,    
   /*! Error starting capture. */
   CVRES_VIDCAP_START_ERR,            
   /*! Error ending capture. */
   CVRES_VIDCAP_STOP_ERR,             
   /*! Error in video format. */
   CVRES_VIDCAP_FORMAT_ERR,           
   /*! Error getting grab buffer. */
   CVRES_VIDCAP_GET_BUFFER_ERR,       
   /*! Video capture timed out! */
   CVRES_VIDCAP_TIMEOUT,              
   /*! Camera property not supported. */
   CVRES_VIDCAP_PROPERTY_NOT_SUPPORTED, 
   /*! Capture mode not supported. */
   CVRES_VIDCAP_MODE_NOT_SUPPORTED,   
   /*! Capture error occurred */
   CVRES_VIDCAP_CAPTURE_ERROR,
   /*! Capture device disconnected */
   CVRES_VIDCAP_CAPTURE_DEVICE_DISCONNECTED,
   /*! Stop() already being called */
   CVRES_VIDCAP_CAPTURE_STOP_IN_USE,
   /*! Sent to callback to indicate a stop */
   CVRES_VIDCAP_CAPTURE_STOPPING,
   /*! Timeout waiting on synch */
   CVRES_VIDCAP_SYNC_TIMEOUT,
   /*! No devices found - no enumeration possible. */
   CVRES_VIDCAP_NO_DEVICES,
   /*! Invalid device index - try refreshing the device list. */
   CVRES_VIDCAP_INVALID_DEVICE_INDEX,
   /*! Buffer for device name is too small. */
   CVRES_VIDCAP_NAME_BUFFER_TOO_SMALL,
   /*! Unable to add capture device to filter graph */
   CVRES_VIDCAP_CAPTURE_ADD_FAILED,
   /*! Unable to bind to capture device */
   CVRES_VIDCAP_CAPTURE_BIND_FAILED,
   /*! No available pin found on capture device */
   CVRES_VIDCAP_CAPTURE_NO_AVAILABLE_PIN,
   /*! Unable to connect the device to the sample grabber (incompatible?) */
   CVRES_VIDCAP_CAPTURE_GRABBER_CONNECT_FAILED,
   /*! Unable to connect the grabber to null renderer */
   CVRES_VIDCAP_GRABBER_CONNECT_FAILED,
   /*! Video is not in a supported format. */
   CVRES_VIDCAP_VIDEO_FORMAT_NOT_SUPPORTED,
   /*! Unable to query device capabilities. */
   CVRES_VIDCAP_CAPABILITY_CHECK_FAILED,

};

#endif /* _CVRESVIDCAP_H_ */

