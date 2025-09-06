
#include "qhyccderr.h"
#include "qhyccdcamdef.h"
#include "qhyccdstruct.h"
#include "stdint.h"
#include "qhyccd_config.h"
#include <functional>
#include <string>





#if defined (_WIN32)
//#include "cyapi.h"
#endif

#ifndef __QHYCCD_H__
#define __QHYCCD_H__

typedef void qhyccd_handle;

EXPORTC void STDCALL OutputQHYCCDDebug(char *strOutput);

EXPORTC void STDCALL SetQHYCCDAutoDetectCamera(bool enable);

EXPORTC void STDCALL SetQHYCCDLogLevel(uint8_t logLevel);

#if (defined(__linux__ )&&!defined (__ANDROID__)) ||(defined (__APPLE__)&&defined( __MACH__)) ||(defined(__linux__ )&&defined (__ANDROID__))

EXPORTC void STDCALL SetQHYCCDLogFunction(std::function<void(const std::string &message)> logFunction);
EXPORTC void STDCALL SetQHYCCDBufferNumber(uint32_t BufNumber);

#endif

EXPORTC void STDCALL EnableQHYCCDMessage(bool enable);
EXPORTC void STDCALL set_histogram_equalization(bool enable);
EXPORTC void STDCALL EnableQHYCCDLogFile(bool enable);


/** \fn uint32_t SetQHYCCDSingleFrameTimeOut(qhyccd_handle *h,uint32_t time)
      \brief set single frame time out 
      \param time Timeout . unit is ms. In SDK default is 60,000ms. 0 is unlimit time. Must add some spare value in order to consider the readout time
      \return
	  on success,return QHYCCD_SUCCESS \n
	  QHYCCD_ERROR_INITRESOURCE if the initialize failed \n
	  another QHYCCD_ERROR code on other failures
  */

EXPORTC uint32_t STDCALL SetQHYCCDSingleFrameTimeOut(qhyccd_handle *h,uint32_t time);  


EXPORTC const char* STDCALL GetTimeStamp();

/** \fn uint32_t InitQHYCCDResource()
      \brief initialize QHYCCD SDK resource
      \return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL InitQHYCCDResource(void);

/** \fn uint32_t ReleaseQHYCCDResource()
      \brief release QHYCCD SDK resource
      \return
	  on success,return QHYCCD_SUCCESS \n
	  QHYCCD_ERROR_RELEASERESOURCE if the release failed \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL ReleaseQHYCCDResource(void);

/** \fn uint32_t ScanQHYCCD()
      \brief scan the connected cameras
	  \return
	  on success,return the number of connected cameras \n
	  QHYCCD_ERROR_NO_DEVICE,if no camera connect to computer
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL ScanQHYCCD(void);

/** \fn uint32_t GetQHYCCDId(uint32_t index,char *id)
      \brief get the id from camera
	  \param index sequence number of the connected cameras
	  \param id the id for camera,each camera has only id
	  \return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL GetQHYCCDId(uint32_t index,char *id);

/** \fn uint32_t GetQHYCCDModel(char *id, char *model)
      \brief get camera model name by id
          \param id the id of the camera
          \param model the camera model
          \return
          on success,return QHYCCD_SUCCESS \n
          another QHYCCD_ERROR code in failure
  */
EXPORTC uint32_t STDCALL GetQHYCCDModel(char *id, char *model);

/** \fn qhyccd_handle *OpenQHYCCD(char *id)
      \brief open camera by camera id
	  \param id the id for camera,each camera has only id
      \return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC qhyccd_handle * STDCALL OpenQHYCCD(char *id);

/** \fn uint32_t CloseQHYCCD(qhyccd_handle *handle)
      \brief close camera by handle
	  \param handle camera handle
	  \return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL CloseQHYCCD(qhyccd_handle *handle);

/**
 @fn uint32_t SetQHYCCDStreamMode(qhyccd_handle *handle,uint8_t mode)
 @brief Set the camera's mode to chose the way reading data from camera
 @param handle camera control handle
 @param mode the stream mode \n
 0x00:default mode,single frame mode \n
 0x01:live mode \n
 @return
 on success,return QHYCCD_SUCCESS \n
 another QHYCCD_ERROR code on other failures
 */
EXPORTC uint32_t STDCALL SetQHYCCDStreamMode(qhyccd_handle *handle,uint8_t mode);

/** \fn uint32_t InitQHYCCD(qhyccd_handle *handle)
      \brief initialization specified camera by camera handle
	  \param handle camera control handle
      \return
	  on success,return QHYCCD_SUCCESS \n
	  on failed,return QHYCCD_ERROR_INITCAMERA \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL InitQHYCCD(qhyccd_handle *handle);

/** @fn uint32_t IsQHYCCDControlAvailable(qhyccd_handle *handle,CONTROL_ID controlId)
    @brief check the camera has the queried function or not
    @param handle camera control handle
    @param controlId function type
    @return
	  on have,return QHYCCD_SUCCESS \n
	  on do not have,return QHYCCD_ERROR_NOTSUPPORT \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL IsQHYCCDControlAvailable(qhyccd_handle *handle,CONTROL_ID controlId);

/** @fn uint32_t GetQHYCCDControlName(qhyccd_handle *handle,CONTROL_ID controlId,char *IDname)
    @brief get the specified ControlName from camera
    @param handle camera control handle
    @param controlId function type
	@param IDname the name of current controlId
    @return
	  on have,return QHYCCD_SUCCESS \n
	  on do not have,return QHYCCD_ERROR_NOTSUPPORT \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL GetQHYCCDControlName(qhyccd_handle *handle,CONTROL_ID controlId,char *IDname);

/** \fn uint32_t SetQHYCCDParam(qhyccd_handle *handle,CONTROL_ID controlId,double value)
      \brief set params to camera
      \param handle camera control handle
      \param controlId function type
	  \param value value to camera
	  \return
	  on success,return QHYCCD_SUCCESS \n
	  QHYCCD_ERROR_NOTSUPPORT,if the camera do not have the function \n
	  QHYCCD_ERROR_SETPARAMS,if set params to camera failed \n
	  another QHYCCD_ERROR code on other failures
  */

EXPORTC uint32_t STDCALL SetQHYCCDParam(qhyccd_handle *handle,CONTROL_ID controlId, double value);

