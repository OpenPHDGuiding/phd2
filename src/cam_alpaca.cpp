/*
 *  cam_alpaca.cpp
 *  PHD Guiding
 *
 *  Copyright (c) 2026 PHD2 Developers
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

#ifdef ALPACA_CAMERA

#include "cam_alpaca.h"
#include "camera.h"
#include "config_alpaca.h"
#include "alpaca_client.h"
#include "image_math.h"
#include <wx/stopwatch.h>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cctype>

namespace {
constexpr std::uint32_t kImageBytesMetadataVersion = 1;
constexpr std::size_t kImageBytesMetadataSize = 11 * sizeof(std::uint32_t);

constexpr std::uint32_t kImageTypeInt16 = 1;
constexpr std::uint32_t kImageTypeInt32 = 2;
constexpr std::uint32_t kImageTypeDouble = 3;
constexpr std::uint32_t kImageTypeSingle = 4;
constexpr std::uint32_t kImageTypeUInt64 = 5;
constexpr std::uint32_t kImageTypeByte = 6;
constexpr std::uint32_t kImageTypeInt64 = 7;
constexpr std::uint32_t kImageTypeUInt16 = 8;
constexpr std::uint32_t kImageTypeUInt32 = 9;

bool ReadUInt32LE(const std::string& data, std::size_t offset, std::uint32_t *value)
{
    if (offset + sizeof(std::uint32_t) > data.size())
        return false;
    const unsigned char *ptr = reinterpret_cast<const unsigned char *>(data.data() + offset);
    *value = static_cast<std::uint32_t>(ptr[0]) |
             (static_cast<std::uint32_t>(ptr[1]) << 8) |
             (static_cast<std::uint32_t>(ptr[2]) << 16) |
             (static_cast<std::uint32_t>(ptr[3]) << 24);
    return true;
}

bool IsContentTypeImageBytes(const std::string& contentType)
{
    std::string lower;
    lower.resize(contentType.size());
    std::transform(contentType.begin(), contentType.end(), lower.begin(), ::tolower);
    return lower.find("application/imagebytes") != std::string::npos;
}

bool IsContentTypeJson(const std::string& contentType)
{
    std::string lower;
    lower.resize(contentType.size());
    std::transform(contentType.begin(), contentType.end(), lower.begin(), ::tolower);
    return lower.find("application/json") != std::string::npos;
}
} // namespace

CameraAlpaca::CameraAlpaca()
{
    Connected = false;
    m_hasGuideOutput = false;
    HasGainControl = false;
    HasSubframes = true;
    PropertyDialogType = PROPDLG_WHEN_DISCONNECTED;
    Color = false;
    m_driverVersion = 1;
    m_bitsPerPixel = 0;
    m_swapAxes = false;
    m_canAbortExposure = false;
    m_canStopExposure = false;
    m_canSetCoolerTemperature = false;
    m_canGetCoolerPower = false;

    ClearStatus();
    // load the values from the current profile
    m_host = pConfig->Profile.GetString("/alpaca/host", _T("localhost"));
    m_port = pConfig->Profile.GetLong("/alpaca/port", 6800);
    m_deviceNumber = pConfig->Profile.GetLong("/alpaca/camera_device", 0);
    // Set Name after loading settings so it has the correct values
    Name = wxString::Format("Alpaca Camera [%s:%ld/%ld]", m_host, m_port, m_deviceNumber);
    m_client = nullptr;
}

CameraAlpaca::~CameraAlpaca()
{
    Disconnect();
    if (m_client)
    {
        delete m_client;
        m_client = nullptr;
    }
}

void CameraAlpaca::ClearStatus()
{
    Connected = false;
    m_maxSize = wxSize(0, 0);
    FrameSize = UNDEFINED_FRAME_SIZE;
    m_bitsPerPixel = 0;
    m_driverPixelSize = 0.0;
    m_roi = wxRect();
    m_curBin = 1;
}

wxByte CameraAlpaca::BitsPerPixel()
{
    return m_bitsPerPixel;
}

bool CameraAlpaca::GetDevicePixelSize(double *devPixelSize)
{
    if (!Connected)
    {
        Debug.Write("Alpaca Camera: GetDevicePixelSize called when not connected\n");
        return true; // Error: not connected
    }

    if (m_driverPixelSize <= 0.0)
    {
        Debug.Write(wxString::Format("Alpaca Camera: GetDevicePixelSize - invalid pixel size (%.2f)\n", m_driverPixelSize));
        return true; // Error: invalid pixel size
    }

    *devPixelSize = m_driverPixelSize;
    Debug.Write(wxString::Format("Alpaca Camera: GetDevicePixelSize returning %.2f microns\n", m_driverPixelSize));
    return false; // Success
}

bool CameraAlpaca::Connect(const wxString& camId)
{
    // If not configured open the setup dialog
    if (m_host == _T("localhost") && m_port == 6800 && m_deviceNumber == 0)
    {
        CameraSetup();
        // Reload values after dialog
        m_host = pConfig->Profile.GetString("/alpaca/host", _T("localhost"));
        m_port = pConfig->Profile.GetLong("/alpaca/port", 6800);
        m_deviceNumber = pConfig->Profile.GetLong("/alpaca/camera_device", 0);
        // If still using defaults after setup, user probably cancelled - don't try to connect
        if (m_host == _T("localhost") && m_port == 6800 && m_deviceNumber == 0)
        {
            Debug.Write("Alpaca Camera: Setup cancelled or not configured, skipping connection\n");
            return CamConnectFailed(_("Alpaca Camera: Setup cancelled or not configured"));
        }
    }

    if (Connected)
    {
        Debug.Write("Alpaca Camera: attempt to connect when already connected\n");
        return false;
    }

    Debug.Write(wxString::Format("Alpaca Camera connecting to %s:%ld device %ld\n", m_host, m_port, m_deviceNumber));

    // Recreate client if it doesn't exist or if settings have changed
    // (AlpacaClient stores host/port for URL building, so we need a new one if they changed)
    if (!m_client)
    {
        m_client = new AlpacaClient(m_host, m_port, m_deviceNumber);
    }
    // Note: If client exists but settings changed, CameraSetup() should have deleted it
    // But we check here just in case Connect() is called directly after profile change

    // Check if device is connected
    wxString endpoint = wxString::Format("camera/%ld/connected", m_deviceNumber);
    bool connected = false;
    long errorCode = 0;
    if (!m_client->GetBool(endpoint, &connected, &errorCode))
    {
        wxString errorMsg;
        if (errorCode == 0)
        {
            errorMsg = wxString::Format(_("Alpaca Camera: Cannot reach server at %s:%ld. Please check:\n- The Alpaca server is running\n- The IP address and port are correct\n- Firewall is not blocking the connection\n- Network connectivity is working"), m_host, m_port);
        }
        else if (errorCode == 200)
        {
            errorMsg = wxString::Format(_("Alpaca Camera: Server at %s:%ld returned an authentication response instead of camera API data for device %ld.\n\nThis usually means:\n- The Alpaca server has authentication enabled\n- A reverse proxy is intercepting requests\n- The server requires authentication for API access\n\nPlease check the server configuration to allow direct API access, or check the debug log for the actual response received."), m_host, m_port, m_deviceNumber);
        }
        else
        {
            errorMsg = wxString::Format(_("Alpaca Camera: Failed to connect to %s:%ld - HTTP error %ld. Please check that the Alpaca server is running and device %ld exists."), m_host, m_port, errorCode, m_deviceNumber);
        }
        Debug.Write(errorMsg + "\n");
        return CamConnectFailed(errorMsg);
    }

    if (!connected)
    {
        // Try to connect
        wxString connectEndpoint = wxString::Format("camera/%ld/connected", m_deviceNumber);
        wxString params = "Connected=true";
        JsonParser parser;
        if (!m_client->Put(connectEndpoint, params, parser, &errorCode))
        {
            wxString errorMsg = wxString::Format(_("Alpaca Camera: Failed to connect device %ld on %s:%ld - error %ld"), m_deviceNumber, m_host, m_port, errorCode);
            Debug.Write(errorMsg + "\n");
            return CamConnectFailed(errorMsg);
        }

        bool nowConnected = false;
        long verifyErrorCode = 0;
        int connectTimeoutMs = GetTimeoutMs();
        if (connectTimeoutMs < 2000)
        {
            connectTimeoutMs = 2000;
        }
        else if (connectTimeoutMs > 30000)
        {
            connectTimeoutMs = 30000;
        }
        const int pollIntervalMs = 100;
        int attempts = connectTimeoutMs / pollIntervalMs;
        if (attempts < 1)
        {
            attempts = 1;
        }
        Debug.Write(wxString::Format("Alpaca Camera: waiting up to %d ms for device %ld to connect\n",
                                     connectTimeoutMs, m_deviceNumber));
        for (int attempt = 0; attempt < attempts; ++attempt)
        {
            if (m_client->GetBool(endpoint, &nowConnected, &verifyErrorCode) && nowConnected)
            {
                break;
            }
            wxMilliSleep(pollIntervalMs);
        }
        if (!nowConnected)
        {
            int cameraState = 0;
            long stateErrorCode = 0;
            wxString stateEndpoint = wxString::Format("camera/%ld/camerastate", m_deviceNumber);
            if (m_client->GetInt(stateEndpoint, &cameraState, &stateErrorCode))
            {
                Debug.Write(wxString::Format("Alpaca Camera: Connected property did not update, but CameraState=%d; continuing\n",
                                             cameraState));
                nowConnected = true;
            }
            else
            {
                wxString errorMsg;
                if (stateErrorCode != 0)
                {
                    errorMsg = wxString::Format(_("Alpaca Camera: Device %ld did not report connected and CameraState failed (error %ld) on %s:%ld"),
                                                m_deviceNumber, stateErrorCode, m_host, m_port);
                }
                else if (verifyErrorCode != 0)
                {
                    errorMsg = wxString::Format(_("Alpaca Camera: Device %ld did not report connected (error %ld) on %s:%ld"),
                                                m_deviceNumber, verifyErrorCode, m_host, m_port);
                }
                else
                {
                    errorMsg = wxString::Format(_("Alpaca Camera: Timed out waiting for device %ld to connect on %s:%ld"),
                                                m_deviceNumber, m_host, m_port);
                }
                Debug.Write(errorMsg + "\n");
                return CamConnectFailed(errorMsg);
            }
        }
    }

    // Get camera name
    endpoint = wxString::Format("camera/%ld/name", m_deviceNumber);
    wxString cameraName;
    if (m_client->GetString(endpoint, &cameraName, &errorCode) && !cameraName.IsEmpty())
    {
        Name = wxString::Format("Alpaca Camera [%s:%ld/%ld] - %s", m_host, m_port, m_deviceNumber, cameraName);
        Debug.Write(wxString::Format("Alpaca Camera: setting camera Name = %s\n", Name));
    }

    // Get driver description (optional, like NINA does)
    endpoint = wxString::Format("camera/%ld/description", m_deviceNumber);
    wxString driverDesc;
    if (m_client->GetString(endpoint, &driverDesc, &errorCode) && !driverDesc.IsEmpty())
    {
        Debug.Write(wxString::Format("Alpaca Camera: Driver Description = %s\n", driverDesc));
        // Optionally append to Name for better identification
        if (!Name.Contains(driverDesc))
        {
            Name += wxString::Format(" (%s)", driverDesc);
        }
    }
    else
    {
        Debug.Write(wxString::Format("Alpaca Camera: Description property not available (HTTP %ld), skipping\n", errorCode));
    }

    // Check capabilities - mirror ASCOM approach

    // See if we have an onboard guider output (optional property)
    endpoint = wxString::Format("camera/%ld/canpulseguide", m_deviceNumber);
    bool canPulseGuide = false;
    if (m_client->GetBool(endpoint, &canPulseGuide, &errorCode))
    {
        m_hasGuideOutput = canPulseGuide;
    }
    else
    {
        // CanPulseGuide is optional - if not available, assume no pulse guide support
        m_hasGuideOutput = false;
        Debug.Write(wxString::Format("Alpaca Camera: CanPulseGuide property not available (HTTP %ld), assuming no pulse guide support\n", errorCode));
    }

    // Check abort exposure capability (optional - default to false if not available)
    endpoint = wxString::Format("camera/%ld/canabortexposure", m_deviceNumber);
    if (m_client->GetBool(endpoint, &m_canAbortExposure, &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca Camera: CanAbortExposure = %s\n", m_canAbortExposure ? "true" : "false"));
    }
    else
    {
        // CanAbortExposure is optional - if not available, default to false
        m_canAbortExposure = false;
        Debug.Write(wxString::Format("Alpaca Camera: CanAbortExposure property not available (HTTP %ld), defaulting to false\n", errorCode));
    }

    // Check stop exposure capability (optional - default to false if not available)
    endpoint = wxString::Format("camera/%ld/canstopexposure", m_deviceNumber);
    if (m_client->GetBool(endpoint, &m_canStopExposure, &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca Camera: CanStopExposure = %s\n", m_canStopExposure ? "true" : "false"));
    }
    else
    {
        // CanStopExposure is optional - if not available, default to false
        m_canStopExposure = false;
        Debug.Write(wxString::Format("Alpaca Camera: CanStopExposure property not available (HTTP %ld), defaulting to false\n", errorCode));
    }
    
    // At least one of them should be available for proper operation, but we'll continue anyway
    if (!m_canAbortExposure && !m_canStopExposure)
    {
        Debug.Write("Alpaca Camera: Warning - neither CanAbortExposure nor CanStopExposure is available. Exposure abort may not work.\n");
    }

    // Check if we have a shutter
    endpoint = wxString::Format("camera/%ld/hasshutter", m_deviceNumber);
    bool hasShutter = false;
    if (m_client->GetBool(endpoint, &hasShutter, &errorCode))
    {
        HasShutter = hasShutter;
    }

    // Get the image size of a full frame
    endpoint = wxString::Format("camera/%ld/cameraxsize", m_deviceNumber);
    int camXSize = 0;
    if (!m_client->GetInt(endpoint, &camXSize, &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca Camera: cannot get CameraXSize property from %s:%ld device %ld, HTTP %ld\n", m_host, m_port, m_deviceNumber, errorCode));
        wxString errorMsg = wxString::Format(_("Alpaca Camera driver missing the %s property.\n\nServer: %s:%ld\nDevice: %ld\nHTTP Error: %ld\n\nPlease check:\n- The device number is correct\n- The camera is properly connected to the Alpaca server\n- Report this error to your Alpaca driver provider"), "CameraXSize", m_host, m_port, m_deviceNumber, errorCode);
        return CamConnectFailed(errorMsg);
    }
    m_maxSize.SetWidth(camXSize);

    endpoint = wxString::Format("camera/%ld/cameraysize", m_deviceNumber);
    int camYSize = 0;
    if (!m_client->GetInt(endpoint, &camYSize, &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca Camera: cannot get CameraYSize property, HTTP %ld\n", errorCode));
        return CamConnectFailed(wxString::Format(_("Alpaca Camera driver missing the %s property. Please report this error to your Alpaca driver provider."), "CameraYSize"));
    }
    m_maxSize.SetHeight(camYSize);

    m_swapAxes = false;

    // Get MaxADU for bits per pixel
    endpoint = wxString::Format("camera/%ld/maxadu", m_deviceNumber);
    int maxADU = 0;
    if (!m_client->GetInt(endpoint, &maxADU, &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca Camera: cannot get MaxADU property, HTTP %ld\n", errorCode));
        m_bitsPerPixel = 16; // assume 16 BPP
    }
    else
    {
        m_bitsPerPixel = maxADU <= 255 ? 8 : 16;
    }

    // Get the interface version of the driver
    m_driverVersion = 1;
    endpoint = wxString::Format("camera/%ld/interfaceversion", m_deviceNumber);
    int interfaceVersion = 1;
    if (m_client->GetInt(endpoint, &interfaceVersion, &errorCode))
    {
        m_driverVersion = interfaceVersion;
    }

    // Check if color sensor
    if (m_driverVersion > 1)
    {
        endpoint = wxString::Format("camera/%ld/sensortype", m_deviceNumber);
        int sensorType = 0; // 0 = Monochrome, 1 = Color, 2 = RGGB, etc.
        if (m_client->GetInt(endpoint, &sensorType, &errorCode) && sensorType > 1)
        {
            Color = true;
            HasBayer = true;
        }
    }

    // Get pixel size in microns (required property, like NINA reads)
    endpoint = wxString::Format("camera/%ld/pixelsizex", m_deviceNumber);
    double pixelSizeX = 0.0;
    if (!m_client->GetDouble(endpoint, &pixelSizeX, &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca Camera: cannot get PixelSizeX property, HTTP %ld\n", errorCode));
        return CamConnectFailed(wxString::Format(_("Alpaca Camera driver missing the %s property. Please report this error to your Alpaca driver provider."), "PixelSizeX"));
    }
    
    if (pixelSizeX <= 0.0)
    {
        Debug.Write(wxString::Format("Alpaca Camera: PixelSizeX is invalid (%.2f), must be > 0\n", pixelSizeX));
        return CamConnectFailed(_("Alpaca Camera driver returned invalid pixel size. Please check your camera driver configuration."));
    }
    
    m_driverPixelSize = pixelSizeX;
    Debug.Write(wxString::Format("Alpaca Camera: PixelSizeX = %.2f microns\n", pixelSizeX));

    endpoint = wxString::Format("camera/%ld/pixelsizey", m_deviceNumber);
    double pixelSizeY = 0.0;
    if (m_client->GetDouble(endpoint, &pixelSizeY, &errorCode))
    {
        if (pixelSizeY > 0.0)
        {
            // If we got PixelSizeY, use the maximum of X and Y
            m_driverPixelSize = wxMax(m_driverPixelSize, pixelSizeY);
            Debug.Write(wxString::Format("Alpaca Camera: PixelSizeY = %.2f microns, using max = %.2f microns\n", pixelSizeY, m_driverPixelSize));
        }
        else
        {
            Debug.Write(wxString::Format("Alpaca Camera: PixelSizeY is invalid (%.2f), using PixelSizeX value %.2f\n", pixelSizeY, m_driverPixelSize));
        }
    }
    else
    {
        // PixelSizeY is optional - if not available, use PixelSizeX
        Debug.Write(wxString::Format("Alpaca Camera: PixelSizeY property not available (HTTP %ld), using PixelSizeX value %.2f microns\n", errorCode, m_driverPixelSize));
    }
    
    Debug.Write(wxString::Format("Alpaca Camera: Final driver pixel size = %.2f microns\n", m_driverPixelSize));

    // Get max binning
    int maxBinX = 1, maxBinY = 1;
    endpoint = wxString::Format("camera/%ld/maxbinx", m_deviceNumber);
    if (m_client->GetInt(endpoint, &maxBinX, &errorCode))
    {
        // Got maxBinX
    }
    endpoint = wxString::Format("camera/%ld/maxbiny", m_deviceNumber);
    if (m_client->GetInt(endpoint, &maxBinY, &errorCode))
    {
        // Got maxBinY
    }
    MaxHwBinning = wxMin(maxBinX, maxBinY);
    Debug.Write(wxString::Format("Alpaca camera: MaxBinning is %hu\n", MaxHwBinning));
    if (HwBinning > MaxHwBinning)
        HwBinning = MaxHwBinning;
    m_curBin = HwBinning;

    // Set binning (only if not already 1, as 1 is typically the default and some servers don't allow setting it)
    // Use ASCOM standard parameter names: BinX and BinY (PascalCase)
    if (HwBinning != 1)
    {
        endpoint = wxString::Format("camera/%ld/binx", m_deviceNumber);
        wxString params = wxString::Format("BinX=%d", HwBinning);
        if (!m_client->Put(endpoint, params, JsonParser(), &errorCode))
        {
            Debug.Write(wxString::Format("Alpaca Camera: failed to set BinX, HTTP %ld\n", errorCode));
            return CamConnectFailed(_("The Alpaca camera failed to set binning. See the debug log for more information."));
        }
        endpoint = wxString::Format("camera/%ld/biny", m_deviceNumber);
        params = wxString::Format("BinY=%d", HwBinning);
        if (!m_client->Put(endpoint, params, JsonParser(), &errorCode))
        {
            Debug.Write(wxString::Format("Alpaca Camera: failed to set BinY, HTTP %ld\n", errorCode));
            return CamConnectFailed(_("The Alpaca camera failed to set binning. See the debug log for more information."));
        }
    }

    // Check for cooler
    HasCooler = false;
    endpoint = wxString::Format("camera/%ld/cooleron", m_deviceNumber);
    bool coolerOn = false;
    if (m_client->GetBool(endpoint, &coolerOn, &errorCode))
    {
        Debug.Write("Alpaca camera: has cooler\n");
        HasCooler = true;

        endpoint = wxString::Format("camera/%ld/cansetccdtemperature", m_deviceNumber);
        if (!m_client->GetBool(endpoint, &m_canSetCoolerTemperature, &errorCode))
        {
            Debug.Write(wxString::Format("Alpaca Camera: cannot get CanSetCCDTemperature property, HTTP %ld\n", errorCode));
            return CamConnectFailed(wxString::Format(_("Alpaca Camera driver missing the %s property. Please report this error to your Alpaca driver provider."), "CanSetCCDTemperature"));
        }

        endpoint = wxString::Format("camera/%ld/cangetcoolerpower", m_deviceNumber);
        if (!m_client->GetBool(endpoint, &m_canGetCoolerPower, &errorCode))
        {
            Debug.Write(wxString::Format("Alpaca Camera: cannot get CanGetCoolerPower property, HTTP %ld\n", errorCode));
            return CamConnectFailed(wxString::Format(_("Alpaca Camera driver missing the %s property. Please report this error to your Alpaca driver provider."), "CanGetCoolerPower"));
        }
    }
    else
    {
        if (errorCode == 1031)
        {
            wxString errorMsg = wxString::Format(_("Alpaca Camera: Device %ld reports not connected while querying CoolerOn on %s:%ld"),
                                                 m_deviceNumber, m_host, m_port);
            Debug.Write(errorMsg + "\n");
            return CamConnectFailed(errorMsg);
        }
        Debug.Write(wxString::Format("Alpaca camera: CoolerOn not available (error %ld) => assuming no cooler present\n", errorCode));
    }

    // defer defining FrameSize since it is not simply derivable from max size and binning
    FrameSize = UNDEFINED_FRAME_SIZE;
    m_roi = wxRect(); // reset ROI state in case we're reconnecting

    Connected = true;

    return false;
}

bool CameraAlpaca::Disconnect()
{
    if (!Connected)
    {
        Debug.Write("Alpaca camera: attempt to disconnect when not connected\n");
        return false;
    }

    if (m_client)
    {
        // Disconnect the device
        wxString endpoint = wxString::Format("camera/%ld/connected", m_deviceNumber);
        wxString params = "Connected=false";
        long errorCode = 0;
        m_client->Put(endpoint, params, JsonParser(), &errorCode);
        // Don't fail if disconnect fails - device might already be disconnected
    }

    Connected = false;
    ClearStatus();
    return false;
}

void CameraAlpaca::ShowPropertyDialog()
{
    CameraSetup();
}

void CameraAlpaca::CameraSetup()
{
    // show the server and device configuration
    AlpacaConfig alpacaDlg(wxGetApp().GetTopWindow(), _("Alpaca Camera Selection"), ALPACA_TYPE_CAMERA);
    alpacaDlg.m_host = m_host;
    alpacaDlg.m_port = m_port;
    alpacaDlg.m_deviceNumber = m_deviceNumber;

    // initialize with actual values
    alpacaDlg.SetSettings();

    if (alpacaDlg.ShowModal() == wxID_OK)
    {
        // if OK save the values to the current profile
        alpacaDlg.SaveSettings();
        m_host = alpacaDlg.m_host;
        m_port = alpacaDlg.m_port;
        m_deviceNumber = alpacaDlg.m_deviceNumber;
        pConfig->Profile.SetString("/alpaca/host", m_host);
        pConfig->Profile.SetLong("/alpaca/port", m_port);
        pConfig->Profile.SetLong("/alpaca/camera_device", m_deviceNumber);
        Name = wxString::Format("Alpaca Camera [%s:%ld/%ld]", m_host, m_port, m_deviceNumber);

        // Recreate client with new settings
        if (m_client)
        {
            delete m_client;
            m_client = nullptr;
        }
    }
}

bool CameraAlpaca::HasNonGuiCapture()
{
    return true;
}

bool CameraAlpaca::AbortExposure()
{
    if (!(m_canAbortExposure || m_canStopExposure))
        return false;

    wxString endpoint;
    if (m_canAbortExposure)
    {
        endpoint = wxString::Format("camera/%ld/abortexposure", m_deviceNumber);
    }
    else
    {
        endpoint = wxString::Format("camera/%ld/stopexposure", m_deviceNumber);
    }

    long errorCode = 0;
    bool result = m_client->PutAction(endpoint, m_canAbortExposure ? "AbortExposure" : "StopExposure", "", &errorCode);
    Debug.Write(wxString::Format("Alpaca_%s returns err = %d\n", m_canAbortExposure ? "AbortExposure" : "StopExposure", result));
    return !result;
}

bool CameraAlpaca::Capture(usImage& img, const CaptureParams& captureParams)
{
    int duration = captureParams.duration;
    int options = captureParams.captureOptions;
    bool takeSubframe = UseSubframes;
    wxRect roi(captureParams.subframe);

    if (roi.width <= 0 || roi.height <= 0)
    {
        takeSubframe = false;
    }

    bool binning_changed = false;
    if (HwBinning != m_curBin)
    {
        binning_changed = true;
        takeSubframe = false; // subframe may be out of bounds now
        if (HwBinning == 1)
            FrameSize.Set(m_maxSize.x, m_maxSize.y);
        else
            FrameSize = UNDEFINED_FRAME_SIZE; // we don't know the binned size until we get a frame
    }

    if (takeSubframe && FrameSize == UNDEFINED_FRAME_SIZE)
    {
        // if we do not know the full frame size, we cannot take a
        // subframe until we receive a full frame and get the frame size
        takeSubframe = false;
    }

    // Program the size
    if (!takeSubframe)
    {
        wxSize sz;
        if (FrameSize != UNDEFINED_FRAME_SIZE)
        {
            // we know the actual frame size
            sz = FrameSize;
        }
        else
        {
            // the max size divided by the binning may be larger than
            // the actual frame, but setting a larger size should
            // request the full binned frame which we want
            sz.Set(m_maxSize.x / HwBinning, m_maxSize.y / HwBinning);
        }
        roi = wxRect(sz);
    }

    // Set binning if changed
    // Use ASCOM standard parameter names: BinX and BinY (PascalCase)
    if (binning_changed)
    {
        wxString endpoint = wxString::Format("camera/%ld/binx", m_deviceNumber);
        wxString params = wxString::Format("BinX=%d", HwBinning);
        long errorCode = 0;
        if (!m_client->Put(endpoint, params, JsonParser(), &errorCode))
        {
            pFrame->Alert(_("The Alpaca camera failed to set binning. See the debug log for more information."));
            return true;
        }
        endpoint = wxString::Format("camera/%ld/biny", m_deviceNumber);
        params = wxString::Format("BinY=%d", HwBinning);
        if (!m_client->Put(endpoint, params, JsonParser(), &errorCode))
        {
            pFrame->Alert(_("The Alpaca camera failed to set binning. See the debug log for more information."));
            return true;
        }
        m_curBin = HwBinning;
    }

    // Set ROI if changed
    // Skip setting ROI if it's the full frame (0,0,width,height) as that's the default
    // Some Alpaca servers don't support setting ROI or have issues with full-frame ROI
    if (roi != m_roi)
    {
        bool isFullFrame = (roi.GetLeft() == 0 && roi.GetTop() == 0 && 
                           roi.GetWidth() == m_maxSize.x / HwBinning && 
                           roi.GetHeight() == m_maxSize.y / HwBinning);
        
        if (!isFullFrame)
        {
            // Only set ROI if it's not the full frame
            // Use ASCOM standard parameter names: StartX, StartY, NumX, NumY (PascalCase)
            wxString endpoint = wxString::Format("camera/%ld/startx", m_deviceNumber);
            wxString params = wxString::Format("StartX=%d", roi.GetLeft());
            long errorCode = 0;
            if (!m_client->Put(endpoint, params, JsonParser(), &errorCode))
            {
                Debug.Write(wxString::Format("Alpaca Camera: failed to set StartX, HTTP %ld (ROI may not be supported)\n", errorCode));
                // Don't fail capture if ROI setting fails - continue with full frame
            }

            endpoint = wxString::Format("camera/%ld/starty", m_deviceNumber);
            params = wxString::Format("StartY=%d", roi.GetTop());
            if (!m_client->Put(endpoint, params, JsonParser(), &errorCode))
            {
                Debug.Write(wxString::Format("Alpaca Camera: failed to set StartY, HTTP %ld (ROI may not be supported)\n", errorCode));
            }

            endpoint = wxString::Format("camera/%ld/numx", m_deviceNumber);
            params = wxString::Format("NumX=%d", roi.GetWidth());
            if (!m_client->Put(endpoint, params, JsonParser(), &errorCode))
            {
                Debug.Write(wxString::Format("Alpaca Camera: failed to set NumX, HTTP %ld (ROI may not be supported)\n", errorCode));
            }

            endpoint = wxString::Format("camera/%ld/numy", m_deviceNumber);
            params = wxString::Format("NumY=%d", roi.GetHeight());
            if (!m_client->Put(endpoint, params, JsonParser(), &errorCode))
            {
                Debug.Write(wxString::Format("Alpaca Camera: failed to set NumY, HTTP %ld (ROI may not be supported)\n", errorCode));
            }
        }
        
        m_roi = roi;
    }

    bool takeDark = HasShutter && ShutterClosed;

    // Start the exposure
    wxString startExposureEndpoint = wxString::Format("camera/%ld/startexposure", m_deviceNumber);
    wxString params = wxString::Format("Duration=%.3f&Light=%s", duration / 1000.0, takeDark ? "false" : "true");
    long errorCode = 0;
    if (!m_client->PutAction(startExposureEndpoint, "StartExposure", params, &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca_StartExposure failed, HTTP %ld\n", errorCode));
        pFrame->Alert(_("Alpaca error -- Cannot start exposure with given parameters"));
        return true;
    }

    CameraWatchdog watchdog(duration, GetTimeoutMs());

    if (duration > 100)
    {
        // wait until near end of exposure
        if (WorkerThread::MilliSleep(duration - 100, WorkerThread::INT_ANY) &&
            (WorkerThread::TerminateRequested() || AbortExposure()))
        {
            return true;
        }
    }

    while (true) // wait for image to finish and d/l
    {
        wxMilliSleep(20);
        bool ready = false;
        wxString imageReadyEndpoint = wxString::Format("camera/%ld/imageready", m_deviceNumber);
        if (!m_client->GetBool(imageReadyEndpoint, &ready, &errorCode))
        {
            Debug.Write(wxString::Format("Alpaca_ImageReady failed, HTTP %ld\n", errorCode));
            pFrame->Alert(_("Exception thrown polling camera"));
            return true;
        }
        if (ready)
            break;
        if (WorkerThread::InterruptRequested() && (WorkerThread::TerminateRequested() || AbortExposure()))
        {
            return true;
        }
        if (watchdog.Expired())
        {
            DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
            return true;
        }
    }

    // Get image array
    wxString imageArrayEndpoint = wxString::Format("camera/%ld/imagearray", m_deviceNumber);
    std::string responseBody;
    std::string contentType;
    if (!m_client->GetRaw(imageArrayEndpoint, "application/imagebytes, application/json", &responseBody, &contentType, &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca Camera: Failed to get image array, HTTP %ld\n", errorCode));
        pFrame->Alert(_("Error reading image"));
        return true;
    }
    Debug.Write(wxString::Format("Alpaca Camera: imagearray response content-type '%s', %lu bytes\n",
                                 contentType.c_str(), static_cast<unsigned long>(responseBody.size())));

    auto decode_imagebytes = [&](const std::string& payload) -> bool
    {
        if (payload.size() < kImageBytesMetadataSize)
        {
            Debug.Write("Alpaca Camera: ImageBytes response too small for metadata\n");
            pFrame->Alert(_("Error reading image"));
            return true;
        }

        std::uint32_t metadataVersion = 0;
        std::uint32_t errorNumber = 0;
        std::uint32_t dataStart = 0;
        std::uint32_t transmissionType = 0;
        std::uint32_t rank = 0;
        std::uint32_t width = 0;
        std::uint32_t height = 0;

        if (!ReadUInt32LE(payload, 0, &metadataVersion) ||
            !ReadUInt32LE(payload, 4, &errorNumber) ||
            !ReadUInt32LE(payload, 16, &dataStart) ||
            !ReadUInt32LE(payload, 24, &transmissionType) ||
            !ReadUInt32LE(payload, 28, &rank) ||
            !ReadUInt32LE(payload, 32, &width) ||
            !ReadUInt32LE(payload, 36, &height))
        {
            Debug.Write("Alpaca Camera: Failed reading ImageBytes metadata\n");
            pFrame->Alert(_("Error reading image"));
            return true;
        }

        if (metadataVersion != kImageBytesMetadataVersion)
        {
            Debug.Write(wxString::Format("Alpaca Camera: ImageBytes metadata version %u not supported\n", metadataVersion));
        }

        if (errorNumber != 0)
        {
            wxString errorMessage;
            if (dataStart < payload.size())
            {
                errorMessage = wxString(payload.substr(dataStart).c_str(), wxConvUTF8);
            }
            Debug.Write(wxString::Format("Alpaca Camera: ImageBytes error %u: %s\n", errorNumber, errorMessage));
            pFrame->Alert(_("Error reading image"));
            return true;
        }

        if (rank != 2 || width == 0 || height == 0)
        {
            Debug.Write("Alpaca Camera: ImageBytes unsupported rank or dimensions\n");
            pFrame->Alert(_("Error reading image"));
            return true;
        }

        std::size_t bytesPerElement = 0;
        switch (transmissionType)
        {
        case kImageTypeByte:
            bytesPerElement = 1;
            break;
        case kImageTypeUInt16:
        case kImageTypeInt16:
            bytesPerElement = 2;
            break;
        case kImageTypeInt32:
            bytesPerElement = 4;
            break;
        default:
            Debug.Write(wxString::Format("Alpaca Camera: ImageBytes unsupported transmission type %u\n", transmissionType));
            pFrame->Alert(_("Error reading image"));
            return true;
        }

        std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        std::size_t requiredBytes = dataStart + pixelCount * bytesPerElement;
        if (dataStart < kImageBytesMetadataSize || requiredBytes > payload.size())
        {
            Debug.Write("Alpaca Camera: ImageBytes payload truncated\n");
            pFrame->Alert(_("Error reading image"));
            return true;
        }

        if (takeSubframe)
        {
            if (FrameSize == UNDEFINED_FRAME_SIZE)
            {
                Debug.Write("internal error: taking subframe before full frame\n");
                pFrame->Alert(_("Error reading image"));
                return true;
            }

            if (img.Init(FrameSize))
            {
                pFrame->Alert(_("Memory allocation error"));
                return true;
            }

            img.Clear();
            img.Subframe = roi;
        }
        else
        {
            FrameSize.Set(width, height);
            if (img.Init(FrameSize))
            {
                pFrame->Alert(_("Memory allocation error"));
                return true;
            }
        }

        const unsigned char *ptr = reinterpret_cast<const unsigned char *>(payload.data() + dataStart);
        for (std::uint32_t x = 0; x < width; ++x)
        {
            for (std::uint32_t y = 0; y < height; ++y)
            {
                int value = 0;
                switch (transmissionType)
                {
                case kImageTypeByte:
                    value = static_cast<int>(*ptr);
                    ptr += 1;
                    break;
                case kImageTypeUInt16:
                {
                    std::uint16_t v = static_cast<std::uint16_t>(ptr[0]) |
                                      (static_cast<std::uint16_t>(ptr[1]) << 8);
                    value = static_cast<int>(v);
                    ptr += 2;
                    break;
                }
                case kImageTypeInt16:
                {
                    std::int16_t v = static_cast<std::int16_t>(ptr[0] | (ptr[1] << 8));
                    value = static_cast<int>(v);
                    ptr += 2;
                    break;
                }
                case kImageTypeInt32:
                default:
                    value = static_cast<int>(ptr[0]) |
                            (static_cast<int>(ptr[1]) << 8) |
                            (static_cast<int>(ptr[2]) << 16) |
                            (static_cast<int>(ptr[3]) << 24);
                    ptr += 4;
                    break;
                }

                if (takeSubframe)
                {
                    if (y >= static_cast<std::uint32_t>(roi.y) &&
                        y < static_cast<std::uint32_t>(roi.y + roi.height) &&
                        x >= static_cast<std::uint32_t>(roi.x) &&
                        x < static_cast<std::uint32_t>(roi.x + roi.width))
                    {
                        unsigned short *dataptr = img.ImageData + y * img.Size.GetWidth() + x;
                        *dataptr = static_cast<unsigned short>(value);
                    }
                }
                else
                {
                    unsigned short *dataptr = img.ImageData + y * img.Size.GetWidth() + x;
                    *dataptr = static_cast<unsigned short>(value);
                }
            }
        }

        return false;
    };

    bool imageBytesDecoded = false;
    if (IsContentTypeImageBytes(contentType))
    {
        Debug.Write("Alpaca Camera: ImageBytes response detected, decoding\n");
        if (decode_imagebytes(responseBody))
            return true;
        Debug.Write("Alpaca Camera: ImageBytes decode successful\n");
        imageBytesDecoded = true;
    }

    if (!imageBytesDecoded)
    {
        bool looksJson = IsContentTypeJson(contentType);
        if (!looksJson && !responseBody.empty())
        {
            char c = responseBody[0];
            looksJson = (c == '{' || c == '[');
        }
        if (!looksJson)
        {
            Debug.Write("Alpaca Camera: Unexpected response content type\n");
            pFrame->Alert(_("Error reading image"));
            return true;
        }

        Debug.Write("Alpaca Camera: Using JSON ImageArray response\n");
        JsonParser parser;
        if (!parser.Parse(responseBody))
        {
            Debug.Write(wxString::Format("Alpaca Camera: JSON parse error: %s\n", parser.ErrorDesc()));
            pFrame->Alert(_("Error reading image"));
            return true;
        }

        // Parse image array from JSON
        const json_value *root = parser.Root();
        if (!root || root->type != JSON_OBJECT)
        {
            Debug.Write("Alpaca Camera: Invalid image array response\n");
            pFrame->Alert(_("Error reading image"));
            return true;
        }

        // Find the Value array in the response
        const json_value *valueArray = nullptr;
        json_for_each(n, root)
        {
            if (n->name && strcmp(n->name, "Value") == 0 && n->type == JSON_ARRAY)
            {
                valueArray = n;
                break;
            }
        }

        if (!valueArray)
        {
            Debug.Write("Alpaca Camera: No Value array in response\n");
            pFrame->Alert(_("Error reading image"));
            return true;
        }

        // Get image dimensions from the array structure
        // Alpaca returns a 2D array: [[row1], [row2], ...]
        int imageHeight = 0;
        int imageWidth = 0;

        const json_value *firstRow = valueArray->first_child;
        if (firstRow && firstRow->type == JSON_ARRAY)
        {
            imageHeight = 0;
            const json_value *row = firstRow;
            while (row)
            {
                imageHeight++;
                if (imageHeight == 1)
                {
                    // Count elements in first row
                    const json_value *elem = row->first_child;
                    while (elem)
                    {
                        imageWidth++;
                        elem = elem->next_sibling;
                    }
                }
                row = row->next_sibling;
            }
        }

        if (imageWidth == 0 || imageHeight == 0)
        {
            Debug.Write("Alpaca Camera: Invalid image dimensions\n");
            pFrame->Alert(_("Error reading image"));
            return true;
        }

        // Check for axis swapping
        if (!takeSubframe && !m_swapAxes && imageWidth < imageHeight && m_maxSize.x > m_maxSize.y)
        {
            Debug.Write(wxString::Format("Alpaca camera: array axes are flipped (%dx%d) vs (%dx%d)\n", imageWidth, imageHeight, m_maxSize.x, m_maxSize.y));
            m_swapAxes = true;
        }

        if (m_swapAxes)
            std::swap(imageWidth, imageHeight);

        if (takeSubframe)
        {
            if (FrameSize == UNDEFINED_FRAME_SIZE)
            {
                // should never happen since we arranged not to take a subframe
                // unless full frame size is known
                Debug.Write("internal error: taking subframe before full frame\n");
                pFrame->Alert(_("Error reading image"));
                return true;
            }

            if (img.Init(FrameSize))
            {
                pFrame->Alert(_("Memory allocation error"));
                return true;
            }

            img.Clear();
            img.Subframe = roi;

            // Copy image data from JSON array - subframe case
            // Iterate over ROI region in the full image
            const json_value *row = valueArray->first_child;
            int y = 0;
            while (row && y < imageHeight)
            {
                if (y >= roi.y && y < roi.y + roi.height && row->type == JSON_ARRAY)
                {
                    const json_value *elem = row->first_child;
                    int x = 0;
                    unsigned short *dataptr = img.ImageData + (y - roi.y + roi.y) * img.Size.GetWidth() + roi.x;
                    while (elem && x < imageWidth)
                    {
                        if (x >= roi.x && x < roi.x + roi.width)
                        {
                            if (elem->type == JSON_INT)
                            {
                                *dataptr = static_cast<unsigned short>(elem->int_value);
                            }
                            else if (elem->type == JSON_FLOAT)
                            {
                                *dataptr = static_cast<unsigned short>(elem->float_value);
                            }
                            dataptr++;
                        }
                        elem = elem->next_sibling;
                        x++;
                    }
                }
                row = row->next_sibling;
                y++;
            }
        }
        else
        {
            FrameSize.Set(imageWidth, imageHeight);

            if (img.Init(FrameSize))
            {
                pFrame->Alert(_("Memory allocation error"));
                return true;
            }

            // Copy image data from JSON array
            const json_value *row = valueArray->first_child;
            int y = 0;
            while (row && y < imageHeight)
            {
                if (row->type == JSON_ARRAY)
                {
                    const json_value *elem = row->first_child;
                    int x = 0;
                    while (elem && x < imageWidth)
                    {
                        unsigned short *dataptr = img.ImageData + y * imageWidth + x;
                        if (elem->type == JSON_INT)
                        {
                            *dataptr = static_cast<unsigned short>(elem->int_value);
                        }
                        else if (elem->type == JSON_FLOAT)
                        {
                            *dataptr = static_cast<unsigned short>(elem->float_value);
                        }
                        elem = elem->next_sibling;
                        x++;
                    }
                }
                row = row->next_sibling;
                y++;
            }
        }
    }

    if (options & CAPTURE_SUBTRACT_DARK)
        SubtractDark(img);
    if ((options & CAPTURE_RECON) && HasBayer && captureParams.CombinedBinning() == 1)
        QuickLRecon(img);

    return false;
}

bool CameraAlpaca::ST4PulseGuideScope(int direction, int duration)
{
    if (!m_hasGuideOutput)
        return true;

    if (!pMount || !pMount->IsConnected())
        return false;

    // Start the motion (which may stop on its own)
    int alpacaDirection = -1;
    switch (direction)
    {
    case NORTH:
        alpacaDirection = 0;
        break;
    case SOUTH:
        alpacaDirection = 1;
        break;
    case EAST:
        alpacaDirection = 2;
        break;
    case WEST:
        alpacaDirection = 3;
        break;
    default:
        return true;
    }

    wxString endpoint = wxString::Format("camera/%ld/pulseguide", m_deviceNumber);
    wxString params = wxString::Format("Direction=%d&Duration=%d", alpacaDirection, duration);

    long errorCode = 0;
    if (!m_client->PutAction(endpoint, "PulseGuide", params, &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca Camera: PulseGuide failed, HTTP %ld\n", errorCode));
        return true;
    }

    MountWatchdog watchdog(duration, 5000);

    if (watchdog.Time() < duration) // likely returned right away and not after move - enter poll loop
    {
        while (true)
        {
            wxString isPulseGuidingEndpoint = wxString::Format("camera/%ld/ispulseguiding", m_deviceNumber);
            bool isMoving = false;
            long errorCode = 0;
            if (!m_client->GetBool(isPulseGuidingEndpoint, &isMoving, &errorCode))
            {
                Debug.Write(wxString::Format("Alpaca Camera: IsPulseGuiding failed, HTTP %ld\n", errorCode));
                pFrame->Alert(_("Alpaca driver failed checking IsPulseGuiding. See the debug log for more information."));
                return true;
            }
            if (!isMoving)
                break;
            wxMilliSleep(50);
            if (WorkerThread::TerminateRequested())
                return true;
            if (watchdog.Expired())
            {
                Debug.Write("Mount watchdog timed-out waiting for Alpaca_IsPulseGuiding to clear\n");
                return true;
            }
        }
    }

    return false;
}

bool CameraAlpaca::SetCoolerOn(bool on)
{
    if (!HasCooler)
    {
        Debug.Write("cam has no cooler!\n");
        return true; // error
    }

    if (!Connected)
    {
        Debug.Write("camera cannot set cooler on/off when not connected\n");
        return true;
    }

    wxString endpoint = wxString::Format("camera/%ld/cooleron", m_deviceNumber);
    wxString params = wxString::Format("CoolerOn=%s", on ? "true" : "false");
    long errorCode = 0;
    if (!m_client->Put(endpoint, params, JsonParser(), &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca error turning camera cooler %s, HTTP %ld\n", on ? "on" : "off", errorCode));
        pFrame->Alert(wxString::Format(_("Alpaca error turning camera cooler %s"), on ? _("on") : _("off")));
        return true;
    }

    return false;
}

bool CameraAlpaca::SetCoolerSetpoint(double temperature)
{
    if (!HasCooler || !m_canSetCoolerTemperature)
    {
        Debug.Write("camera cannot set cooler temperature\n");
        return true; // error
    }

    if (!Connected)
    {
        Debug.Write("camera cannot set cooler setpoint when not connected\n");
        return true;
    }

    wxString endpoint = wxString::Format("camera/%ld/setccdtemperature", m_deviceNumber);
    wxString params = wxString::Format("SetCCDTemperature=%.2f", temperature);
    long errorCode = 0;
    if (!m_client->Put(endpoint, params, JsonParser(), &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca error setting cooler setpoint, HTTP %ld\n", errorCode));
        return true;
    }

    return false;
}

bool CameraAlpaca::GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature)
{
    if (!HasCooler)
        return true; // error

    wxString endpoint = wxString::Format("camera/%ld/cooleron", m_deviceNumber);
    bool coolerOn = false;
    long errorCode = 0;
    if (!m_client->GetBool(endpoint, &coolerOn, &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca error getting CoolerOn property, HTTP %ld\n", errorCode));
        return true;
    }
    *on = coolerOn;

    endpoint = wxString::Format("camera/%ld/ccdtemperature", m_deviceNumber);
    double ccdTemp = 0.0;
    if (!m_client->GetDouble(endpoint, &ccdTemp, &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca error getting CCDTemperature property, HTTP %ld\n", errorCode));
        return true;
    }
    *temperature = ccdTemp;

    if (m_canSetCoolerTemperature)
    {
        endpoint = wxString::Format("camera/%ld/setccdtemperature", m_deviceNumber);
        double setTemp = 0.0;
        if (!m_client->GetDouble(endpoint, &setTemp, &errorCode))
        {
            Debug.Write(wxString::Format("Alpaca error getting SetCCDTemperature property, HTTP %ld\n", errorCode));
            return true;
        }
        *setpoint = setTemp;
    }
    else
        *setpoint = *temperature;

    if (m_canGetCoolerPower)
    {
        endpoint = wxString::Format("camera/%ld/coolerpower", m_deviceNumber);
        double coolerPower = 0.0;
        if (!m_client->GetDouble(endpoint, &coolerPower, &errorCode))
        {
            Debug.Write(wxString::Format("Alpaca error getting CoolerPower property, HTTP %ld\n", errorCode));
            return true;
        }
        *power = coolerPower;
    }
    else
        *power = 100.0;

    return false;
}

bool CameraAlpaca::GetSensorTemperature(double *temperature)
{
    wxString endpoint = wxString::Format("camera/%ld/ccdtemperature", m_deviceNumber);
    double ccdTemp = 0.0;
    long errorCode = 0;
    if (!m_client->GetDouble(endpoint, &ccdTemp, &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca error getting CCDTemperature property, HTTP %ld\n", errorCode));
        return true;
    }
    *temperature = ccdTemp;

    return false;
}

bool CameraAlpaca::ST4HasNonGuiMove()
{
    return true;
}

GuideCamera *AlpacaCameraFactory::MakeAlpacaCamera()
{
    return new CameraAlpaca();
}

#endif // ALPACA_CAMERA
