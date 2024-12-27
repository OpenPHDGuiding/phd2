#ifndef CONVFUNCS_H
#define CONVFUNCS_H

/***********************************************************************************************************
 * These are convenience functions, may possibly increase development efficiency, hope to help you.
 * These functions are independent of each other, you can copy any function into your code.
 * If you have any problems, please contact me: lei.zhang@player-one-astronomy.com
***********************************************************************************************************/

#include "PlayerOneCamera.h"

/***********************************************************************
*some instructions
*to CPP: You can treat POABool as bool,for example:
*        POABool isOK = POA_TRUE;
*        if(isOK) {...} or if(isOK == POA_TRUE){...}
*to pure C: C does not support function overloading, you need change some functions name, for example:
*           POAGetConfig ==> POAGetConfig_INT
***********************************************************************/


//Get the current value of POAConfig with POAValueType is VAL_INT, eg: POA_EXPOSURE, POA_GAIN
POAErrors POAGetConfig(int nCameraID, POAConfig confID, long *pValue, POABool *pIsAuto)
{
    POAValueType pConfValueType;
    POAErrors error = POAGetConfigValueType(confID, &pConfValueType);
    if (error == POA_OK)
    {
        if (pConfValueType != VAL_INT)
        { return POA_ERROR_INVALID_CONFIG; }
    }
    else
    {
        return error;
    }

    POAConfigValue confValue;
    error = POAGetConfig(nCameraID, confID, &confValue, pIsAuto);

    if(error == POA_OK)
    { *pValue = confValue.intValue; }

    return error;
}


//Get the current value of POAConfig with POAValueType is VAL_FLOAT, eg: POA_TEMPERATURE, POA_EGAIN
POAErrors POAGetConfig(int nCameraID, POAConfig confID, double *pValue, POABool *pIsAuto)
{
    POAValueType pConfValueType;
    POAErrors error = POAGetConfigValueType(confID, &pConfValueType);
    if (error == POA_OK)
    {
        if (pConfValueType != VAL_FLOAT)
        { return POA_ERROR_INVALID_CONFIG; }
    }
    else
    {
        return error;
    }

    POAConfigValue confValue;
    error = POAGetConfig(nCameraID, confID, &confValue, pIsAuto);

    if(error == POA_OK)
    { *pValue = confValue.floatValue; }

    return error;
}


//Get the current value of POAConfig with POAValueType is VAL_BOOL, eg: POA_COOLER, POA_PIXEL_BIN_SUM
POAErrors POAGetConfig(int nCameraID, POAConfig confID, POABool *pIsEnable)
{
    POAValueType pConfValueType;
    POAErrors error = POAGetConfigValueType(confID, &pConfValueType);
    if (error == POA_OK)
    {
        if (pConfValueType != VAL_BOOL)
        {
            return POA_ERROR_INVALID_CONFIG;
        }
    }
    else
    {
        return error;
    }

    POAConfigValue confValue;
    POABool boolValue;
    error = POAGetConfig(nCameraID, confID, &confValue, &boolValue);

    if (error == POA_OK)
    { *pIsEnable = confValue.boolValue; }

    return error;
}


//Set the POAConfig value, the POAValueType of POAConfig is VAL_INT, eg: POA_TARGET_TEMP, POA_OFFSET
POAErrors POASetConfig(int nCameraID, POAConfig confID, long nValue, POABool isAuto)
{
    POAValueType pConfValueType;
    POAErrors error = POAGetConfigValueType(confID, &pConfValueType);
    if (error == POA_OK)
    {
        if (pConfValueType != VAL_INT)
        {
            return POA_ERROR_INVALID_CONFIG;
        }
    }
    else
    {
        return error;
    }

    POAConfigValue confValue;
    confValue.intValue = nValue;

    return POASetConfig(nCameraID, confID, confValue, isAuto);
}


//Set the POAConfig value, the POAValueType of POAConfig is VAL_FLOAT, Note: currently, there is no POAConfig which POAValueType is VAL_FLOAT needs to be set
POAErrors POASetConfig(int nCameraID, POAConfig confID, double fValue, POABool isAuto)
{
    POAValueType pConfValueType;
    POAErrors error = POAGetConfigValueType(confID, &pConfValueType);
    if (error == POA_OK)
    {
        if (pConfValueType != VAL_FLOAT)
        {
            return POA_ERROR_INVALID_CONFIG;
        }
    }
    else
    {
        return error;
    }

    POAConfigValue confValue;
    confValue.floatValue = fValue;

    return POASetConfig(nCameraID, confID, confValue, isAuto);
}


//Set the POAConfig value, the POAValueType of POAConfig is VAL_BOOL, eg: POA_HARDWARE_BIN, POA_GUIDE_NORTH
POAErrors POASetConfig(int nCameraID, POAConfig confID, POABool isEnable)
{
    POAValueType pConfValueType;
    POAErrors error = POAGetConfigValueType(confID, &pConfValueType);
    if (error == POA_OK)
    {
        if (pConfValueType != VAL_BOOL)
        {
            return POA_ERROR_INVALID_CONFIG;
        }
    }
    else
    {
        return error;
    }

    POAConfigValue confValue;
    confValue.boolValue = isEnable;

    return POASetConfig(nCameraID, confID, confValue, POA_FALSE);
}