/** \fn double GetQHYCCDParam(qhyccd_handle *handle,CONTROL_ID controlId)
      \brief get the params value from camera
      \param handle camera control handle
      \param controlId function type
	  \return
	  on success,return the value\n
	  QHYCCD_ERROR_NOTSUPPORT,if the camera do not have the function \n
	  QHYCCD_ERROR_GETPARAMS,if get camera params'value failed \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC double STDCALL GetQHYCCDParam(qhyccd_handle *handle,CONTROL_ID controlId);

/** \fn uint32_t GetQHYCCDParamMinMaxStep(qhyccd_handle *handle,CONTROL_ID controlId,double *min,double *max,double *step)
      \brief get the params value from camera
      \param handle camera control handle
      \param controlId function type
	  \param *min the pointer to the function's min value
	  \param *max the pointer to the function's max value
	  \param *step the pointer to the function's single step value
	  \return
	  on success,return QHYCCD_SUCCESS\n
	  QHYCCD_ERROR_NOTSUPPORT,if the camera do not have the function \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL GetQHYCCDParamMinMaxStep(qhyccd_handle *handle,CONTROL_ID controlId,double *min,double *max,double *step);

/** @fn uint32_t SetQHYCCDResolution(qhyccd_handle *handle,uint32_t x,uint32_t y,uint32_t xsize,uint32_t ysize)
    @brief set camera ouput resolution
    @param handle camera control handle
    @param x the top left position x
    @param y the top left position y
    @param xsize the image width
    @param ysize the image height
    @return
        on success,return QHYCCD_SUCCESS\n
        another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL SetQHYCCDResolution(qhyccd_handle *handle,uint32_t x,uint32_t y,uint32_t xsize,uint32_t ysize);

/** \fn uint32_t GetQHYCCDMemLength(qhyccd_handle *handle)
      \brief get the minimum memory space for image data to save(byte)
      \param handle camera control handle
      \return
	  on success,return the total memory space for image data(byte) \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL GetQHYCCDMemLength(qhyccd_handle *handle);

/** \fn uint32_t ExpQHYCCDSingleFrame(qhyccd_handle *handle)
      \brief start to expose one frame
      \param handle camera control handle
      \return
	  on success,return QHYCCD_SUCCESS \n
	  QHYCCD_ERROR_EXPOSING,if the camera is exposing \n
	  QHYCCD_ERROR_EXPFAILED,if start failed \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL ExpQHYCCDSingleFrame(qhyccd_handle *handle);

/**
   @fn uint32_t GetQHYCCDSingleFrame(qhyccd_handle *handle,uint32_t *w,uint32_t *h,uint32_t *bpp,uint32_t *channels,uint8_t *imgdata)
   @brief get live frame data from camera
   @param handle camera control handle
   @param *w pointer to width of ouput image
   @param *h pointer to height of ouput image
   @param *bpp pointer to depth of ouput image
   @param *channels pointer to channels of ouput image
   @param *imgdata image data buffer
   @return
   on success,return QHYCCD_SUCCESS \n
   QHYCCD_ERROR_GETTINGFAILED,if get data failed \n
   another QHYCCD_ERROR code on other failures
 */
EXPORTC uint32_t STDCALL GetQHYCCDSingleFrame(qhyccd_handle *handle,uint32_t *w,uint32_t *h,uint32_t *bpp,uint32_t *channels,uint8_t *imgdata);

/**
  @fn uint32_t CancelQHYCCDExposing(qhyccd_handle *handle)
  @brief force stop the camera long exposure. But host software must readout the image data. Please note not all camera can use this method.
  @param handle camera control handle
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL CancelQHYCCDExposing(qhyccd_handle *handle);

/**
  @fn uint32_t CancelQHYCCDExposingAndReadout(qhyccd_handle *handle)
  @brief force stop the camera long exposure. And also camera does not send back the image data. Host software must not readout the data. All camera support this mode. 
  @param handle camera control handle
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL CancelQHYCCDExposingAndReadout(qhyccd_handle *handle);

/** 
  @fn uint32_t BeginQHYCCDLive(qhyccd_handle *handle)
  @brief in live video mode, start continue exposing. Only need to start once before StopQHYCCDLive.
	@param handle camera control handle
  @return
	on success,return QHYCCD_SUCCESS \n
	another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL BeginQHYCCDLive(qhyccd_handle *handle);

/**
    @fn uint32_t GetQHYCCDLiveFrame(qhyccd_handle *handle,uint32_t *w,uint32_t *h,uint32_t *bpp,uint32_t *channels,uint8_t *imgdata)
    @brief get live frame data from camera
	  @param handle camera control handle
	  @param *w pointer to width of ouput image
	  @param *h pointer to height of ouput image
    @param *bpp pointer to depth of ouput image
    @param *channels pointer to channels of ouput image
    @param *imgdata image data buffer
	  @return
	  on success,return QHYCCD_SUCCESS \n
	  QHYCCD_ERROR_GETTINGFAILED,if get data failed \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL GetQHYCCDLiveFrame(qhyccd_handle *handle,uint32_t *w,uint32_t *h,uint32_t *bpp,uint32_t *channels,uint8_t *imgdata);

/** \fn uint32_t StopQHYCCDLive(qhyccd_handle *handle)
      \brief stop the camera continue exposing
	  \param handle camera control handle
	  \return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL StopQHYCCDLive(qhyccd_handle *handle);

/** \
  @fn uint32_t SetQHYCCDBinMode(qhyccd_handle *handle,uint32_t wbin,uint32_t hbin)
  @brief set camera's bin mode for ouput image data
  @param handle camera control handle
  @param wbin width bin mode
  @param hbin height bin mode
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
*/




EXPORTFUNC uint32_t STDCALL QHYCCDPcieRecv(qhyccd_handle *handle, void * data, int len,uint64_t timeout);
EXPORTFUNC uint32_t STDCALL GetQHYCCDPcieDDRNum(qhyccd_handle *handle);



EXPORTC uint32_t STDCALL SetQHYCCDBinMode(qhyccd_handle *handle,uint32_t wbin,uint32_t hbin);

