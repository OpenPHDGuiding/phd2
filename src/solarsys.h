/*
 *  solarsys.h
 *  PHD Guiding
 *
 *  Solar, lunar and planetary detection extensions created by Leo Shatz
 *  Copyright (c) 2023-2024 PHD2 Developers
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
 *    Neither the name of openphdguiding.org nor the names of its
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

#ifndef PLANETARY_INCLUDED

#define PLANETARY_INCLUDED

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

// Solar, lunar and planetary detection state and control class
class SolarSystemObject
{
private:
    // Solar system object guiding parameters
    bool m_paramEnabled;
    bool m_paramDetectionPaused;
    bool m_paramRoiEnabled;

    double m_paramMinRadius;
    double m_paramMaxRadius;
    int m_paramLowThreshold;
    int m_paramHighThreshold;
    bool m_paramShowElementsButtonState;

    bool m_showVisualElements;
    bool m_prevCaptureActive;
    bool m_measuringSharpnessMode;
    bool m_unknownHFD;
    double m_focusSharpness;
    int m_starProfileSize;

    float m_eccentricity;
    float m_angle;

    wxMutex m_syncLock;
    cv::Point2f m_prevClickedPoint;

    std::vector<cv::Point2f> m_diskContour;
    int m_centoid_x;
    int m_centoid_y;
    int m_sm_circle_x;
    int m_sm_circle_y;
    int m_frameWidth;
    int m_frameHeight;

public:
    wxString m_statusMsg;
    bool m_detected;
    float m_center_x;
    float m_center_y;
    int m_radius;
    int m_searchRegion;
    float m_prevSearchRegion;

    bool m_roiActive;
    cv::Rect m_roiRect;
    bool m_userLClick;
    int m_clicked_x;
    int m_clicked_y;

    int m_detectionCounter;
    bool m_simulationZeroOffset;
    bool m_cameraSimulationRefPointValid;

    // PHD2 parameters saved before enabling solar system object mode and restored
    // after disabling
    bool m_phd2_MassChangeThresholdEnabled;
    bool m_phd2_UseSubframes;
    bool m_phd2_MultistarEnabled;

public:
    SolarSystemObject();
    ~SolarSystemObject();

    bool FindDisk(const usImage *image, bool autoSelect, Star *pDisk);
    bool AutoFindDisk(const usImage& image, Star *pDisk);
    // Calcular SNR, peak value and mass of a solar system object
    double CalcPlanetMetrics(const usImage *pImg, int center_x, int center_y, int radius, int annulusWidth, Star *pDisk);
    bool FindSolarSystemObject(const usImage *pImage, bool autoSelect);
    double GetHFD();
    wxString GetHfdLabel();
    bool IsPixelMetrics();
    void ToggleSharpness();
    void GetDetectionStatus(wxString& statusMsg);
    bool UpdateCaptureState(bool CaptureActive);

    bool Get_SolarSystemObjMode() { return m_paramEnabled; }
    void Set_SolarSystemObjMode(bool enabled) { m_paramEnabled = enabled; }
    bool GetDetectionPausedState() { return m_paramDetectionPaused; }
    void SetDetectionPausedState(bool paused) { m_paramDetectionPaused = paused; }
    void Set_minRadius(double val);
    double Get_minRadius() { return m_paramMinRadius; }
    void Set_maxRadius(double val);
    double Get_maxRadius() { return m_paramMaxRadius; }
    bool GetRoiEnableState() { return m_paramRoiEnabled; }
    void SetRoiEnableState(bool enabled) { m_paramRoiEnabled = enabled; }
    void Set_lowThreshold(int value);
    int Get_lowThreshold() { return m_paramLowThreshold; }
    void Set_highThreshold(int value);
    int Get_highThreshold() { return m_paramHighThreshold; }

    void ShowVisualElements(bool state);
    bool VisualElementsEnabled() { return m_showVisualElements; }
    void SetShowFeaturesButtonState(bool state) { m_paramShowElementsButtonState = state; }
    bool GetShowFeaturesButtonState() { return m_paramShowElementsButtonState; }

public:
    // Displaying visual aid for solar system object parameter tuning
    bool m_showMinMaxDiameters;
    void RefreshMinMaxDiameters() { m_showMinMaxDiameters = true; }
    void VisualHelper(wxDC& dc, Star primaryStar, double scaleFactor);
    void RestoreDetectionParams();

private:
    wxStopWatch m_SolarSystemObjWatchdog;
    typedef struct
    {
        float x;
        float y;
        float radius;
    } CircleDescriptor;
    struct LineParameters
    {
        bool valid;
        bool vertical;
        float slope;
        float b;
    } m_DiameterLineParameters;
    typedef struct WeightedCircle
    {
        float x;
        float y;
        float r;
        float score;
    } WeightedCircle;

private:
    double ComputeSobelSharpness(const cv::Mat& img);
    double CalcSharpness(cv::Mat& FullFrame, cv::Point2f& clickedPoint, bool detectionResult);
    void CalcLineParams(CircleDescriptor p1, CircleDescriptor p2);
    int RefineDiskCenter(float& bestScore, CircleDescriptor& diskCenter, std::vector<cv::Point2f>& diskContour, int minRadius,
                         int maxRadius, float searchRadius, float resolution = 1.0);
    float FindContourCenter(CircleDescriptor& diskCenter, CircleDescriptor& smallestCircle,
                            std::vector<cv::Point2f>& bestContourVector, cv::Moments& mu, int minRadius, int maxRadius);
    void FindCenters(cv::Mat image, const std::vector<cv::Point>& contour, CircleDescriptor& bestCentroid,
                     CircleDescriptor& smallestCircle, std::vector<cv::Point2f>& bestContour, cv::Moments& mu, int minRadius,
                     int maxRadius);
    bool FindOrbisCenter(cv::Mat img8, int minRadius, int maxRadius, bool roiActive, cv::Point2f& clickedPoint,
                         cv::Rect& roiRect, bool activeRoiLimits, float distanceRoiMax);
};

#endif // PLANETARY_INCLUDED
