/*
 *  cam_alpaca.cpp
 *  PHD Guiding
 *
 *  Created for Alpaca Server support
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
#include <curl/curl.h>
#include <sstream>
#include <vector>

class CameraAlpaca : public GuideCamera
{
private:
    AlpacaClient *m_client;
    wxString m_host;
    long m_port;
    long m_deviceNumber;
    wxSize m_maxSize;
    wxByte m_bitsPerPixel;
    double m_pixelSize;
    bool m_hasBayer;
    bool m_exposureInProgress;

    void ClearStatus();
    void CameraSetup();

public:
    CameraAlpaca();
    ~CameraAlpaca();

    bool Connect(const wxString& camId) override;
    bool Disconnect() override;
    bool HasNonGuiCapture() override;
    wxByte BitsPerPixel() override;
    bool GetDevicePixelSize(double *pixSize) override;
    void ShowPropertyDialog() override;

    bool Capture(int duration, usImage& img, int options, const wxRect& subframe) override;
};

CameraAlpaca::CameraAlpaca()
{
    ClearStatus();
    // load the values from the current profile
    m_host = pConfig->Profile.GetString("/alpaca/host", _T("localhost"));
    m_port = pConfig->Profile.GetLong("/alpaca/port", 6800);
    m_deviceNumber = pConfig->Profile.GetLong("/alpaca/camera_device", 0);
    Name = wxString::Format("Alpaca Camera [%s:%ld/%ld]", m_host, m_port, m_deviceNumber);
    PropertyDialogType = PROPDLG_ANY;
    HasSubframes = true;
    m_bitsPerPixel = 0;
    m_hasBayer = false;
    m_client = nullptr;
}

CameraAlpaca::~CameraAlpaca()
{
    Disconnect();
    if (m_client)
    {
        delete m_client;
    }
}

void CameraAlpaca::ClearStatus()
{
    Connected = false;
    m_maxSize = wxSize(0, 0);
    FrameSize = UNDEFINED_FRAME_SIZE;
    m_bitsPerPixel = 0;
    m_pixelSize = 0.0;
    m_hasBayer = false;
    m_exposureInProgress = false;
}

bool CameraAlpaca::Connect(const wxString& camId)
{
    // If not configured open the setup dialog
    if (m_host == _T("localhost") && m_port == 6800)
    {
        CameraSetup();
        // If still using defaults after setup, user probably cancelled - don't try to connect
        if (m_host == _T("localhost") && m_port == 6800)
        {
            Debug.Write("Alpaca Camera: Setup cancelled or not configured, skipping connection\n");
            return true; // Error - not configured
        }
    }

    if (Connected)
    {
        return false;
    }

    Debug.Write(wxString::Format("Alpaca Camera connecting to %s:%ld device %ld\n", m_host, m_port, m_deviceNumber));

    if (!m_client)
    {
        m_client = new AlpacaClient(m_host, m_port, m_deviceNumber);
    }

    // Check if device is connected
    wxString endpoint = wxString::Format("camera/%ld/connected", m_deviceNumber);
    bool connected = false;
    long errorCode = 0;
    if (!m_client->GetBool(endpoint, &connected, &errorCode))
    {
        wxString errorMsg;
        if (errorCode == 0)
        {
            // HTTP 0 means no response - network/connectivity issue
            errorMsg = wxString::Format(_("Alpaca Camera: Cannot reach server at %s:%ld. Please check:\n- The Alpaca server is running\n- The IP address and port are correct\n- Firewall is not blocking the connection\n- Network connectivity is working"), m_host, m_port);
        }
        else if (errorCode == 200)
        {
            // HTTP 200 but response parsing failed - could be authentication response or wrong format
            errorMsg = wxString::Format(_("Alpaca Camera: Server at %s:%ld returned an authentication response instead of camera API data for device %ld.\n\nThis usually means:\n- The Alpaca server has authentication enabled\n- A reverse proxy is intercepting requests\n- The server requires authentication for API access\n\nPlease check the server configuration to allow direct API access, or check the debug log for the actual response received."), m_host, m_port, m_deviceNumber);
        }
        else
        {
            // Got HTTP response but it was an error (4xx, 5xx, etc.)
            errorMsg = wxString::Format(_("Alpaca Camera: Failed to connect to %s:%ld - HTTP error %ld. Please check that the Alpaca server is running and device %ld exists."), m_host, m_port, errorCode, m_deviceNumber);
        }
        Debug.Write(errorMsg + "\n");
        pFrame->Alert(errorMsg);
        return true;
    }

    if (!connected)
    {
        // Try to connect
        wxString connectEndpoint = wxString::Format("camera/%ld/connected", m_deviceNumber);
        wxString params = "Connected=true";
        JsonParser parser;
        if (!m_client->Put(connectEndpoint, params, parser, &errorCode))
        {
            wxString errorMsg = wxString::Format(_("Alpaca Camera: Failed to connect device %ld on %s:%ld - HTTP error %ld"), m_deviceNumber, m_host, m_port, errorCode);
            Debug.Write(errorMsg + "\n");
            pFrame->Alert(errorMsg);
            return true;
        }
    }

    // Get camera properties
    wxString camTypeEndpoint = wxString::Format("camera/%ld/cameraxsize", m_deviceNumber);
    int camXSize = 0;
    if (m_client->GetInt(camTypeEndpoint, &camXSize, &errorCode))
    {
        wxString camYSizeEndpoint = wxString::Format("camera/%ld/cameraysize", m_deviceNumber);
        int camYSize = 0;
        if (m_client->GetInt(camYSizeEndpoint, &camYSize, &errorCode))
        {
            m_maxSize.Set(camXSize, camYSize);
            FrameSize = m_maxSize;
        }
    }

    // Get pixel size
    wxString pixelSizeEndpoint = wxString::Format("camera/%ld/pixelsizex", m_deviceNumber);
    double pixelSizeX = 0.0;
    if (m_client->GetDouble(pixelSizeEndpoint, &pixelSizeX, &errorCode))
    {
        m_pixelSize = pixelSizeX;
    }

    // Get bits per pixel
    wxString bitDepthEndpoint = wxString::Format("camera/%ld/bitdepth", m_deviceNumber);
    int bitDepth = 0;
    if (m_client->GetInt(bitDepthEndpoint, &bitDepth, &errorCode))
    {
        m_bitsPerPixel = static_cast<wxByte>(bitDepth);
    }

    // Check for Bayer pattern
    wxString sensorTypeEndpoint = wxString::Format("camera/%ld/sensortype", m_deviceNumber);
    int sensorType = 0; // 0 = Monochrome, 1 = Color, 2 = RGGB, etc.
    if (m_client->GetInt(sensorTypeEndpoint, &sensorType, &errorCode))
    {
        m_hasBayer = (sensorType > 0);
    }

    Connected = true;
    Debug.Write(wxString::Format("Alpaca Camera connected: %dx%d, %d bits, pixel size %.2f\n",
                                 m_maxSize.x, m_maxSize.y, m_bitsPerPixel, m_pixelSize));
    return false;
}

bool CameraAlpaca::Disconnect()
{
    if (!Connected || !m_client)
    {
        return false;
    }

    Debug.Write("Alpaca Camera disconnecting\n");

    // Disconnect the device
    wxString endpoint = wxString::Format("camera/%ld/connected", m_deviceNumber);
    wxString params = "Connected=false";
    long errorCode = 0;
    m_client->Put(endpoint, params, JsonParser(), &errorCode);

    ClearStatus();
    return false;
}

wxByte CameraAlpaca::BitsPerPixel()
{
    return m_bitsPerPixel;
}

bool CameraAlpaca::GetDevicePixelSize(double *pixSize)
{
    if (!Connected)
    {
        return true; // error
    }

    *pixSize = m_pixelSize;
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

bool CameraAlpaca::Capture(int duration, usImage& img, int options, const wxRect& subframeArg)
{
    if (!Connected || !m_client)
    {
        return true;
    }

    bool takeSubframe = UseSubframes && subframeArg.width > 0 && subframeArg.height > 0;
    wxRect subframe(subframeArg);

    if (!takeSubframe)
    {
        subframe = wxRect(m_maxSize);
    }

    // Start exposure
    wxString startExposureEndpoint = wxString::Format("camera/%ld/startexposure", m_deviceNumber);
    wxString params = wxString::Format("Duration=%.3f&Light=true", duration / 1000.0);
    long errorCode = 0;
    if (!m_client->PutAction(startExposureEndpoint, "StartExposure", params, &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca Camera: Failed to start exposure, HTTP %ld\n", errorCode));
        return true;
    }

    m_exposureInProgress = true;

    // Wait for exposure to complete
    CameraWatchdog watchdog(duration, GetTimeoutMs());
    bool imageReady = false;

    while (!imageReady)
    {
        wxMilliSleep(100);

        if (WorkerThread::TerminateRequested())
        {
            return true;
        }

        if (watchdog.Expired())
        {
            Debug.Write("Alpaca Camera: Exposure timeout\n");
            return true;
        }

        // Check if image is ready
        wxString imageReadyEndpoint = wxString::Format("camera/%ld/imageready", m_deviceNumber);
        if (m_client->GetBool(imageReadyEndpoint, &imageReady, &errorCode))
        {
            if (imageReady)
            {
                break;
            }
        }
        else
        {
            // If request failed, log it but continue polling (might be temporary)
            if (errorCode != 0)
            {
                Debug.Write(wxString::Format("Alpaca Camera: imageready check failed, HTTP %ld, continuing to poll\n", errorCode));
            }
        }
    }

    m_exposureInProgress = false;

    // Get image array
    wxString imageArrayEndpoint = wxString::Format("camera/%ld/imagearray", m_deviceNumber);
    JsonParser parser;
    if (!m_client->Get(imageArrayEndpoint, parser, &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca Camera: Failed to get image array, HTTP %ld\n", errorCode));
        return true;
    }

    // Parse image array from JSON
    const json_value *root = parser.Root();
    if (!root || root->type != JSON_OBJECT)
    {
        Debug.Write("Alpaca Camera: Invalid image array response\n");
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
        return true;
    }

    // Update frame size if needed
    if (FrameSize == UNDEFINED_FRAME_SIZE || FrameSize.x != imageWidth || FrameSize.y != imageHeight)
    {
        FrameSize.Set(imageWidth, imageHeight);
        if (m_maxSize.x == 0 || m_maxSize.y == 0)
        {
            m_maxSize = FrameSize;
        }
    }

    // Initialize image
    if (takeSubframe)
    {
        if (img.Init(FrameSize))
        {
            return true;
        }
        img.Clear();
        img.Subframe = subframe;
    }
    else
    {
        if (img.Init(FrameSize))
        {
            return true;
        }
        img.Clear();
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
                if (takeSubframe)
                {
                    if (x >= subframe.x && x < subframe.x + subframe.width &&
                        y >= subframe.y && y < subframe.y + subframe.height)
                    {
                        int imgX = x - subframe.x;
                        int imgY = y - subframe.y;
                        unsigned short *dataptr = img.ImageData + (imgY + subframe.y) * img.Size.GetWidth() + (imgX + subframe.x);
                        if (elem->type == JSON_INT)
                        {
                            *dataptr = static_cast<unsigned short>(elem->int_value);
                        }
                        else if (elem->type == JSON_FLOAT)
                        {
                            *dataptr = static_cast<unsigned short>(elem->float_value);
                        }
                    }
                }
                else
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
                }
                elem = elem->next_sibling;
                x++;
            }
        }
        row = row->next_sibling;
        y++;
    }

    if (options & CAPTURE_SUBTRACT_DARK)
    {
        SubtractDark(img);
    }

    if (m_hasBayer && (options & CAPTURE_RECON))
    {
        QuickLRecon(img);
    }

    return false;
}

GuideCamera *AlpacaCameraFactory::MakeAlpacaCamera()
{
    return new CameraAlpaca();
}

#endif // ALPACA_CAMERA