/**
   @fn uint32_t SetQHYCCDBitsMode(qhyccd_handle *handle,uint32_t bits)
   @brief set camera's depth bits for ouput image data
   @param handle camera control handle
   @param bits image depth
   @return
   on success,return QHYCCD_SUCCESS \n
   another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL SetQHYCCDBitsMode(qhyccd_handle *handle,uint32_t bits);

/** \fn uint32_t ControlQHYCCDTemp(qhyccd_handle *handle,double targettemp)
      \brief This is a auto temprature control for QHYCCD cameras. \n
             To control this,you need call this api every single second
          \param handle camera control handle
          \param targettemp the target control temprature
          \return
          on success,return QHYCCD_SUCCESS \n
          another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL ControlQHYCCDTemp(qhyccd_handle *handle,double targettemp);

/** \fn uint32_t ControlQHYCCDGuide(qhyccd_handle *handle,uint32_t direction,uint16_t duration)
      \brief control the camera' guide port
	  \param handle camera control handle
	  \param direction direction \n
           0: EAST RA+   \n
           3: WEST RA-   \n
           1: NORTH DEC+ \n
           2: SOUTH DEC- \n
	  \param duration duration of the direction
	  \return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL ControlQHYCCDGuide(qhyccd_handle *handle,uint32_t direction,uint16_t duration);

/**
 @fn uint32_t SendOrder2QHYCCDCFW(qhyccd_handle *handle,char *order,uint32_t length)
    @brief control color filter wheel port
    @param handle camera control handle
 @param order order send to color filter wheel
 @param length the order string length
 @return
 on success,return QHYCCD_SUCCESS \n
 another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL SendOrder2QHYCCDCFW(qhyccd_handle *handle,char *order,uint32_t length);

/**
 @fn 	uint32_t GetQHYCCDCFWStatus(qhyccd_handle *handle,char *status)
    @brief control color filter wheel port
    @param handle camera control handle
 @param status the color filter wheel position status
 @return
 on success,return QHYCCD_SUCCESS \n
 another QHYCCD_ERROR code on other failures
  */
EXPORTC	uint32_t STDCALL GetQHYCCDCFWStatus(qhyccd_handle *handle,char *status);

/**
 @fn 	uint32_t IsQHYCCDCFWPlugged(qhyccd_handle *handle)
    @brief control color filter wheel port
    @param handle camera control handle
 @return
 on success,return QHYCCD_SUCCESS \n
 another QHYCCD_ERROR code on other failures
  */
EXPORTC	uint32_t STDCALL IsQHYCCDCFWPlugged(qhyccd_handle *handle);

//Get the number of triger mode for camera
EXPORTC uint32_t STDCALL GetQHYCCDTrigerInterfaceNumber(qhyccd_handle *handle, uint32_t *modeNumber);
//Get the name of for every triger mode 
EXPORTC uint32_t STDCALL GetQHYCCDTrigerInterfaceName(qhyccd_handle *handle, uint32_t modeNumber, char *name);
//Setup triger interface
EXPORTC uint32_t STDCALL SetQHYCCDTrigerInterface(qhyccd_handle *handle, uint32_t trigerMode);
//Setup triger-in mode on/off
EXPORTC uint32_t STDCALL SetQHYCCDTrigerFunction(qhyccd_handle *h,bool value);
/**
 \fn   uint32_t SetQHYCCDTrigerMode(qhyccd_handle *handle,uint32_t trigerMode)
 \brief set camera triger mode
 \param handle camera control handle
 \param trigerMode triger mode
 \return
on success,return QHYCCD_SUCCESS \n
another QHYCCD_ERROR code on other failures
*/
//Setup triger mode for camera
EXPORTC uint32_t STDCALL SetQHYCCDTrigerMode(qhyccd_handle *handle,uint32_t trigerMode);
//Setup triger-out mode on/off
EXPORTC uint32_t STDCALL EnableQHYCCDTrigerOut(qhyccd_handle *handle);
EXPORTC uint32_t STDCALL EnableQHYCCDTrigerOutA(qhyccd_handle *handle);
//Send software triger-in signal to camera
EXPORTC uint32_t STDCALL SendSoftTriger2QHYCCDCam(qhyccd_handle *handle);

EXPORTC uint32_t STDCALL SetQHYCCDTrigerFilterOnOff(qhyccd_handle *handle, bool onoff);

EXPORTC uint32_t STDCALL SetQHYCCDTrigerFilterTime(qhyccd_handle *handle, uint32_t time);


/** \fn void Bits16ToBits8(qhyccd_handle *h,uint8_t *InputData16,uint8_t *OutputData8,uint32_t imageX,uint32_t imageY,uint16_t B,uint16_t W)
      \brief turn 16bits data into 8bits
      \param h camera control handle
      \param InputData16 for 16bits data memory
      \param OutputData8 for 8bits data memory
      \param imageX image width
      \param imageY image height
      \param B for stretch balck
      \param W for stretch white
  */
EXPORTC void STDCALL Bits16ToBits8(qhyccd_handle *h,uint8_t *InputData16,uint8_t *OutputData8,uint32_t imageX,uint32_t imageY,uint16_t B,uint16_t W);

/**
   @fn void HistInfo192x130(qhyccd_handle *h,uint32_t x,uint32_t y,uint8_t *InBuf,uint8_t *OutBuf)
   @brief make the hist info
   @param h camera control handle
   @param x image width
   @param y image height
   @param InBuf for the raw image data
   @param OutBuf for 192x130 8bits 3 channels image
  */
EXPORTC void  STDCALL HistInfo192x130(qhyccd_handle *h,uint32_t x,uint32_t y,uint8_t *InBuf,uint8_t *OutBuf);


/**
    @fn uint32_t OSXInitQHYCCDFirmware(char *path)
    @brief download the firmware to camera.(this api just need call in OSX system)
    @param path path to HEX file
  */
EXPORTC uint32_t STDCALL OSXInitQHYCCDFirmware(char *path);

/**
    @fn uint32_t OSXInitQHYCCDFirmware(char *path)
    @brief download the firmware to camera.(this api just need call in OSX system)
    @param path path to HEX file
  */
EXPORTC uint32_t STDCALL OSXInitQHYCCDFirmwareArray();



EXPORTC uint32_t STDCALL OSXInitQHYCCDAndroidFirmwareArray(int idVendor,int idProduct,
    qhyccd_handle *handle);



