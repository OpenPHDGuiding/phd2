/****************************************************************************
**
** Copyright (C) 2023 The Player One Astronomy Co., Ltd.
** This software is the secondary software development kit (SDK) for
** the astronomy cameras made by Player One Astronomy Co., Ltd.
** Player One Astronomy Co., Ltd (hereinafter referred to as "the company") owns its copyright.
** This SDK is only used for the secondary development of our company's cameras or other equipment.
** You can use our company's products and this SDK to develop any products without any restrictions.
** This software may use third-party software or technology (including open source code and public domain code that this software may use),
** and such use complies with their open source license or has obtained legal authorization.
** If you have any questions, please contact us: support@player-one-astronomy.com
** Copyright (C) Player One Astronomy Co., Ltd. All rights reserved.
**
****************************************************************************/

#ifndef PLAYERONECAMERA_H
#define PLAYERONECAMERA_H


#ifdef __cplusplus
extern "C" {
#endif


#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define POACAMERA_API __declspec(dllexport)
#else
#  define POACAMERA_API     __attribute__((visibility("default")))
#endif

//-----For more information, please read our development manual-----//

typedef enum _POABool ///< BOOL Value Definition
{
    POA_FALSE = 0,  ///< false
    POA_TRUE        ///< true
} POABool;

typedef enum _POABayerPattern ///< Bayer Pattern Definition
{
    POA_BAYER_RG = 0,   ///< RGGB
    POA_BAYER_BG,       ///< BGGR
    POA_BAYER_GR,       ///< GRBG
    POA_BAYER_GB,       ///< GBRG
    POA_BAYER_MONO = -1 ///< Monochrome, the mono camera with this
} POABayerPattern;

typedef enum _POAImgFormat  ///< Image Data Format Definition
{
    POA_RAW8 = 0,   ///< 8bit raw data, 1 pixel 1 byte, value range[0, 255]
    POA_RAW16,      ///< 16bit raw data, 1 pixel 2 bytes, value range[0, 65535]
    POA_RGB24,      ///< RGB888 color data, 1 pixel 3 bytes, value range[0, 255] (only color camera)
    POA_MONO8,      ///< 8bit monochrome data, convert the Bayer Filter Array to monochrome data. 1 pixel 1 byte, value range[0, 255] (only color camera)
    POA_END = -1
} POAImgFormat;

typedef enum _POAErrors                 ///< Return Error Code Definition
{
    POA_OK = 0,                         ///< operation successful
    POA_ERROR_INVALID_INDEX,            ///< invalid index, means the index is < 0 or >= the count( camera or config)
    POA_ERROR_INVALID_ID,               ///< invalid camera ID
    POA_ERROR_INVALID_CONFIG,           ///< invalid POAConfig
    POA_ERROR_INVALID_ARGU,             ///< invalid argument(parameter)
    POA_ERROR_NOT_OPENED,               ///< camera not opened
    POA_ERROR_DEVICE_NOT_FOUND,         ///< camera not found, may be removed
    POA_ERROR_OUT_OF_LIMIT,             ///< the value out of limit
    POA_ERROR_EXPOSURE_FAILED,          ///< camera exposure failed
    POA_ERROR_TIMEOUT,                  ///< timeout
    POA_ERROR_SIZE_LESS,                ///< the data buffer size is not enough
    POA_ERROR_EXPOSING,                 ///< camera is exposing. some operation, must stop exposure first
    POA_ERROR_POINTER,                  ///< invalid pointer, when get some value, do not pass the NULL pointer to the function
    POA_ERROR_CONF_CANNOT_WRITE,        ///< the POAConfig is not writable
    POA_ERROR_CONF_CANNOT_READ,         ///< the POAConfig is not readable
    POA_ERROR_ACCESS_DENIED,            ///< access denied
    POA_ERROR_OPERATION_FAILED,         ///< operation failed, maybe the camera is disconnected suddenly
    POA_ERROR_MEMORY_FAILED             ///< memory allocation failed
} POAErrors;

typedef enum _POACameraState            ///< Camera State Definition
{
    STATE_CLOSED = 0,                   ///< camera was closed
    STATE_OPENED,                       ///< camera was opened, but not exposing
    STATE_EXPOSING                      ///< camera is exposing
} POACameraState;

typedef enum _POAValueType              ///< Config Value Type Definition
{
    VAL_INT = 0,                        ///< integer(long)
    VAL_FLOAT,                          ///< float(double)
    VAL_BOOL                            ///< bool(POABool)
} POAValueType;

typedef enum _POAConfig                 ///< Camera Config Definition
{
    POA_EXPOSURE = 0,                   ///< exposure time(unit: us), read-write, valueType == VAL_INT
    POA_GAIN,                           ///< gain, read-write, valueType == VAL_INT
    POA_HARDWARE_BIN,                   ///< hardware bin, read-write, valueType == VAL_BOOL
    POA_TEMPERATURE,                    ///< camera temperature(uint: C), read-only, valueType == VAL_FLOAT
    POA_WB_R,                           ///< red pixels coefficient of white balance, read-write, valueType == VAL_INT
    POA_WB_G,                           ///< green pixels coefficient of white balance, read-write, valueType == VAL_INT
    POA_WB_B,                           ///< blue pixels coefficient of white balance, read-write, valueType == VAL_INT
    POA_OFFSET,                         ///< camera offset, read-write, valueType == VAL_INT
    POA_AUTOEXPO_MAX_GAIN,              ///< maximum gain when auto-adjust, read-write, valueType == VAL_INT
    POA_AUTOEXPO_MAX_EXPOSURE,          ///< maximum exposure when auto-adjust(uint: ms), read-write, valueType == VAL_INT
    POA_AUTOEXPO_BRIGHTNESS,            ///< target brightness when auto-adjust, read-write, valueType == VAL_INT
    POA_GUIDE_NORTH,                    ///< ST4 guide north, generally,it's DEC+ on the mount, read-write, valueType == VAL_BOOL
    POA_GUIDE_SOUTH,                    ///< ST4 guide south, generally,it's DEC- on the mount, read-write, valueType == VAL_BOOL
    POA_GUIDE_EAST,                     ///< ST4 guide east, generally,it's RA+ on the mount, read-write, valueType == VAL_BOOL
    POA_GUIDE_WEST,                     ///< ST4 guide west, generally,it's RA- on the mount, read-write, valueType == VAL_BOOL
    POA_EGAIN,                          ///< e/ADU, This value will change with gain, read-only, valueType == VAL_FLOAT
    POA_COOLER_POWER,                   ///< cooler power percentage[0-100%](only cool camera), read-only, valueType == VAL_INT
    POA_TARGET_TEMP,                    ///< camera target temperature(uint: C), read-write, valueType == VAL_INT
    POA_COOLER,                         ///< turn cooler(and fan) on or off, read-write, valueType == VAL_BOOL
    POA_HEATER,                         ///< (deprecated)get state of lens heater(on or off), read-only, valueType == VAL_BOOL
    POA_HEATER_POWER,                   ///< lens heater power percentage[0-100%], read-write, valueType == VAL_INT
    POA_FAN_POWER,                      ///< radiator fan power percentage[0-100%], read-write, valueType == VAL_INT
    POA_FLIP_NONE,                      ///< no flip, Note: set this config(POASetConfig), the 'confValue' will be ignored, read-write, valueType == VAL_BOOL
    POA_FLIP_HORI,                      ///< flip the image horizontally, Note: set this config(POASetConfig), the 'confValue' will be ignored, read-write, valueType == VAL_BOOL
    POA_FLIP_VERT,                      ///< flip the image vertically, Note: set this config(POASetConfig), the 'confValue' will be ignored, read-write, valueType == VAL_BOOL
    POA_FLIP_BOTH,                      ///< flip the image horizontally and vertically, Note: set this config(POASetConfig), the 'confValue' will be ignored, read-write, valueType == VAL_BOOL
    POA_FRAME_LIMIT,                    ///< Frame rate limit, the range:[0, 2000], 0 means no limit, read-write, valueType == VAL_INT
    POA_HQI,                            ///< High Quality Image, for those without DDR camera(guide camera), if set POA_TRUE, this will reduce the waviness and stripe of the image,
                                        ///< but frame rate may go down, note: this config has no effect on those cameras that with DDR. read-write, valueType == VAL_BOOL
    POA_USB_BANDWIDTH_LIMIT,            ///< USB bandwidth limit, read-write, valueType == VAL_INT
    POA_PIXEL_BIN_SUM,                  ///< take the sum of pixels after binning, POA_TRUE is sum and POA_FLASE is average, default is POA_FLASE, read-write, valueType == VAL_BOOL
    POA_MONO_BIN                        ///< only for color camera, when set to POA_TRUE, pixel binning will use neighbour pixels and image after binning will lose the bayer pattern, read-write, valueType == VAL_BOOL

} POAConfig;

typedef struct _POACameraProperties     ///< Camera Properties Definition
{
    char cameraModelName[256];          ///< the camera name
    char userCustomID[16];              ///< user custom name, it will be will be added after the camera name, max len 16 bytes,like:Mars-C [Juno], default is empty
    int cameraID;                       ///< it's unique,camera can be controlled and set by the cameraID
    int maxWidth;                       ///< max width of the camera
    int maxHeight;                      ///< max height of the camera
    int bitDepth;                       ///< ADC depth of CMOS sensor
    POABool isColorCamera;              ///< is a color camera or not
    POABool isHasST4Port;               ///< does the camera have ST4 port, if not, camera don't support ST4 guide
    POABool isHasCooler;                ///< does the camera have cooler assembly, generally, the cooled camera with cooler, window heater and fan
    POABool isUSB3Speed;                ///< is usb3.0 speed connection
    POABayerPattern bayerPattern;       ///< the bayer filter pattern of camera
    double pixelSize;                   ///< camera pixel size(unit: um)
    char SN[64];                        ///< the serial number of camera,it's unique
    char sensorModelName[32];           ///< the sersor model(name) of camera, eg: IMX462
    char localPath[256];                ///< the path of the camera in the computer host
    int bins[8];                        ///< bins supported by the camera, 1 == bin1, 2 == bin2,..., end with 0, eg:[1,2,3,4,0,0,0,0]
    POAImgFormat imgFormats[8];         ///< image data format supported by the camera, end with POA_END, eg:[POA_RAW8, POA_RAW16, POA_END,...]
    POABool isSupportHardBin;           ///< does the camera sensor support hardware bin (since V3.3.0)
    int pID;                            ///< camera's Product ID, note: the vID of PlayerOne is 0xA0A0 (since V3.3.0)

    char reserved[248];                 ///< reserved, the size of reserved has changed from 256 to 248 since V3.3.0
} POACameraProperties;

typedef union _POAConfigValue           ///< Config Value Definition
{
    long intValue;                      ///< long
    double floatValue;                  ///< double
    POABool boolValue;                  ///< POABool
} POAConfigValue;

typedef struct _POAConfigAttributes     ///< Camera Config Attributes Definition(every POAConfig has a POAConfigAttributes)
{
    POABool isSupportAuto;              ///< is support auto?
    POABool isWritable;                 ///< is writable?
    POABool isReadable;                 ///< is readable?
    POAConfig configID;                 ///< config ID, eg: POA_EXPOSURE
    POAValueType valueType;             ///< value type, eg: VAL_INT
    POAConfigValue maxValue;            ///< maximum value
    POAConfigValue minValue;            ///< minimum value
    POAConfigValue defaultValue;        ///< default value
    char szConfName[64];                ///< POAConfig name, eg: POA_EXPOSURE: "Exposure", POA_TARGET_TEMP: "TargetTemp"
    char szDescription[128];            ///< a brief introduction about this one POAConfig

    char reserved[64];                  ///< reserved
} POAConfigAttributes;


/***********************************************************************************************************
 * Here is a simple process, please refer to our development manual and examples for detailed usage:
 * ----POAGetCameraCount
 * ----POAGetCameraProperties
 * ----POAOpenCamera
 * ----POAInitCamera
 * ----POAGetConfigsCount
 * ----POAGetConfigAttributes
 * ----POASetImageSize
 *     POASetImageStartPos
 *     POASetImageFormat
 * ----POAStartExposure
 * while(1) //this is recommended to do in another thread
 * {
 *      POABool pIsReady = POA_FALSE;
        while(pIsReady == POA_FALSE)
        {
            //sleep(exposure_us /1000 / 10); //ms
            POAImageReady(cameraID, &pIsReady);
        }

        POAGetImageData
 * }
 *
 * ----POAStopExposure
 * ----POACloseCamera
***********************************************************************************************************/

/**
 * @brief POAGetCameraCount: get camera count
 *
 * @return the counts of POA cameras connected to the computer host
 */
POACAMERA_API  int POAGetCameraCount();


/**
 * @brief POAGetCameraProperties: get the property of the connected cameras, NO need to open the camera for this operation
 *
 * @param nIndex (input), the range: [0, camera count), note: index is not cameraID
 *
 * @param pProp (output), pointer to POACameraProperties structure, POACameraProperties structure needs to malloc memory first
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_POINTER: pProp is NULL pointer
 *          POA_ERROR_INVALID_INDEX: no camera connected or nIndex value out of boundary
 */
POACAMERA_API  POAErrors POAGetCameraProperties(int nIndex, POACameraProperties *pProp);


/**
 * @brief POAGetCameraPropertiesByID: get the property of the connected cameras by ID, it's a convenience function to get the property of the known camera ID
 *
 * @param nCameraID (input), get from in the POACameraProperties structure
 *
 * @param pProp (output), pointer to POACameraProperties structure, POACameraProperties structure needs to malloc memory first
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_POINTER: pProp is NULL pointer
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 */
POACAMERA_API  POAErrors POAGetCameraPropertiesByID(int nCameraID, POACameraProperties *pProp);


/**
 * @brief POAOpenCamera: open the camera, note: the following API functions need to open the camera first
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_DEVICE_NOT_FOUND: camera may be removed
 *          POA_ERROR_OPERATION_FAILED: operation failed
 */
POACAMERA_API  POAErrors POAOpenCamera(int nCameraID);


/**
 * @brief POAInitCamera: initialize the camera's hardware, parameters, and malloc memory
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 *          POA_ERROR_MEMORY_FAILED: malloc memory failed, may be the available memory is insufficient.
 *          POA_ERROR_OPERATION_FAILED: operation failed
 */
POACAMERA_API  POAErrors POAInitCamera(int nCameraID);


/**
 * @brief POACloseCamera: close the camera and free allocated memory
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 */
POACAMERA_API  POAErrors POACloseCamera(int nCameraID);


/**
 * @brief POAGetConfigsCount: get the count of POAConfig available for this camera
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param pConfCount (output), pointer to an int to save the count
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_POINTER: pConfCount is NULL pointer
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 */
POACAMERA_API  POAErrors POAGetConfigsCount(int nCameraID, int *pConfCount);


/**
 * @brief POAGetConfigAttributes: get POAConfig attribute by index
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param nConfIndex (intput), the range: [0, configs count)
 *
 * @param pConfAttr (output),  pointer to POAConfigAttributes structure, POAConfigAttributes structure needs to malloc memory first
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_POINTER: pConfAttr is NULL pointer
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 *          POA_ERROR_INVALID_INDEX: no camera connected or nConfIndex value out of boundary
 */
POACAMERA_API  POAErrors POAGetConfigAttributes(int nCameraID, int nConfIndex, POAConfigAttributes *pConfAttr);


/**
 * @brief POAGetConfigAttributesByConfigID: get POAConfig attribute by POAConfig ID, it's a convenience function to get the attribute of the known POAConfig ID
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param confID (input), POAConfig ID, eg: POA_EXPOSURE, POA_USB_BANDWIDTH
 *
 * @param pConfAttr (output),  pointer to POAConfigAttributes structure, POAConfigAttributes structure needs to malloc memory first
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_POINTER: pConfAttr is NULL pointer
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 *          POA_ERROR_INVALID_CONFIG: the confID is not a POAConfig or this camera don't support this POAConfig
 */
POACAMERA_API  POAErrors POAGetConfigAttributesByConfigID(int nCameraID, POAConfig confID, POAConfigAttributes *pConfAttr);


/**
 * @brief POASetConfig: set POAConfig value and auto value
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param confID (input), POAConfig ID, eg: POA_EXPOSURE, POA_USB_BANDWIDTH
 *
 * @param confValue (input), the confValue set to the POAConfig, eg: POAConfigValue confValue; confValue.intValue = 1000
 *
 * @param isAuto (input), set the POAConfig auto
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 *          POA_ERROR_INVALID_CONFIG: the confID is not a POAConfig or this camera don't support this POAConfig
 *          POA_ERROR_CONF_CANNOT_WRITE: this POAConfig does not support writing
 *          POA_ERROR_OPERATION_FAILED: operation failed
 */
POACAMERA_API  POAErrors POASetConfig(int nCameraID, POAConfig confID, POAConfigValue confValue, POABool isAuto);


/**
 * @brief POAGetConfig: get the POAConfig value and auto value
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param confID (input), POAConfig ID, eg: POA_EXPOSURE, POA_USB_BANDWIDTH
 *
 * @param pConfValue (output), pointer to a POAConfigValue for saving the value get from POAConfig
 *
 * @param pIsAuto (output), pointer to a POABool for saving the auto value get from POAConfig
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_POINTER: pConfValue or pIsAuto is NULL pointer
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 *          POA_ERROR_INVALID_CONFIG: the confID is not a POAConfig or this camera don't support this POAConfig
 *          POA_ERROR_CONF_CANNOT_READ: this POAConfig does not support reading
 *          POA_ERROR_OPERATION_FAILED: operation failed
 */
POACAMERA_API  POAErrors POAGetConfig(int nCameraID, POAConfig confID, POAConfigValue *pConfValue, POABool *pIsAuto);


/**
 * @brief POAGetConfigValueType: get POAConfig value type
 *
 * @param confID (input), POAConfig ID, eg: POA_EXPOSURE, POA_USB_BANDWIDTH
 *
 * @param pConfValueType (output), pointer to a POAValueType value, like: VAL_INT
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_POINTER: pConfValueType is NULL pointer
 *          POA_ERROR_INVALID_CONFIG: the confID is not a POAConfig or this camera don't support this POAConfig
 */
POACAMERA_API  POAErrors POAGetConfigValueType(POAConfig confID, POAValueType *pConfValueType);


/**
 * @brief POAGetImageStartPos: get the start position of the ROI area.
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param pStartX (output), pointer to a int value for saving startX
 *
 * @param pStartY (output), pointer to a int value for saving startY
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_POINTER: pStartX or pStartY is NULL pointer
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 */
POACAMERA_API  POAErrors POAGetImageStartPos(int nCameraID, int *pStartX, int *pStartY);


/**
 * @brief POASetImageStartPos: set the start position of the ROI area.
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param startX (input), the starting point X of the ROI
 *
 * @param startY (input), the starting point Y of the ROI
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 *          POA_ERROR_INVALID_ARGU: startX or startY < 0
 *          POA_ERROR_OPERATION_FAILED: operation failed
 */
POACAMERA_API  POAErrors POASetImageStartPos(int nCameraID, int startX, int startY);


/**
 * @brief POAGetImageSize: get the image size of the ROI area
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param pWidth (output), pointer to a int value for saving width
 *
 * @param pHeight (output), pointer to a int value for saving height
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_POINTER: pWidth or pHeight is NULL pointer
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 */
POACAMERA_API  POAErrors POAGetImageSize(int nCameraID, int *pWidth, int *pHeight);


/**
 * @brief POASetImageSize: set the image size of the ROI area, note: should stop exposure first if exposing
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param width (input), the width of ROI area, the width must divide 4 and no remainder, means: width % 4 == 0
 *                note: If you set the width % 4 != 0, this function will automatically adjust the width,
 *                      please call POAGetImageSize to get the width after adjusting
 *
 * @param height (input), the height of ROI area, the height must divide 2 and no remainder, means: height % 2 == 0
 *                note: If you set the height % 2 != 0, this function will automatically adjust the height,
 *                      please call POAGetImageSize to get the height after adjusting
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 *          POA_ERROR_INVALID_ARGU: width or height < 0
 *          POA_ERROR_OPERATION_FAILED: operation failed
 */
POACAMERA_API  POAErrors POASetImageSize(int nCameraID, int width, int height);


/**
 * @brief POAGetImageBin: get the pixel bin method
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param pBin (output), pointer to a int value for saving bin
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_POINTER: pBin is NULL pointer
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 */
POACAMERA_API  POAErrors POAGetImageBin(int nCameraID, int *pBin);


/**
 * @brief POASetImageBin: set the pixel bin method, note: should stop exposure first if exposing,
 *                        If return successful, the image size (width & height) and start position will be changed,
 *                        Please call POAGetImageStartPos and  POAGetImageSize to get the new image size and start position after binning
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param bin (input), binning method, eg: 1, 2, ....
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 *          POA_ERROR_OPERATION_FAILED: operation failed
 */
POACAMERA_API  POAErrors POASetImageBin(int nCameraID, int bin);


/**
 * @brief POAGetImageFormat: get image format
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param pImgFormat (output), pointer to a POAImgFormat value for saving image foramt, eg: POA_RAW16
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_POINTER: pImgFormat is NULL pointer
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 */
POACAMERA_API  POAErrors POAGetImageFormat(int nCameraID, POAImgFormat *pImgFormat);


/**
 * @brief POASetImageFormat: set image format, note: should stop exposure first if exposing
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param imgFormat (input), image (pixels) format, eg: POA_RAW8, POA_RGB24
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 *          POA_ERROR_INVALID_ARGU: the imgFormat is not a POAImgFormat
 *          POA_ERROR_OPERATION_FAILED: operation failed
 */
POACAMERA_API  POAErrors POASetImageFormat(int nCameraID, POAImgFormat imgFormat);


/**
 * @brief POAStartExposure: start camera exposure
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param bSingleFrame (input), POA_TRUE: SnapMode, after the exposure, will not continue(Single Shot), POA_FALSE: VideoMode, continuous exposure
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 *          POA_ERROR_OPERATION_FAILED: operation failed
 *          POA_ERROR_EXPOSING: camera is exposing
 */
POACAMERA_API  POAErrors POAStartExposure(int nCameraID, POABool bSingleFrame);


/**
 * @brief POAStopExposure: stop camera exposure
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 *          POA_ERROR_OPERATION_FAILED: operation failed
 */
POACAMERA_API  POAErrors POAStopExposure(int nCameraID);


/**
 * @brief POAGetCameraState get the camera current state
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param pCameraState (output), pointer to a POACameraState value for saving camera state
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_POINTER: pCameraState is NULL pointer
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 */
POACAMERA_API  POAErrors POAGetCameraState(int nCameraID, POACameraState *pCameraState);


/**
 * @brief POAImageReady: the image data is available? if pIsReady is true, you can call POAGetImageData to get image data
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param pIsReady (output), pointer to a POABool value for saving the image data is ready
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_POINTER: pIsReady is NULL pointer
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 */
POACAMERA_API  POAErrors POAImageReady(int nCameraID, POABool *pIsReady);


/**
 * @brief POAGetImageData: get image data after exposure, this function will block and waiting for timeout
 *                         Note: recommended to use POAImageReady function for waiting, if image data 'Is Ready', calling this function will return immediately
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param pBuf (output), pointer to data buffer, note: the buffer need to be mallloced first, and make sure it's big enough
 *
 * @param nBufSize (input), the buffer size, POA_RAW8: width*height, POA_RAW16: width*height*2, POA_RGB24: width*height*3
 *
 * @param nTimeoutms (input), wait time (ms), recommend set it to exposure+500ms, -1 means infinite waiting
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_POINTER: pBuf is NULL pointer
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 *          POA_ERROR_INVALID_ARGU: may be lBufSize < 0
 *          POA_ERROR_SIZE_LESS: the nBufSize is not enough to hold the data
 *          POA_ERROR_OPERATION_FAILED: operation failed
 */
POACAMERA_API  POAErrors POAGetImageData(int nCameraID, unsigned char *pBuf, long lBufSize, int nTimeoutms);


/**
 * @brief POAGetDroppedImagesCount: get the dropped image count, reset it to 0 after stop capture
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param pDroppedCount (output), pointer to a int value for saving the dropped image count
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_POINTER: pDroppedCount is NULL pointer
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 */
POACAMERA_API  POAErrors POAGetDroppedImagesCount(int nCameraID, int *pDroppedCount);


/**
 * @brief POAGetSensorModeCount: get the number of sensor mode
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param pModeCount (output), pointer to a int value for saving the sensor mode count, NOTE: 0 means camera don't supported mode selection
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_POINTER: pModeCount is NULL pointer
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 */
POACAMERA_API  POAErrors POAGetSensorModeCount(int nCameraID, int *pModeCount);


typedef struct _POASensorModeInfo ///< Sensor mode information
{
    char name[64];  ///< sensor mode name, can be used to display on the UI (eg: Combobox)
    char desc[128]; ///< sensor mode description, may be useful for tooltip
}POASensorModeInfo;


/**
 * @brief POAGetSensorModeInfo: get the camera sensor mode information according to the index
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param modeIndex (input), the range: [0, mode count)
 *
 * @param pSenModeInfo (output), pointer to a POASensorModeInfo value for saving the sensor mode information
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_POINTER: pSenModeInfo is NULL pointer
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 *          POA_ERROR_ACCESS_DENIED: camera don't supported mode selection
 *          POA_ERROR_INVALID_ARGU: modeIndex is out of range
 */
POACAMERA_API  POAErrors POAGetSensorModeInfo(int nCameraID, int modeIndex, POASensorModeInfo *pSenModeInfo);


/**
 * @brief POASetSensorMode: set the camera sensor mode by the index, Note: should stop exposure first if exposing
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param modeIndex (input), the range: [0, mode count)
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 *          POA_ERROR_ACCESS_DENIED: camera don't supported mode selection
 *          POA_ERROR_INVALID_ARGU: modeIndex is out of range
 *          POA_ERROR_OPERATION_FAILED: operation failed, maybe the camera is disconnected suddenly
 */
POACAMERA_API  POAErrors POASetSensorMode(int nCameraID, int modeIndex);


/**
 * @brief POAGetSensorMode: get camera the current sensor mode
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 * @param pModeIndex (output), pointer to a int value for saving the current sensor mode index
 * @return  POA_OK: operation successful
 *          POA_ERROR_POINTER: pModeIndex is NULL pointer
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 *          POA_ERROR_ACCESS_DENIED: camera don't supported mode selection
 *          POA_ERROR_OPERATION_FAILED: operation failed, the current mode is not matched
 */
POACAMERA_API  POAErrors POAGetSensorMode(int nCameraID, int *pModeIndex);


/**
 * @brief POASetUserCustomID: set user custom ID into camera flash, if set successfully, reacquire the information of this camera to get the custom ID
 *                            Note: this operation will interrupt the exposure, if start a Signal Frame exposure , the exposure progress will be terminated.
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param pCustomID (input), pointer to a string,like: const char* pCustomID = "MyCamera"，if pCustomID is NULL, the previous Settings will be cleared
 *
 * @param len (input), max len is 16, if len > 16, the extra part will be cut off, if len is 0, the previous Settings will be cleared
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 *          POA_ERROR_NOT_OPENED: camera not opened
 *          POA_ERROR_EXPOSING：this operation is not allowed while the camera is exposing
 *          POA_ERROR_OPERATION_FAILED: operation failed
 */
POACAMERA_API  POAErrors POASetUserCustomID(int nCameraID, const char* pCustomID, int len);


/**
 * @brief POAGetGainOffset: get some preset values, Note: deprecated, please use the following function
 *
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 *
 * @param pOffsetHighestDR (output), offset at highest dynamic range
 *
 * @param pOffsetUnityGain (output), offset at unity gain
 *
 * @param pGainLowestRN (output), gain at lowest read noise
 *
 * @param pOffsetLowestRN (output), offset at lowest read noise
 *
 * @param pHCGain (output), gain at HCG Mode(High Conversion Gain)
 *
 * @return  POA_OK: operation successful
 *          POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 */
POACAMERA_API  POAErrors POAGetGainOffset(int nCameraID, int *pOffsetHighestDR, int *pOffsetUnityGain, int *pGainLowestRN, int *pOffsetLowestRN, int *pHCGain);

/**
 * @brief POAGetGainsAndOffsets: get some preset values of gain and offset
 * @param nCameraID (input), get from in the POACameraProperties structure, use POAGetCameraProperties function
 * @param pGainHighestDR (output), gain at highest dynamic range, in most cases, this gain is 0
 * @param pHCGain (output), gain at HCG Mode(High Conversion Gain)
 * @param pUnityGain (output), unity gain(or standard gain), with this gain, eGain(e/ADU) will be 1
 * @param pGainLowestRN (output), aka Maximum Analog Gain, gain at lowest read noise
 * @param pOffsetHighestDR (output), offset at highest dynamic range
 * @param pOffsetHCGain (output), offset at HCG Mode
 * @param pOffsetUnityGain (output), offset at unity gain
 * @param pOffsetLowestRN (output), offset at lowest read noise
 * @return POA_OK: operation successful
 *         POA_ERROR_INVALID_ID: no camera with this ID was found or the ID is out of boundary
 */
POACAMERA_API  POAErrors POAGetGainsAndOffsets(int nCameraID, int *pGainHighestDR, int *pHCGain, int *pUnityGain, int *pGainLowestRN,
                                               int *pOffsetHighestDR, int *pOffsetHCGain, int *pOffsetUnityGain, int *pOffsetLowestRN);


/**
 * @brief POAGetErrorString: convert POAErrors enum to char *, it is convenient to print or display errors
 *
 * @param err (intput), POAErrors, the value returned by the API function
 *
 * @return  point to const char* error
 */
POACAMERA_API const char* POAGetErrorString(POAErrors err);


/**
 * @brief POAGetAPIVersion: get the API version
 *
 * @return: it's a integer value, easy to do version comparison, eg: 20200202
 */
POACAMERA_API int POAGetAPIVersion();


/**
 * @brief POAGetSDKVersion: get the sdk version
 *
 * @return  point to const char* version(major.minor.patch), eg: 1.0.1
 */
POACAMERA_API const char* POAGetSDKVersion();


/**this function for matlab**/
POACAMERA_API  POAErrors POASetConfig_M(int nCameraID, POAConfig confID, double cfgVal, POABool isAuto);

#ifdef __cplusplus
}
#endif

#endif
