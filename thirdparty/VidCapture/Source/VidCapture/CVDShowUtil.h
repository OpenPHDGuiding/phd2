/// \file CVDShowUtil.h
/// \brief DirectShow utility functions straight out of the DirectX 9 documentation.
///
/// These provide easy calls to connect between Direct Show filters and manage
/// the memory associated with media type objects.
///

#ifndef CVDShowUtil_H_
#define CVDShowUtil_H_

#include <windows.h>
#include <Dshow.h>   // DirectShow (using DirectX 9.0 for dev)
#include <qedit.h>   // Sample grabber defines



/// GetUnconnectedPin finds an unconnected pin on a filter 
/// in the desired direction.
///
/// \param pFilter - pointer to filter
/// \param PinDir - direction of pin (PINDIR_INPUT or PINDIR_OUTPUT)
/// \param ppPin - Receives pointer to pin interface
///
/// \return HRESULT - S_OK on success
/// \sa ConnectFilters(), CVVidCaptureDSWin32
HRESULT GetUnconnectedPin(
    IBaseFilter *pFilter,
    PIN_DIRECTION PinDir,
    IPin **ppPin);

///
/// ConnectFilters connects a pin of an upstream filter to the 
/// downstream filter pDest.
///
/// \param pGraph - Filter graph (both filters must be already added)
/// \param pOut - Output pin on upstream filter. See GetUnconnectedPin()
/// \param pDest - Downstream filter to be connected
///
/// \return HRESULT - S_OK on success
/// \sa GetUnconnectedPin(), CVVidCaptureDSWin32
HRESULT ConnectFilters(
    IGraphBuilder *pGraph,
    IPin *pOut,           
    IBaseFilter *pDest);  

///
/// DisconnectPins() disconnects any attached filters.
/// 
/// \param pFilter - filter to disconnect
/// \return HRESULT - S_OK on success
///
HRESULT DisconnectPins(IBaseFilter *pFilter);

///
/// ConnectFilters connects two filters together.
///
/// \param pGraph - Filter graph (both filters must be already added)
/// \param pSrc - Upstream filter
/// \param pDest - Downstream filter to be connected
///
/// \return HRESULT - S_OK on success
/// \sa GetUnconnectedPin(), CVVidCaptureDSWin32
HRESULT ConnectFilters(
    IGraphBuilder *pGraph, 
    IBaseFilter *pSrc, 
    IBaseFilter *pDest);

///
/// LocalFreeMediaType frees the format block of a media type object
/// \param mt - reference to media type
/// \sa LocalDeleteMediaType
void LocalFreeMediaType(AM_MEDIA_TYPE& mt);

/// LocalDeleteMediaType frees the format block of a media type object,
/// then deletes it.
/// \param pmt - pointer to media type
/// \sa LocalFreeMediaType
void LocalDeleteMediaType(AM_MEDIA_TYPE *pmt);

#endif //CVDShowUtil_H_