/** @fn uint32_t GetQHYCCDChipInfo(qhyccd_handle *h,double *chipw,double *chiph,uint32_t *imagew,uint32_t *imageh,double *pixelw,double *pixelh,uint32_t *bpp)
      @brief get the camera's ccd/cmos chip info
      @param h camera control handle
      @param chipw chip size width
      @param chiph chip size height
      @param imagew chip output image width
      @param imageh chip output image height
      @param pixelw chip pixel size width
      @param pixelh chip pixel size height
      @param bpp chip pixel depth
  */
EXPORTC uint32_t STDCALL GetQHYCCDChipInfo(qhyccd_handle *h,double *chipw,double *chiph,uint32_t *imagew,uint32_t *imageh,double *pixelw,double *pixelh,uint32_t *bpp);

/** @fn uint32_t GetQHYCCDEffectiveArea(qhyccd_handle *h,uint32_t *startX, uint32_t *startY, uint32_t *sizeX, uint32_t *sizeY)
      @brief get the camera's ccd/cmos chip info
      @param h camera control handle
      @param startX the Effective area x position
      @param startY the Effective area y position
      @param sizeX the Effective area x size
      @param sizeY the Effective area y size
	  @return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL GetQHYCCDEffectiveArea(qhyccd_handle *h,uint32_t *startX, uint32_t *startY, uint32_t *sizeX, uint32_t *sizeY);

/** @fn uint32_t GetQHYCCDOverScanArea(qhyccd_handle *h,uint32_t *startX, uint32_t *startY, uint32_t *sizeX, uint32_t *sizeY)
      @brief get the camera's ccd/cmos chip info
      @param h camera control handle
      @param startX the OverScan area x position
      @param startY the OverScan area y position
      @param sizeX the OverScan area x size
      @param sizeY the OverScan area y size
	  @return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL GetQHYCCDOverScanArea(qhyccd_handle *h,uint32_t *startX, uint32_t *startY, uint32_t *sizeX, uint32_t *sizeY);

EXPORTC uint32_t STDCALL GetQHYCCDCurrentROI(qhyccd_handle *handle, uint32_t *startX, uint32_t *startY, uint32_t *sizeX, uint32_t *sizeY);

/** @fn uint32_t GetQHYCCDImageStabilizationGravity(qhyccd_handle *h,uint32_t *startX, uint32_t *startY)
      @brief The position coordinate of the target center of gravity in the chip in image stabilization
      @param h camera control handle
      @param  Gravity CenterX of the chipsize
      @param  Gravity CenterY of the chipsize
	  @return
	  on success,return QHYCCD_SUCCESS \n
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL GetQHYCCDImageStabilizationGravity(qhyccd_handle *handle, int *GravityX, int *GravityY);

/** @fn uint32_t SetQHYCCDFocusSetting(qhyccd_handle *h,uint32_t focusCenterX, uint32_t focusCenterY)
      @brief Set the camera on focus mode
      @param h camera control handle
      @param focusCenterX
      @param focusCenterY
      @return
	  on success,return QHYCCD_SUCCESS \n
 
	  another QHYCCD_ERROR code on other failures
  */
EXPORTC uint32_t STDCALL SetQHYCCDFocusSetting(qhyccd_handle *h,uint32_t focusCenterX, uint32_t focusCenterY);

/** @fn uint32_t GetQHYCCDExposureRemaining(qhyccd_handle *h)
      @brief Get remaining ccd/cmos expose time
      @param h camera control handle
      @return
      100 or less 100,it means exposoure is over \n
      another is remaining time
 */
EXPORTC uint32_t STDCALL GetQHYCCDExposureRemaining(qhyccd_handle *h);

/** @fn uint32_t GetQHYCCDFWVersion(qhyccd_handle *h,uint8_t *buf)
      @brief Get the QHYCCD's firmware version
      @param h camera control handle
	  @param buf buffer for version info
      @return
	  on success,return QHYCCD_SUCCESS \n
 
	  another QHYCCD_ERROR code on other failures
 */
EXPORTC uint32_t STDCALL GetQHYCCDFWVersion(qhyccd_handle *h,uint8_t *buf);
EXPORTC uint32_t STDCALL GetQHYCCDFPGAVersion(qhyccd_handle *h, uint8_t fpga_index, uint8_t *buf);

/** @fn uint32_t SetQHYCCDInterCamSerialParam(qhyccd_handle *h,uint32_t opt)
      @brief Set InterCam serial2 params
      @param h camera control handle
	  @param opt the param \n
	  opt: \n
	   0x00 baud rate 9600bps  8N1 \n
       0x01 baud rate 4800bps  8N1 \n
       0x02 baud rate 19200bps 8N1 \n
       0x03 baud rate 28800bps 8N1 \n
       0x04 baud rate 57600bps 8N1
      @return
	  on success,return QHYCCD_SUCCESS \n
 
	  another QHYCCD_ERROR code on other failures
 */
EXPORTC uint32_t STDCALL SetQHYCCDInterCamSerialParam(qhyccd_handle *h,uint32_t opt);

/** @fn uint32_t QHYCCDInterCamSerialTX(qhyccd_handle *h,char *buf,uint32_t length)
      @brief Send data to InterCam serial2
      @param h camera control handle
	  @param buf buffer for data
	  @param length to send
      @return
	  on success,return QHYCCD_SUCCESS \n
 
	  another QHYCCD_ERROR code on other failures
 */
EXPORTC uint32_t STDCALL QHYCCDInterCamSerialTX(qhyccd_handle *h,char *buf,uint32_t length);

/** @fn uint32_t QHYCCDInterCamSerialRX(qhyccd_handle *h,char *buf)
      @brief Get data from InterCam serial2
      @param h camera control handle
	  @param buf buffer for data
      @return
	  on success,return the data number \n
 
	  another QHYCCD_ERROR code on other failures
 */
EXPORTC uint32_t STDCALL QHYCCDInterCamSerialRX(qhyccd_handle *h,char *buf);

/** @fn uint32_t QHYCCDInterCamOledOnOff(qhyccd_handle *handle,uint8_t onoff)
     @brief turn off or turn on the InterCam's Oled
     @param handle camera control handle
  @param onoff on or off the oled \n
  1:on \n
  0:off \n
     @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
   */