//Get the range of a POAConfig, eg: exposure range[10us, 2000000000us], default is 10000us
POAErrors GetConfigRange(int nCameraID, POAConfig confID, long *pMax, long *pMin, long *pDefult)
{
    if(!pMax || !pMin || !pDefult)
    { return POA_ERROR_POINTER; }

    POAValueType pConfValueType;
    POAErrors error = POAGetConfigValueType(confID, &pConfValueType);
    if (error == POA_OK)
    {
        if (pConfValueType != VAL_INT)
        { return POA_ERROR_INVALID_CONFIG; }
    }
    else
    {
        return error;
    }

    POAConfigAttributes confAttri;
    error = POAGetConfigAttributesByConfigID(nCameraID, confID, &confAttri);
    if(error == POA_OK)
    {
        *pMax = confAttri.maxValue.intValue;
        *pMin = confAttri.minValue.intValue;
        *pDefult = confAttri.defaultValue.intValue;
    }

    return error;
}


POAErrors POAGetFlip(int nCameraID, POABool *pIsFlipHori, POABool *pIsFlipVert)
{
    if(!pIsFlipHori || !pIsFlipVert)
    {
        return POA_ERROR_POINTER;
    }

    POAConfigValue confValue;
    POABool boolValue;
    POAErrors error = POAGetConfig(nCameraID, POA_FLIP_BOTH, &confValue, &boolValue);

    if(error != POA_OK)
    { return error; }

    if(confValue.boolValue) //is both flip
    {
        *pIsFlipHori = POA_TRUE;
        *pIsFlipVert = POA_TRUE;

        return POA_OK;
    }

    confValue.boolValue = POA_FALSE;
    error = POAGetConfig(nCameraID, POA_FLIP_VERT, &confValue, &boolValue);

    if(error != POA_OK)
    { return error; }

    if(confValue.boolValue) // is vert flip
    {
        *pIsFlipHori = POA_FALSE;
        *pIsFlipVert = POA_TRUE;

        return POA_OK;
    }

    confValue.boolValue = POA_FALSE;
    error = POAGetConfig(nCameraID, POA_FLIP_HORI, &confValue, &boolValue);

    if(error != POA_OK)
    { return error; }

    if(confValue.boolValue) // is hori flip
    {
        *pIsFlipHori = POA_TRUE;
        *pIsFlipVert = POA_FALSE;

        return POA_OK;
    }

    *pIsFlipHori = POA_FALSE;
    *pIsFlipVert = POA_FALSE;

    return POA_OK;
}


POAErrors POASetFlip(int nCameraID, POABool isFlipHori, POABool isFlipVert)
{
    POAConfig confID;
    if(isFlipHori && isFlipVert) //both flip
    {
        confID = POA_FLIP_BOTH;
    }
    else if(!isFlipHori && isFlipVert) //vert flip
    {
        confID = POA_FLIP_VERT;
    }
    else if(isFlipHori && !isFlipVert) //hori flip
    {
        confID = POA_FLIP_HORI;
    }
    else                              //none flip
    {
        confID = POA_FLIP_NONE;
    }

    POAConfigValue confValue;
    confValue.boolValue = POA_TRUE;

    return POASetConfig(nCameraID, confID, confValue, POA_FALSE);
}


typedef enum _GuideDirection
{
    GUIDE_NORTH = 0,
    GUIDE_SOUTH,
    GUIDE_EAST,
    GUIDE_WEST
} GuideDirection;

//recommend: thread{
//                    POAGuide_ST4(nCameraID, GUIDE_NORTH, POA_TRUE);  // start guide to north
//                    sleep(100ms);                                    // guide a period of time
//                    POAGuide_ST4(nCameraID, GUIDE_NORTH, POA_FALSE); // stop guide
//                  }
POAErrors POAGuide_ST4(int nCameraID, GuideDirection guideDirection, POABool isON)
{
    POAConfig confID;
    switch (guideDirection)
    {
    case GUIDE_NORTH:
        confID = POA_GUIDE_NORTH;
        break;
    case GUIDE_SOUTH:
        confID = POA_GUIDE_SOUTH;
        break;
    case GUIDE_EAST:
        confID = POA_GUIDE_EAST;
        break;
    case GUIDE_WEST:
        confID = POA_GUIDE_WEST;
        break;
    default:
        return POA_ERROR_INVALID_ARGU;
    }

    POAConfigValue confValue;
    confValue.boolValue = isON;

    return POASetConfig(nCameraID, confID, confValue, POA_FALSE);
}


const char* ImageFormatToString(POAImgFormat imgFmt)
{
    switch (imgFmt)
    {
    case POA_RAW8:
        return "RAW8";
    case POA_RAW16:
        return "RAW16";
    case POA_RGB24:
        return "RGB24";
    case POA_MONO8:
        return "MONO8";
    default:
        return "Unknown";
    }
}


const char* BayerPatternToString(POABayerPattern bayPat)
{
    switch (bayPat)
    {
    case POA_BAYER_RG:
        return "RGGB";
    case POA_BAYER_BG:
        return "BGGR";
    case POA_BAYER_GR:
        return "GRBG";
    case POA_BAYER_GB:
        return "GBRG";
    case POA_BAYER_MONO:
        return "NONE";
    default:
        return "Unknown";
    }
}


const char* BoolToString(POABool bFlag)
{
    return bFlag ? "Yes" : "No";
}

#endif // CONVFUNCS_H
