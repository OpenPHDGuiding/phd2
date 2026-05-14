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

enum DetectionModes
{
    modeBlob = 1,
    modeContours = 2
};
struct CentroidResult
{
    DetectionModes mode;
    float centroidX;
    float centroidY;
    int objectSize;
};
// Solar, lunar and planetary detection state and control class
class SolarSystemObject
{
private:
    // Solar system object guiding parameters
    bool m_paramEnabled;
    bool m_paramDetectionPaused;
    bool m_paramRoiEnabled;
    usImage *m_preProcessedImage;
    bool m_preProcessedImageValid;
    bool m_paramShowPreProcessed;
    double m_paramMinRadius;
    double m_paramMaxRadius;
    int m_paramLowThreshold;
    int m_paramHighThreshold;
    bool m_paramShowElementsButtonState;
    bool m_paramResamplingEnabled;

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
    std::vector<cv::Point> m_blobContour;
    CentroidResult m_lastCentroidResult;
    int m_frameWidth;
    int m_frameHeight;
    DetectionModes m_detectionMode;
    int m_guiderCadence;

    // TODO: Sort out the private/public mess
public:
    wxString m_statusMsg;
    bool m_detected;
    float m_center_x;
    float m_center_y;
    int m_radius;
    int m_searchRegion;
    float m_prevSearchRegion;
    double m_paramMinBlobDiameter;
    double m_paramMaxBlobDiameter;
    double m_paramBlobThreshold;
    bool m_paramInvertBlob;
    bool m_paramBlobAutoThreshold;
    int m_paramGuiderCadence;
    int m_savedGuiderCadence;

    bool m_roiActive;
    cv::Rect m_roiRect;
    bool m_userLClick;
    int m_clicked_x;
    int m_clicked_y;

    int m_detectionCounter;
    bool m_simulationZeroOffset;
    bool m_cameraSimulationRefPointValid;

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
    CentroidResult GetLastCentroidResult() { return m_lastCentroidResult; }

    DetectionModes GetDetectionMode() { return m_detectionMode; }
    void SetDetectionMode(DetectionModes mode);
    bool GetSolarSystemObjMode() { return m_paramEnabled; }
    bool GetDetectionPausedState() { return m_paramDetectionPaused; }
    void SetDetectionPausedState(bool paused) { m_paramDetectionPaused = paused; }
    double GetMinBlobDiameter() { return m_paramMinBlobDiameter; }
    double GetMaxBlobDiameter() { return m_paramMaxBlobDiameter; }
    double GetBlobThreshold() { return m_paramBlobThreshold; }
    double GetBlobAutoThreshold() { return m_paramBlobAutoThreshold; }
    usImage *GetPreProcessedImage() { return m_preProcessedImage; }
    bool PreProcessedImageValid() { return m_preProcessedImageValid; }
    bool ShowPreProcessedImage() { return m_paramShowPreProcessed; }
    void SetMinBlobDiameter(double val);
    void SetMaxBlobDiameter(double val);
    void SetBlobThreshold(double val);
    void SetBlobInversion(bool val);
    void SetBlobAutoThreshold(bool val);
    void SetMinRadius(double val);
    void SetShowPreProcessedImage(bool val);
    double GetMinRadius() { return m_paramMinRadius; }
    void SetMaxRadius(double val);
    double GetMaxRadius() { return m_paramMaxRadius; }
    void RefreshMinMaxDiameters() { m_showMinMaxDiameters = true; }
    bool GetRoiEnableState() { return m_paramRoiEnabled; }
    void SetRoiEnableState(bool enabled) { m_paramRoiEnabled = enabled; }
    void SetLowThreshold(int value);
    int GetLowThreshold() { return m_paramLowThreshold; }
    void SetHighThreshold(int value);
    int GetHighThreshold() { return m_paramHighThreshold; }
    int GetGuiderCadence() { return m_paramGuiderCadence; }
    void SetGuiderCadence(int cadenceMS);
    void SuspendGuiderCadence();
    void ResumeGuiderCadence();
    bool GetResamplingEnabled() { return m_paramResamplingEnabled; }
    void SetResamplingEnabled(bool Enabled);
    DescriptiveStats *m_distanceStats;
    double m_lastDistance;
    int m_resampleCount;
    int m_resampleReductionCount;
    bool m_retryingFind;
    int m_lostTargetEvents;
    int m_totalDetectionEvents;

    void ShowVisualElements(bool state);
    bool VisualElementsEnabled() { return m_showVisualElements; }
    void SetShowFeaturesButtonState(bool state) { m_paramShowElementsButtonState = state; }
    bool GetShowFeaturesButtonState() { return m_paramShowElementsButtonState; }
    void ResetDetectionStats();

    // Displaying visual aid for solar system object parameter tuning
    bool m_showMinMaxDiameters;
    void VisualHelper(wxDC& dc, Star primaryStar, double scaleFactor);
    void InitializeDetectionParams();

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
    bool FindContoursCentroid(cv::Mat img8, bool roiActive, cv::Point2f& clickedPoint, cv::Rect& roiRect, bool activeRoiLimits,
                              float distanceRoiMax, CentroidResult& rslt);
    bool FindBlobCentroid(cv::Mat testMat, int roiX, int roiY, CentroidResult& rslt, std::vector<cv::Point>& blobContour);
};

#endif // PLANETARY_INCLUDED