EXPORTC uint32_t STDCALL QHYCCDInterCamOledOnOff(qhyccd_handle *handle,uint8_t onoff);

/**
  @fn uint32_t SetQHYCCDInterCamOledBrightness(qhyccd_handle *handle,uint8_t brightness)
  @brief send data to show on InterCam's OLED
  @param handle camera control handle
  @param brightness the oled's brightness
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
*/
EXPORTC uint32_t STDCALL SetQHYCCDInterCamOledBrightness(qhyccd_handle *handle,uint8_t brightness);

/**
  @fn uint32_t SendFourLine2QHYCCDInterCamOled(qhyccd_handle *handle,char *messagetemp,char *messageinfo,char *messagetime,char *messagemode)
  @brief spilit the message to two line,send to camera
  @param handle camera control handle
  @param messagetemp message for the oled's 1st line
  @param messageinfo message for the oled's 2nd line
  @param messagetime message for the oled's 3rd line
  @param messagemode message for the oled's 4th line
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
*/
EXPORTC uint32_t STDCALL SendFourLine2QHYCCDInterCamOled(qhyccd_handle *handle,char *messagetemp,char *messageinfo,char *messagetime,char *messagemode);
/**
  @fn uint32_t SendTwoLine2QHYCCDInterCamOled(qhyccd_handle *handle,char *messageTop,char *messageBottom)
  @brief spilit the message to two line,send to camera
  @param handle camera control handle
  @param messageTop message for the oled's 1st line
  @param messageBottom message for the oled's 2nd line
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
*/
EXPORTC uint32_t STDCALL SendTwoLine2QHYCCDInterCamOled(qhyccd_handle *handle,char *messageTop,char *messageBottom);

/**
  @fn uint32_t SendOneLine2QHYCCDInterCamOled(qhyccd_handle *handle,char *messageTop)
  @brief spilit the message to two line,send to camera
  @param handle camera control handle
  @param messageTop message for all the oled
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
*/
EXPORTC uint32_t STDCALL SendOneLine2QHYCCDInterCamOled(qhyccd_handle *handle,char *messageTop);

/**
  @fn uint32_t GetQHYCCDCameraStatus(qhyccd_handle *h,uint8_t *buf)
  @brief Get the camera statu
  @param h camera control handle
  @param buf camera's status save space
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
 */
EXPORTC uint32_t STDCALL GetQHYCCDCameraStatus(qhyccd_handle *h,uint8_t *buf);

/**
 @fn uint32_t GetQHYCCDShutterStatus(qhyccd_handle *handle)
 @brief get the camera's shutter status
 @param handle camera control handle
 @return
 on success,return status \n
 0x00:shutter turn to right \n
 0x01:shutter from right turn to middle \n
 0x02:shutter from left turn to middle \n
 0x03:shutter turn to left \n
 0xff:IDLE \n
 another QHYCCD_ERROR code on other failures
*/
EXPORTC uint32_t STDCALL GetQHYCCDShutterStatus(qhyccd_handle *handle);

/**
  @fn uint32_t ControlQHYCCDShutter(qhyccd_handle *handle,uint8_t status)
  @brief control camera's shutter
  @param handle camera control handle
  @param status the shutter status \n
  0x00:shutter turn to right \n
  0x01:shutter from right turn to middle \n
  0x02:shutter from left turn to middle \n
  0x03:shutter turn to left \n
  0xff:IDLE
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
*/
EXPORTC uint32_t STDCALL ControlQHYCCDShutter(qhyccd_handle *handle,uint8_t status);

/**
  @fn uint32_t GetQHYCCDHumidity(qhyccd_handle *handle,double *hd)
  @brief query cavity's humidity
  @param handle control handle
  @param hd the humidity value
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
*/
EXPORTC uint32_t STDCALL GetQHYCCDPressure(qhyccd_handle *handle,double *pressure);
/**
  @fn uint32_t GetQHYCCDPressure(qhyccd_handle *handle,double *pressure)
  @get the pressure of sensor chamber
  @param handle control handle
  @param pressure : the sensor chamber pressure . unit is mbar  range 0.0-2000.0
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
*/

EXPORTC uint32_t STDCALL GetQHYCCDHumidity(qhyccd_handle *handle,double *hd);


/**
  @fn uint32_t QHYCCDI2CTwoWrite(qhyccd_handle *handle,uint16_t addr,uint16_t value)
  @brief Set the value of the addr register in the camera.
  @param handle camera control handle
  @param addr the address of register
  @param value the value of the address
  @return
  on success,return QHYCCD_SUCCESS \n
  another QHYCCD_ERROR code on other failures
*/
EXPORTC uint32_t STDCALL QHYCCDI2CTwoWrite(qhyccd_handle *handle,uint16_t addr,uint16_t value);

/**
  @fn uint32_t QHYCCDI2CTwoRead(qhyccd_handle *handle,uint16_t addr)
  @brief Get the value of the addr register in the camera.
  @param handle camera control handle
  @param addr the address of register
  @return value of the addr register
*/
EXPORTC uint32_t STDCALL QHYCCDI2CTwoRead(qhyccd_handle *handle,uint16_t addr);

/**
  @fn double GetQHYCCDReadingProgress(qhyccd_handle *handle)
  @brief get reading data from camera progress
  @param handle camera control handle
  @return current progress
*/
EXPORTC double STDCALL GetQHYCCDReadingProgress(qhyccd_handle *handle);


/**
  test pid parameters
*/
EXPORTC uint32_t STDCALL TestQHYCCDPIDParas(qhyccd_handle *h, double p, double i, double d);

EXPORTC uint32_t STDCALL DownloadFX3FirmWare(uint16_t vid,uint16_t pid,char *imgpath);

EXPORTC uint32_t STDCALL GetQHYCCDType(qhyccd_handle *h);

EXPORTC uint32_t STDCALL SetQHYCCDDebayerOnOff(qhyccd_handle *h,bool onoff);

EXPORTC uint32_t STDCALL SetQHYCCDFineTone(qhyccd_handle *h,uint8_t setshporshd,uint8_t shdloc,uint8_t shploc,uint8_t shwidth);

EXPORTC uint32_t STDCALL SetQHYCCDGPSVCOXFreq(qhyccd_handle *handle,uint16_t i);

EXPORTC uint32_t STDCALL SetQHYCCDGPSLedCalMode(qhyccd_handle *handle,uint8_t i);

EXPORTC void STDCALL SetQHYCCDGPSLedCal(qhyccd_handle *handle,uint32_t pos,uint8_t width);

EXPORTC void STDCALL SetQHYCCDGPSPOSA(qhyccd_handle *handle,uint8_t is_slave,uint32_t pos,uint8_t width);

EXPORTC void STDCALL SetQHYCCDGPSPOSB(qhyccd_handle *handle,uint8_t is_slave,uint32_t pos,uint8_t width);

EXPORTC uint32_t STDCALL SetQHYCCDGPSMasterSlave(qhyccd_handle *handle,uint8_t i);

EXPORTC void STDCALL SetQHYCCDGPSSlaveModeParameter(qhyccd_handle *handle,uint32_t target_sec,uint32_t target_us,uint32_t deltaT_sec,uint32_t deltaT_us,uint32_t expTime);

EXPORTC void STDCALL SetQHYCCDQuit();

EXPORTC uint32_t STDCALL QHYCCDVendRequestWrite(qhyccd_handle *h, uint8_t req, uint16_t value, uint16_t index1, uint32_t length, uint8_t *data);
EXPORTC uint32_t STDCALL QHYCCDVendRequestRead(qhyccd_handle *h, uint8_t req, uint16_t value, uint16_t index1, uint32_t length, uint8_t *data);

EXPORTC uint32_t STDCALL QHYCCDReadUSB_SYNC(qhyccd_handle *pDevHandle, uint8_t endpoint, uint32_t length, uint8_t *data, uint32_t timeout);

EXPORTC uint32_t STDCALL QHYCCDLibusbBulkTransfer(qhyccd_handle *pDevHandle, uint8_t endpoint, uint8_t *data, uint32_t length, int32_t *transferred, uint32_t timeout);

EXPORTC uint32_t STDCALL GetQHYCCDSDKVersion(uint32_t *year,uint32_t *month,uint32_t *day,uint32_t *subday);





//APIs for the Readout Mode. One camera may have more than one readout mode. The different readout mode has different base-resolution. For example
//The QHY42PRO support HDR and STD mode. HDR mode base-resolution is 4096*2048. While the STD mode is 2048*2048. In this case we need to use the
//readout mode to set it. The host application need to get the readout mode and select one to set it. The sequece that call this fucntion need to be(......)


EXPORTC uint32_t STDCALL GetQHYCCDNumberOfReadModes(qhyccd_handle *h,uint32_t *numModes);
// Get the maximum resolution for a read mode
EXPORTC uint32_t STDCALL GetQHYCCDReadModeResolution(qhyccd_handle *h,uint32_t modeNumber, uint32_t* width, uint32_t* height);
// Get the name of a read mode
EXPORTC uint32_t STDCALL GetQHYCCDReadModeName(qhyccd_handle *h,uint32_t modeNumber, char* name);
// Set the read mode
EXPORTC uint32_t STDCALL SetQHYCCDReadMode(qhyccd_handle *h,uint32_t modeNumber);
// Get the read mode
EXPORTC uint32_t STDCALL GetQHYCCDReadMode(qhyccd_handle *h,uint32_t* modeNumber);

EXPORTC uint32_t STDCALL GetQHYCCDBeforeOpenParam(
  QHYCamMinMaxStepValue *p,
  CONTROL_ID controlId);
/*
EXPORTC uint32_t STDCALL GetQHYCCDBeforeOpenReadMode(QHYCamReadModeInfo *p);
*/
EXPORTC uint32_t STDCALL EnableQHYCCDBurstMode(qhyccd_handle *h,bool i);
EXPORTC uint32_t STDCALL SetQHYCCDBurstModeStartEnd(qhyccd_handle *h,unsigned short start,unsigned short end);
EXPORTC uint32_t STDCALL EnableQHYCCDBurstCountFun(qhyccd_handle *h,bool i);
EXPORTC uint32_t STDCALL ResetQHYCCDFrameCounter(qhyccd_handle *h);
EXPORTC uint32_t STDCALL SetQHYCCDBurstIDLE(qhyccd_handle *h);
EXPORTC uint32_t STDCALL ReleaseQHYCCDBurstIDLE(qhyccd_handle *h);
EXPORTC uint32_t STDCALL SetQHYCCDBurstModePatchNumber(qhyccd_handle *h,uint32_t value);
EXPORTC uint32_t STDCALL SetQHYCCDEnableLiveModeAntiRBI(qhyccd_handle *h,uint32_t value);
EXPORTC uint32_t STDCALL SetQHYCCDWriteFPGA(qhyccd_handle *h,uint8_t number,uint8_t regindex,uint8_t regvalue);
/**
  @fn uint32_t SetQHYCCDWriteFPGA(qhyccd_handle *h,uint8_t number,uint8_t regindex,uint8_t regvalue);
  @brief Write FPGA register of the camera directly for advanced control
  @param handle camera control handle
  @param number:  if there is multiple FPGA, this is the sequence number . default is 0. 
  @param regindex:  register index. It is 8bit.
  @param regindex:  register value. It is 8bit.
  @return QHYCCD_SUCCESS or QHYCCD_ERROR. If it is QHYCCD_ERROR, it means (1) this model may have not support this function or (2) the API failur to run.
*/


EXPORTC uint32_t STDCALL SetQHYCCDWriteCMOS(qhyccd_handle *h,uint8_t number,uint16_t regindex,uint16_t regvalue);
/**
  @fn uint32_t SetQHYCCDWriteCMOS(qhyccd_handle *h,uint8_t number,uint16_t regindex,uint16_t regvalue);
  @brief Write CMOS register of the camera directly for advanced control
  @param handle camera control handle
  @param number:  if there is multiple CMOS, this is the sequence number . default is 0. 
  @param regindex:  register index. It is 16bit.
  @param regindex:  register value. It is 16bit.
  @return QHYCCD_SUCCESS or QHYCCD_ERROR. If it is QHYCCD_ERROR, it means (1) this model may have not support this function or (2) the API failur to run.
*/


EXPORTC uint32_t STDCALL SetQHYCCDTwoChannelCombineParameter(qhyccd_handle *handle, double x,double ah,double bh,double al,double bl);
/**
  @fn uint32_t SetQHYCCDTwoChannelCombineParameter(qhyccd_handle *handle, double x,double ah,double bh,double al,double bl);
  @brief For the camera with high gain low gain two channel combine to 16bit function, this API can set the combination parameters
  @param handle camera control handle
  @param x:  High gain low gain channel data switch point. (based on the high gain channel data)
  @param ah: High gain channel ratio   (y=ax+b)
  @param bh: High gain channel offset  (y=ax+b)
  @param al: Low gain channel ratio    (y=ax+b)
  @param bl: Low gain channel offset   (y=ax+b)
  @return QHYCCD_SUCCESS or QHYCCD_ERROR. If it is QHYCCD_ERROR, it means (1) this model may have not support this function or (2) the API failur to run.
*/

EXPORTC uint32_t STDCALL EnableQHYCCDImageOSD(qhyccd_handle *h,uint32_t i);

/**
  @fn uint32_t GetQHYCCDPreciseExposureInfo(qhyccd_handle *h,
                                            uint32_t *PixelPeriod_ps,
                                            uint32_t *LinePeriod_ns,
                                            uint32_t *FramePeriod_us,
                                            uint32_t *ClocksPerLine,
                                            uint32_t *LinesPerFrame,
                                            uint32_t *ActualExposureTime,
                                            uint8_t  *isLongExposureMode);
  @brief get the sensor precise timing data from camera. These data can be used for high precise GPS time calculation 
  @param h camera control handle
  @param PixelPeriod_ps return pixel period, unit is ps \n
  @param LinePeriod_ns return row period, unit is ns \n
  @param FramePeriod_us return frame period, unit is us \n 
  @param ClocksPerLine return how many clocks per line \n 
  @param LinesPerFrame return how many rows per frame. Please note this maybe not the picture y size. \n   
  @param ActualExposureTime return actual exposure time. most cmos exposure is row based. So the exposure time is n*row period. It maybe has a little difference with the set value. \n   
  @param isLongExposureMode return if camera works in long exposure mode. For cmos camera. When exposure time > frame period. It will add the verical blanking rows. in this case it is long exposure mode \n     
  @return QHYCCD_SUCCESS or QHYCCD_ERROR. If the camera does not support this function, it will return QHYCCD_ERROR \n

*/
EXPORTC uint32_t STDCALL GetQHYCCDPreciseExposureInfo(qhyccd_handle *h,
                                                         uint32_t *PixelPeriod_ps,
                                                         uint32_t *LinePeriod_ns,
                                                         uint32_t *FramePeriod_us,
                                                         uint32_t *ClocksPerLine,
                                                         uint32_t *LinesPerFrame,
                                                         uint32_t *ActualExposureTime,
                                                         uint8_t  *isLongExposureMode);




/**
  @fn uint32_t GetQHYCCDRollingShutterEndOffset(qhyccd_handle *h,uint32_t row,uint32_t *offset);   
  @brief for rolling shutter camera with GPS meassurement signal output or with GPSBOX connection. it will output the meassurement pulse. But the pulse is not at the exactly time of the \n
         the exposure time. This api will return the calibrated data of the offset value from GPS meassurement pulse to the end of exposure time of certain row. 
  @param h camera control handle \n
  @param row the shutter status \n
  @param offset the shutter offset value from shutter meassure signal falling edge (gps messured end exposure) . unit us \n
  @return QHYCCD_SUCCESS or QHYCCD_ERROR. If the camera does not support this function, it will return QHYCCD_ERROR \n
*/
EXPORTFUNC uint32_t STDCALL GetQHYCCDRollingShutterEndOffset(qhyccd_handle *h,uint32_t row,double *offset);                                                        


EXPORTC void STDCALL QHYCCDQuit();

EXPORTC QHYDWORD STDCALL SetQHYCCDCallBack(QHYCCDProcCallBack ProcCallBack,
    int32_t Flag);

EXPORTFUNC void RegisterPnpEventIn( void (*in_pnp_event_in_func)(char *id));

EXPORTFUNC void RegisterPnpEventOut( void (*in_pnp_event_out_func)(char *id));


EXPORTFUNC uint32_t STDCALL resetDev(char *deviceID, uint32_t readModeIndex, uint8_t streamMode,qhyccd_handle* devHandle, uint32_t* imageWidth, uint32_t* imageHigh, uint32_t bitDepth);

EXPORTFUNC void RegisterDataEventSingle( void (*in_data_event_single_func)(char *id, uint8_t *imgdata));

EXPORTFUNC void RegisterDataEventLive( void (*in_data_event_live_func)(char *id, uint8_t *imgdata));

EXPORTFUNC void RegisterTransferEventError( void (*transfer_event_error_func)());

EXPORTFUNC uint32_t STDCALL PCIEClearDDR(qhyccd_handle *handle);
EXPORTFUNC uint32_t STDCALL GetReadModesNumber(char* deviceID,uint32_t* numModes);

EXPORTFUNC uint32_t STDCALL GetReadModeName(char* deviceID, uint32_t modeIndex, char* modeName);

EXPORTFUNC void STDCALL QHYCCDSensorPhaseReTrain(qhyccd_handle *handle);
EXPORTFUNC void STDCALL QHYCCDReadInitConfigFlash(qhyccd_handle *handle, char* configString_raw64);
EXPORTFUNC void STDCALL QHYCCDEraseInitConfigFlash(qhyccd_handle *handle);
EXPORTFUNC void STDCALL QHYCCDResetFlashULVOError(qhyccd_handle *handle);
EXPORTFUNC void STDCALL QHYCCDTestFlashULVOError(qhyccd_handle *handle);
EXPORTFUNC void STDCALL QHYCCDSetFlashInitPWM(qhyccd_handle *handle,uint8_t pwm);
EXPORTFUNC void STDCALL QHYCCDGetDebugDataD3(qhyccd_handle *handle, char* debugData_raw64);
EXPORTFUNC uint32_t STDCALL QHYCCDSolve(int timeout_s, float scale_l, float  scale_h,float center_ra, float center_dec,float center_r, float& s_ra, float& s_dec,float& s_size_x,float& s_size_y, float& s_rotation);
EXPORTFUNC void STDCALL QHYCCDEqualizeHistogram(uint8_t * pdata, int width, int height, int bpp);
void  QHYCCDGetDebugControlID(CONTROL_ID controlId, bool hasValue, bool isSetValue, double value);


EXPORTFUNC int STDCALL QHYCCD_fpga_list(struct fpga_info_list &list);
EXPORTFUNC uint32_t STDCALL QHYCCD_fpga_open(int id);
EXPORTFUNC void STDCALL QHYCCD_fpga_close();
EXPORTFUNC int STDCALL QHYCCD_fpga_send(int chnl, void * data, int len, int destoff, int last, uint64_t timeout);
EXPORTFUNC int STDCALL QHYCCD_fpga_recv(int chnl, void * data, int len, uint64_t timeout);
EXPORTFUNC void STDCALL QHYCCD_fpga_reset();

EXPORTFUNC uint32_t STDCALL SetQHYCCDLoadCalibrationFrames(qhyccd_handle *handle, uint32_t ImgW, uint32_t ImgH, uint32_t ImgBits, uint32_t ImgChannel, char *DarkFile, char *FlatFile, char *BiasFile);
EXPORTFUNC uint32_t STDCALL SetQHYCCDCalibrationOnOff(qhyccd_handle *handle, bool onoff);

EXPORTFUNC uint32_t STDCALL SetQHYCCDFrameDetectPos(qhyccd_handle *handle, uint32_t pos);
EXPORTFUNC uint32_t STDCALL SetQHYCCDFrameDetectCode(qhyccd_handle *handle, uint8_t code);
EXPORTFUNC uint32_t STDCALL SetQHYCCDFrameDetectOnOff(qhyccd_handle *handle, bool onoff);

EXPORTFUNC uint32_t STDCALL GetQHYCCDSensorName(qhyccd_handle *handle, char *name);

#if 0//PCIE_MODE_TEST

#include "riffa.h"


/**
 * Populates the fpga_info_list pointer with all FPGAs registered in the system.
 * Returns 0 on success, non-zero on error.
 */
EXPORTC int STDCALL QHYCCD_fpga_list(struct fpga_info_list &list);

/**
 * Initializes the FPGA specified by id. On success, returns a pointer to a
 * fpga_t struct. On error, returns NULL. Each FPGA must be opened before any
 * channels can be accessed. Once opened, any number of threads can use the
 * fpga_t struct.
 */
EXPORTC uint32_t STDCALL QHYCCD_fpga_open(int id);

/**
 * Cleans up memory/resources for the FPGA specified by the fd descriptor.
 */
EXPORTC void STDCALL QHYCCD_fpga_close();

/**
 * Sends len words (4 byte words) from data to FPGA channel chnl using the
 * fpga_t struct. The FPGA channel will be sent len, destoff, and last. If last
 * is 1, the channel should interpret the end of this send as the end of a
 * transaction. If last is 0, the channel should wait for additional sends
 * before the end of the transaction. If timeout is non-zero, this call will
 * send data and wait up to timeout ms for the FPGA to respond (between
 * packets) before timing out. If timeout is zero, this call may block
 * indefinitely. Multiple threads sending on the same channel may result in
 * corrupt data or error. This function is thread safe across channels.
 * Returns the number of words sent.
 */
EXPORTC int STDCALL QHYCCD_fpga_send(int chnl, void * data, int len,
                                     int destoff, int last, uint64_t timeout);

/**
 * Receives data from the FPGA channel chnl to the data pointer, using the
 * fpga_t struct. The FPGA channel can send any amount of data, so the data
 * array should be large enough to accommodate. The len parameter specifies the
 * actual size of the data buffer in words (4 byte words). The FPGA channel will
 * specify an offset which will determine where in the data array the data will
 * start being written. If the amount of data (plus offset) exceed the size of
 * the data array (len), then that data will be discarded. If timeout is
 * non-zero, this call will wait up to timeout ms for the FPGA to respond
 * (between packets) before timing out. If timeout is zero, this call may block
 * indefinitely. Multiple threads receiving on the same channel may result in
 * corrupt data or error. This function is thread safe across channels.
 * Returns the number of words received to the data array.
 */
EXPORTC int STDCALL QHYCCD_fpga_recv( int chnl, void * data, int len,
                                     uint64_t timeout);

/**
 * Resets the state of the FPGA and all transfers across all channels. This is
 * meant to be used as an alternative to rebooting if an error occurs while
 * sending/receiving. Calling this function while other threads are sending or
 * receiving will result in unexpected behavior.
 */
EXPORTC void STDCALL QHYCCD_fpga_reset();
#endif

#endif

/// ----------------------------------

void call_pnp_event();
void call_data_event_live(char *id, uint8_t *imgdata);
void call_transfer_event_error();
void call_critical_event_error(qhyccd_handle *h);
EXPORTFUNC void RegisterPnpEventIn( void (*in_pnp_event_in_func)(char *id));
EXPORTFUNC void RegisterPnpEventOut( void (*in_pnp_event_out_func)(char *id));
EXPORTFUNC void RegisterTransferEventError( void (*transfer_event_error_func)());
EXPORTFUNC uint32_t STDCALL PCIEClearDDR(qhyccd_handle *handle);
EXPORTFUNC uint32_t STDCALL PCIEWriteCameraRegister2(qhyccd_handle *handle, unsigned char idx, unsigned char val);
EXPORTFUNC uint32_t STDCALL QHYCCD_DbGainToGainValue(qhyccd_handle *h,double dbgain,double *gainvalue);
EXPORTFUNC uint32_t STDCALL QHYCCD_GainValueToDbGain(qhyccd_handle *h,double gainvalue,double *dbgain);
EXPORTFUNC uint32_t STDCALL QHYCCD_curveSystemGain(qhyccd_handle *handle,double gainV,double *systemgain);
EXPORTFUNC uint32_t STDCALL QHYCCD_curveFullWell(qhyccd_handle *handle,double gainV,double *fullwell);
EXPORTFUNC uint32_t STDCALL QHYCCD_curveReadoutNoise(qhyccd_handle *handle,double gainV,double *readoutnoise);