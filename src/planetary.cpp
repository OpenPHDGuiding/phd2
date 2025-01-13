/*
 *  planetary.cpp
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

#include "phd.h"
#include "planetary.h"
#include "planetary_tool.h"

#include <algorithm>
#include <numeric>

#if ((wxMAJOR_VERSION < 3) && (wxMINOR_VERSION < 9))
# define wxPENSTYLE_DOT wxDOT
#endif

// Using OpenCV namespace
using namespace cv;

// Gaussian weights lookup table
#define GAUSSIAN_SIZE 2000
static float gaussianWeight[GAUSSIAN_SIZE];

// Initialize solar/planetary detection module
SolarSystemObject::SolarSystemObject()
{
    m_paramEnabled = false;
    m_paramDetectionPaused = false;
    m_paramRoiEnabled = false;

    m_prevCaptureActive = false;
    m_detected = false;
    m_radius = 0;
    m_searchRegion = 0;
    m_prevSearchRegion = 0;
    m_starProfileSize = 50;
    m_measuringSharpnessMode = false;
    m_unknownHFD = true;
    m_focusSharpness = 0;

    m_origPoint = Point2f(0, 0);
    m_cameraSimulationMove = Point2f(0, 0);
    m_cameraSimulationRefPoint = Point2f(0, 0);
    m_cameraSimulationRefPointValid = false;
    m_simulationZeroOffset = false;
    m_center_x = m_center_y = 0;

    m_userLClick = false;
    m_roiActive = false;
    m_detectionCounter = 0;
    m_clicked_x = 0;
    m_clicked_y = 0;
    m_prevClickedPoint = Point2f(0, 0);
    m_diskContour.clear();
    m_showVisualElements = false;
    m_showMinMaxDiameters = false;
    m_frameWidth = 0;
    m_frameHeight = 0;
    m_eccentricity = 0;
    m_angle = 0;

    // Build gaussian weighting function table used for circle feature detection
    float sigma = 1.0;
    memset(gaussianWeight, 0, sizeof(gaussianWeight));
    for (double x = 0; x < 20; x += 0.01)
    {
        int i = x * 100 + 0.5;
        if (i < GAUSSIAN_SIZE)
            gaussianWeight[i] += exp(-(pow(x, 2) / (2 * pow(sigma, 2))));
    }

    // Enforce valid range limits on solar system object detection parameters
    // while restoring from configuration
    m_paramMinRadius = pConfig->Profile.GetInt("/PlanetTool/min_radius", PT_MIN_RADIUS_DEFAULT);
    m_paramMinRadius = wxMax(PT_RADIUS_MIN, wxMin(PT_RADIUS_MAX, m_paramMinRadius));
    m_paramMaxRadius = pConfig->Profile.GetInt("/PlanetTool/max_radius", PT_MAX_RADIUS_DEFAULT);
    m_paramMaxRadius = wxMax(PT_RADIUS_MIN, wxMin(PT_RADIUS_MAX, m_paramMaxRadius));
    m_paramLowThreshold = pConfig->Profile.GetInt("/PlanetTool/high_threshold", PT_HIGH_THRESHOLD_DEFAULT) / 2;
    m_paramLowThreshold = wxMax(PT_THRESHOLD_MIN, wxMin(PT_LOW_THRESHOLD_MAX, m_paramLowThreshold));
    m_paramHighThreshold = pConfig->Profile.GetInt("/PlanetTool/high_threshold", PT_HIGH_THRESHOLD_DEFAULT);
    m_paramHighThreshold = wxMax(PT_THRESHOLD_MIN, wxMin(PT_HIGH_THRESHOLD_MAX, m_paramHighThreshold));

    // Save PHD2 settings we change for solar system object guiding
    m_phd2_MassChangeThresholdEnabled = pConfig->Profile.GetBoolean("/guider/onestar/MassChangeThresholdEnabled", false);
    m_phd2_UseSubframes = pConfig->Profile.GetBoolean("/camera/UseSubframes", false);
    m_phd2_MultistarEnabled = pConfig->Profile.GetBoolean("/guider/multistar/enabled", true);

    // Remove the alert dialog setting for pausing solar/planetary detection
    pConfig->Global.DeleteEntry(PausePlanetDetectionAlertEnabledKey());
}

SolarSystemObject::~SolarSystemObject()
{
    // Save all detection parameters
    pConfig->Profile.SetInt("/PlanetTool/min_radius", Get_minRadius());
    pConfig->Profile.SetInt("/PlanetTool/max_radius", Get_maxRadius());
    pConfig->Profile.SetInt("/PlanetTool/high_threshold", Get_highThreshold());
    pConfig->Flush();
}

// Report detected object size or sharpness depending on measurement mode
double SolarSystemObject::GetHFD()
{
    if (m_unknownHFD)
        return std::nan("1");
    if (m_measuringSharpnessMode)
        return m_focusSharpness;
    else
        return m_detected ? m_radius : 0;
}

wxString SolarSystemObject::GetHfdLabel()
{
    if (m_measuringSharpnessMode)
        return _("SHARPNESS: ");
    else
        return _("RADIUS: ");
}

bool SolarSystemObject::IsPixelMetrics()
{
    return Get_SolarSystemObjMode() ? !m_measuringSharpnessMode : true;
}

// Toggle between sharpness and radius display
void SolarSystemObject::ToggleSharpness()
{
    m_measuringSharpnessMode = !m_measuringSharpnessMode;
    m_unknownHFD = true;
}

// The Sobel operator can be used to detect edges in an image, which are more
// pronounced in focused images. You can apply the Sobel operator to the image
// and calculate the sum or mean of the absolute values of the gradients.
double SolarSystemObject::ComputeSobelSharpness(const Mat& img)
{
    Mat grad_x, grad_y;
    Sobel(img, grad_x, CV_32F, 1, 0);
    Sobel(img, grad_y, CV_32F, 0, 1);

    Mat grad;
    magnitude(grad_x, grad_y, grad);

    double sharpness = cv::mean(grad)[0];
    return sharpness;
}

// Calculate focus metrics around the updated tracked position
double SolarSystemObject::CalcSharpness(Mat& FullFrame, Point2f& clickedPoint, bool detectionResult)
{
    double scaleFactor;
    cv::Scalar meanSignal;
    Mat focusRoi;
    int focusX;
    int focusY;

    if (detectionResult)
    {
        focusX = m_center_x;
        focusY = m_center_y;
    }
    else if (norm(clickedPoint))
    {
        focusX = clickedPoint.x;
        focusY = clickedPoint.y;
    }
    else
    {
        // Compute scaling factor to normalize the signal
        meanSignal = cv::mean(focusRoi);
        scaleFactor = meanSignal[0] ? (65536.0 / 256) / meanSignal[0] : 1.0;

        // For failed auto selected star use entire frame for sharpness calculation
        FullFrame.convertTo(focusRoi, CV_32F, scaleFactor);
        return ComputeSobelSharpness(focusRoi);
    }

    const int focusSize = m_paramMaxRadius * 3 / 2.0;
    focusX = wxMax(0, focusX - focusSize / 2);
    focusX = wxMax(0, wxMin(focusX, m_frameWidth - focusSize));
    focusY = wxMax(0, focusY - focusSize / 2);
    focusY = wxMax(0, wxMin(focusY, m_frameHeight - focusSize));
    Rect focusSubFrame = Rect(focusX, focusY, focusSize, focusSize);
    focusRoi = FullFrame(focusSubFrame);

    meanSignal = cv::mean(focusRoi);
    scaleFactor = meanSignal[0] ? (65536.0 / 256) / meanSignal[0] : 1.0;

    focusRoi.convertTo(focusRoi, CV_32F, scaleFactor);
    return ComputeSobelSharpness(focusRoi);
}

// Get current detection status
void SolarSystemObject::GetDetectionStatus(wxString& statusMsg)
{
    statusMsg = wxString::Format(_("Object at (%.1f, %.1f) radius=%d"), m_center_x, m_center_y, m_radius);
}

// Update state used to visualize internally detected features
void SolarSystemObject::ShowVisualElements(bool state)
{
    m_syncLock.Lock();
    m_diskContour.clear();
    m_showVisualElements = state;
    m_syncLock.Unlock();
}

// Notification callback when PHD2 may change CaptureActive state
bool SolarSystemObject::UpdateCaptureState(bool CaptureActive)
{
    bool need_update = false;
    if (m_prevCaptureActive != CaptureActive)
    {
        if (!CaptureActive)
        {
            // Clear selection symbols (green circle/target lock) and visual elements
            if (Get_SolarSystemObjMode())
            {
                ShowVisualElements(false);
                pFrame->pGuider->Reset(false);
            }
            need_update = true;
        }
        else
        {
            // In solar/planetary mode update the state used to
            // control drawing of the internal detection elements.
            if (Get_SolarSystemObjMode() && GetShowFeaturesButtonState())
                ShowVisualElements(true);
            RestartSimulatorErrorDetection();
        }
    }

    // Reset the detection paused state if guiding has been cancelled
    if (!pFrame->pGuider->IsGuiding())
    {
        SetDetectionPausedState(false);
    }

    m_prevCaptureActive = CaptureActive;
    return need_update;
}

// Notification callback when camera is connected/disconnected
void SolarSystemObject::NotifyCameraConnect(bool connected)
{
    bool isSimCam = (pCamera && pCamera->Name == "Simulator");
    pFrame->pStatsWin->ShowSimulatorStats(isSimCam && connected);
    pFrame->pStatsWin->ShowPlanetStats(Get_SolarSystemObjMode() && connected);
    m_userLClick = false;
}

void SolarSystemObject::SaveCameraSimulationMove(double rx, double ry)
{
    m_cameraSimulationMove = Point2f(rx, ry);
    if (m_simulationZeroOffset)
    {
        m_cameraSimulationRefPoint = m_cameraSimulationMove;
        m_cameraSimulationRefPointValid = true;
    }
}

void SolarSystemObject::RestartSimulatorErrorDetection()
{
    m_cameraSimulationRefPointValid = false;
    m_simulationZeroOffset = true;
}

// Helper for visualizing detection radius and internal features
void SolarSystemObject::VisualHelper(wxDC& dc, Star primaryStar, double scaleFactor)
{
    // Do nothin if not in solar/planetary mode or no visual elements are enabled
    if (!Get_SolarSystemObjMode() || !m_showMinMaxDiameters && !VisualElementsEnabled())
        return;

    // Clip drawing region to displayed image frame
    wxImage *pImg = pFrame->pGuider->DisplayedImage();
    if (pImg)
        dc.SetClippingRegion(wxRect(0, 0, pImg->GetWidth(), pImg->GetHeight()));

    // Make sure to use transparent brush
    dc.SetBrush(*wxTRANSPARENT_BRUSH);

    // Display internally detected elements (must be enabled in UI)
    if (VisualElementsEnabled())
    {
        m_syncLock.Lock();

        // Draw contour points in solar/planetary mode
        if (m_diskContour.size())
        {
            dc.SetPen(wxPen(wxColour(230, 0, 0), 2, wxPENSTYLE_SOLID));
            for (const Point2f& contourPoint : m_diskContour)
                dc.DrawCircle((contourPoint.x + m_roiRect.x) * scaleFactor, (contourPoint.y + m_roiRect.y) * scaleFactor, 2);
        }

        m_syncLock.Unlock();
    }

    // Reset clipping region (don't clip min/max circles)
    dc.DestroyClippingRegion();

    // Display min/max diameters for visual feedback
    if (m_showMinMaxDiameters)
    {
        m_showMinMaxDiameters = false;
        if (pFrame->CaptureActive)
        {
            const wxString labelTextMin("min diameter");
            const wxString labelTextMax("max diameter");
            int x = int(primaryStar.X * scaleFactor + 0.5);
            int y = int(primaryStar.Y * scaleFactor + 0.5);
            int radius = int(m_radius * scaleFactor + 0.5);
            float minRadius = Get_minRadius() * scaleFactor;
            float maxRadius = Get_maxRadius() * scaleFactor;
            int minRadius_x = x + minRadius;
            int maxRadius_x = x + maxRadius;
            int lineMin_x = x;
            int lineMax_x = x;

            // Center the elements at the tracking point
            if (m_detected)
            {
                minRadius_x = maxRadius_x = x;
                lineMin_x -= minRadius;
                lineMax_x -= maxRadius;
            }

            // Draw min and max diameters legends
            dc.SetPen(wxPen(wxColour(230, 130, 30), 1, wxPENSTYLE_DOT));
            dc.SetTextForeground(wxColour(230, 130, 30));
            dc.DrawLine(lineMin_x, y - 5, lineMin_x + minRadius * 2, y - 5);
            dc.DrawCircle(minRadius_x, y, minRadius);
            dc.DrawText(labelTextMin, minRadius_x - dc.GetTextExtent(labelTextMin).GetWidth() / 2,
                        y - 10 - dc.GetTextExtent(labelTextMin).GetHeight());

            dc.SetPen(wxPen(wxColour(130, 230, 30), 1, wxPENSTYLE_DOT));
            dc.SetTextForeground(wxColour(130, 230, 30));
            dc.DrawLine(lineMax_x, y + 5, lineMax_x + maxRadius * 2, y + 5);
            dc.DrawCircle(maxRadius_x, y, maxRadius);
            dc.DrawText(labelTextMax, maxRadius_x - dc.GetTextExtent(labelTextMax).GetWidth() / 2, y + 5);
        }
    }
}

void SolarSystemObject::CalcLineParams(CircleDescriptor p1, CircleDescriptor p2)
{
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    if ((p1.radius == 0) || (p2.radius == 0) || (dx * dx + dy * dy < 3))
    {
        m_DiameterLineParameters.valid = false;
        m_DiameterLineParameters.vertical = false;
        m_DiameterLineParameters.slope = 0;
        m_DiameterLineParameters.b = 0;
        return;
    }
    // Check to see if line is vertical
    if (fabs(p1.x - p2.x) < 1)
    {
        // Vertical line, slope is undefined
        m_DiameterLineParameters.valid = true;
        m_DiameterLineParameters.vertical = true;
        m_DiameterLineParameters.slope = std::numeric_limits<double>::infinity();
        m_DiameterLineParameters.b = 0;
    }
    else
    {
        // Calculate slope (m) and y-intercept (b) for a non-vertical line
        m_DiameterLineParameters.valid = true;
        m_DiameterLineParameters.vertical = false;
        m_DiameterLineParameters.slope = (p2.y - p1.y) / (p2.x - p1.x);
        m_DiameterLineParameters.b = p1.y - (m_DiameterLineParameters.slope * p1.x);
    }
}

// Calculate score for given point
static float CalcContourScore(float& radius, Point2f pointToMeasure, std::vector<Point2f>& diskContour, int minRadius,
                              int maxRadius)
{
    std::vector<float> distances;
    distances.reserve(diskContour.size());
    float minIt = FLT_MAX;
    float maxIt = FLT_MIN;

    for (const auto& contourPoint : diskContour)
    {
        float distance = norm(contourPoint - pointToMeasure);
        if (distance >= minRadius && distance <= maxRadius)
        {
            minIt = wxMin(minIt, distance);
            maxIt = wxMax(maxIt, distance);
            distances.push_back(distance);
        }
    }

    // Note: calculating histogram on 0-sized data can crash the application.
    // Reject small sets of points as they usually aren't related to the features
    // we are looking for.
    if (distances.size() < 16)
    {
        radius = 0;
        return 0;
    }

    // Calculate the number of bins
    int bins = int(std::sqrt(distances.size()) + 0.5) | 1;
    float range[] = { std::floor(minIt), std::ceil(maxIt) };
    const float *histRange[] = { range };

    // Calculate the histogram
    Mat hist;
    Mat distData(distances); // Use vector directly to create Mat object
    cv::calcHist(&distData, 1, nullptr, Mat(), hist, 1, &bins, histRange, true, false);

    // Find the peak of the histogram
    double max_value;
    Point max_loc;
    cv::minMaxLoc(hist, nullptr, &max_value, nullptr, &max_loc);
    int max_idx = max_loc.y;

    // Middle of the bin
    float peakDistance = range[0] + (max_idx + 0.5) * ((range[1] - range[0]) / bins);

    float scorePoints = 0;
    for (float distance : distances)
    {
        int index = fabs(distance - peakDistance) * 100 + 0.5;
        if (index < GAUSSIAN_SIZE)
            scorePoints += gaussianWeight[index];
    }

    // Normalize score by total number points in the contour
    radius = peakDistance;
    return scorePoints / diskContour.size();
}

class AsyncCalcScoreThread : public wxThread
{
public:
    std::vector<Point2f> points;
    std::vector<Point2f> contour;
    Point2f center;
    float radius;
    float threadBestScore;
    int minRadius;
    int maxRadius;

public:
    AsyncCalcScoreThread(float bestScore, std::vector<Point2f>& diskContour, std::vector<Point2f>& workLoad, int min_radius,
                         int max_radius)
        : wxThread(wxTHREAD_JOINABLE), threadBestScore(bestScore), contour(diskContour), points(workLoad),
          minRadius(min_radius), maxRadius(max_radius)
    {
        radius = 0;
    }
    // A thread function to run HoughCircles method
    wxThread::ExitCode Entry()
    {
        for (const Point2f& point : points)
        {
            float score = ::CalcContourScore(radius, point, contour, minRadius, maxRadius);
            if (score > threadBestScore)
            {
                threadBestScore = score;
                radius = radius;
                center.x = point.x;
                center.y = point.y;
            }
        }
        return this;
    }
};

/* Find best circle candidate */
int SolarSystemObject::RefineDiskCenter(float& bestScore, CircleDescriptor& diskCenter, std::vector<Point2f>& diskContour,
                                        int minRadius, int maxRadius, float searchRadius, float resolution)
{
    const int maxWorkloadSize = 256;
    const Point2f center = { diskCenter.x, diskCenter.y };
    std::vector<AsyncCalcScoreThread *> threads;

    // Check all points within small circle for search of higher score
    int threadCount = 0;
    bool useThreads = true;
    int workloadSize = 0;
    std::vector<Point2f> workload;
    workload.reserve(maxWorkloadSize);
    for (float x = diskCenter.x - searchRadius; x < diskCenter.x + searchRadius; x += resolution)
        for (float y = diskCenter.y - searchRadius; y < diskCenter.y + searchRadius; y += resolution)
        {
            Point2f pointToMeasure = { x, y };
            float dist = norm(pointToMeasure - center);
            if (dist > searchRadius)
                continue;

            // When finished creating a workload, create and run new processing thread
            if (useThreads && (workloadSize++ >= maxWorkloadSize))
            {
                AsyncCalcScoreThread *thread = new AsyncCalcScoreThread(bestScore, diskContour, workload, minRadius, maxRadius);
                if ((thread->Create() == wxTHREAD_NO_ERROR) && (thread->Run() == wxTHREAD_NO_ERROR))
                {
                    threads.push_back(thread);
                    workload.clear();
                    workloadSize = 0;
                    threadCount++;
                }
                else
                {
                    useThreads = false;
                    Debug.Write(_("RefineDiskCenter: failed to start a thread\n"));
                }
            }
            workload.push_back(pointToMeasure);
        }

    // Process remaining points locally
    for (const Point2f& point : workload)
    {
        float radius;
        float score = ::CalcContourScore(radius, point, diskContour, minRadius, maxRadius);
        if (score > bestScore)
        {
            bestScore = score;
            diskCenter.radius = radius;
            diskCenter.x = point.x;
            diskCenter.y = point.y;
        }
    }

    // Wait for all threads to terminate and process their results
    for (auto thread : threads)
    {
        thread->Wait();
        if (thread->threadBestScore > bestScore)
        {
            bestScore = thread->threadBestScore;
            diskCenter.radius = thread->radius;
            diskCenter.x = thread->center.x;
            diskCenter.y = thread->center.y;
        }

        delete thread;
    }
    return threadCount;
}

// An algorithm to find contour center
float SolarSystemObject::FindContourCenter(CircleDescriptor& diskCenter, CircleDescriptor& circle,
                                           std::vector<Point2f>& diskContour, Moments& mu, int minRadius, int maxRadius)
{
    float score;
    float maxScore = 0;
    float bestScore = 0;
    float radius = 0;
    int searchRadius = circle.radius / 2;
    Point2f pointToMeasure;
    std::vector<WeightedCircle> WeightedCircles;
    WeightedCircles.reserve(searchRadius * 2);

    // When center of mass (centroid) wasn't found use smallest circle for
    // measurement
    if (!m_DiameterLineParameters.valid)
    {
        pointToMeasure.x = circle.x;
        pointToMeasure.y = circle.y;
        score = CalcContourScore(radius, pointToMeasure, diskContour, minRadius, maxRadius);
        diskCenter = circle;
        diskCenter.radius = radius;
        return score;
    }

    if (!m_DiameterLineParameters.vertical && (fabs(m_DiameterLineParameters.slope) <= 1.0))
    {
        // Search along x-axis when line slope is below 45 degrees
        for (pointToMeasure.x = circle.x - searchRadius; pointToMeasure.x <= circle.x + searchRadius; pointToMeasure.x++)
        {
            // Count number of points of the contour which are equidistant from
            // pointToMeasure. The point with maximum score is identified as contour
            // center.
            pointToMeasure.y = m_DiameterLineParameters.slope * pointToMeasure.x + m_DiameterLineParameters.b;
            score = CalcContourScore(radius, pointToMeasure, diskContour, minRadius, maxRadius);
            maxScore = max(score, maxScore);
            WeightedCircle wcircle = { pointToMeasure.x, pointToMeasure.y, radius, score };
            WeightedCircles.push_back(wcircle);
        }
    }
    else
    {
        // Search along y-axis when slope is above 45 degrees
        for (pointToMeasure.y = circle.y - searchRadius; pointToMeasure.y <= circle.y + searchRadius; pointToMeasure.y++)
        {
            // Count number of points of the contour which are equidistant from
            // pointToMeasure. The point with maximum score is identified as contour
            // center.
            if (m_DiameterLineParameters.vertical)
                pointToMeasure.x = circle.x;
            else
                pointToMeasure.x = (pointToMeasure.y - m_DiameterLineParameters.b) / m_DiameterLineParameters.slope;
            score = CalcContourScore(radius, pointToMeasure, diskContour, minRadius, maxRadius);
            maxScore = max(score, maxScore);
            WeightedCircle wcircle = { pointToMeasure.x, pointToMeasure.y, radius, score };
            WeightedCircles.push_back(wcircle);
        }
    }

    // Find local maxima point closer to center of mass,
    // this will help not to select center of the dark disk
    int bestIndex = 0;
    float bestCenterOfMassDistance = 999999;
    Point2f centroid = { float(mu.m10 / mu.m00), float(mu.m01 / mu.m00) };
    for (int i = 1; i < WeightedCircles.size() - 1; i++)
    {
        if ((WeightedCircles[i].score > maxScore * 0.65) && (WeightedCircles[i].score > WeightedCircles[i - 1].score) &&
            (WeightedCircles[i].score > WeightedCircles[i + 1].score))
        {
            WeightedCircle *localMax = &WeightedCircles[i];
            Point2f center = { localMax->x, localMax->y };
            float centerOfMassDistance = norm(centroid - center);
            if (centerOfMassDistance < bestCenterOfMassDistance)
            {
                bestCenterOfMassDistance = centerOfMassDistance;
                bestIndex = i;
            }
        }
    }
    if (WeightedCircles.size() < 3)
    {
        for (int i = 0; i < WeightedCircles.size(); i++)
            if (WeightedCircles[i].score > bestScore)
            {
                bestScore = WeightedCircles[i].score;
                bestIndex = i;
            }
    }

    bestScore = WeightedCircles[bestIndex].score;
    diskCenter.radius = WeightedCircles[bestIndex].r;
    diskCenter.x = WeightedCircles[bestIndex].x;
    diskCenter.y = WeightedCircles[bestIndex].y;

    return bestScore;
}

// Find a minimum enclosing circle of the contour and also its center of mass
void SolarSystemObject::FindCenters(Mat image, const std::vector<Point>& contour, CircleDescriptor& centroid,
                                    CircleDescriptor& circle, std::vector<Point2f>& diskContour, Moments& mu, int minRadius,
                                    int maxRadius)
{
    const std::vector<Point> *effectiveContour = &contour;
    std::vector<Point> decimatedContour;
    Point2f circleCenter;
    float circle_radius = 0;

    // Add extra margins for min/max radii allowing inclusion of contours
    // outside and inside the given range.
    maxRadius = (maxRadius * 5) / 4;
    minRadius = (minRadius * 3) / 4;

    m_eccentricity = 0;
    m_angle = 0;
    circle.radius = 0;
    centroid.radius = 0;
    diskContour.clear();

    // If input contour is too large, decimate it to avoid performance issues
    int decimateRatio = contour.size() > 4096 ? contour.size() / 4096 : 1;
    if (decimateRatio > 1)
    {
        decimatedContour.reserve(contour.size() / decimateRatio);
        for (int i = 0; i < contour.size(); i += decimateRatio)
            decimatedContour.push_back(contour[i]);
        effectiveContour = &decimatedContour;
    }
    diskContour.reserve(effectiveContour->size());
    minEnclosingCircle(*effectiveContour, circleCenter, circle_radius);

    if ((circle_radius <= maxRadius) && (circle_radius >= minRadius))
    {
        // Convert contour to vector of floating points
        for (int i = 0; i < effectiveContour->size(); i++)
        {
            Point pt = (*effectiveContour)[i];
            diskContour.push_back(Point2f(pt.x, pt.y));
        }

        circle.x = circleCenter.x;
        circle.y = circleCenter.y;
        circle.radius = circle_radius;

        // Calculate center of mass based on contour points
        mu = cv::moments(diskContour, false);
        if (mu.m00 > 0)
        {
            centroid.x = mu.m10 / mu.m00;
            centroid.y = mu.m01 / mu.m00;
            centroid.radius = circle.radius;

            // Calculate eccentricity
            double a = mu.mu20 + mu.mu02;
            double b = sqrt(4 * mu.mu11 * mu.mu11 + (mu.mu20 - mu.mu02) * (mu.mu20 - mu.mu02));
            double major_axis = sqrt(2 * (a + b));
            double minor_axis = sqrt(2 * (a - b));
            m_eccentricity = sqrt(1 - (minor_axis * minor_axis) / (major_axis * major_axis));

            // Calculate orientation (theta) in radians and convert to degrees
            float theta = 0.5 * atan2(2 * mu.mu11, (mu.mu20 - mu.mu02));
            m_angle = theta * (180.0 / CV_PI);
        }
    }
}

// Find orb center using circle matching with contours
bool SolarSystemObject::FindOrbisCenter(Mat img8, int minRadius, int maxRadius, bool roiActive, Point2f& clickedPoint,
                                        Rect& roiRect, bool activeRoiLimits, float distanceRoiMax)
{
    int LowThreshold = Get_lowThreshold();
    int HighThreshold = Get_highThreshold();

    // Apply Canny edge detection
    Debug.Write(wxString::Format("Start detection of solar system object (roi:%d "
                                 "low_tr=%d,high_tr=%d,minr=%d,maxr=%d)\n",
                                 roiActive, LowThreshold, HighThreshold, minRadius, maxRadius));
    Mat edges, dilatedEdges;
    Canny(img8, edges, LowThreshold, HighThreshold, 5, true);
    dilate(edges, dilatedEdges, Mat(), Point(-1, -1), 2);

    // Find contours
    std::vector<std::vector<Point>> contours;
    cv::findContours(dilatedEdges, contours, RETR_LIST, CHAIN_APPROX_NONE);

    // Find total number of contours. If the number is too large, it means that
    // edge detection threshold value is possibly too low, or we'll need to
    // decimate number of points before further processing to avoid performance
    // issues.
    int totalPoints = 0;
    for (const auto& contour : contours)
    {
        totalPoints += contour.size();
    }
    if (totalPoints > 512 * 1024)
    {
        Debug.Write(wxString::Format("Too many contour points detected (%d)\n", totalPoints));
        m_statusMsg = _("Too many contour points detected. Please apply pixel binning, "
                        "enable ROI, or increase the Edge Detection Threshold.");
        pFrame->Alert(m_statusMsg, wxICON_WARNING);
        pFrame->pStatsWin->UpdatePlanetFeatureCount(_T("Contour points"), totalPoints);
        return false;
    }

    // Iterate between sets of contours to find the best match
    int contourAllCount = 0;
    int contourMatchingCount = 0;
    float bestScore = 0;
    std::vector<Point2f> bestContour;
    CircleDescriptor bestCircle = { 0 };
    CircleDescriptor bestCentroid = { 0 };
    CircleDescriptor bestDiskCenter = { 0 };
    bestContour.clear();
    int maxThreadsCount = 0;
    for (const auto& contour : contours)
    {
        // Ignore contours with small number of points
        if (contour.size() < 32)
            continue;

        // Find the smallest circle encompassing contour of the object
        // and also center of mass within the contour.
        cv::Moments mu;
        std::vector<Point2f> diskContour;
        CircleDescriptor circle = { 0 };
        CircleDescriptor centroid = { 0 };
        CircleDescriptor diskCenter = { 0 };
        FindCenters(img8, contour, centroid, circle, diskContour, mu, minRadius, maxRadius);

        // Skip circles not within radius range
        if ((circle.radius == 0) || (diskContour.size() == 0))
            continue;

        // Look for a point along the line connecting centers of the smallest circle
        // and center of mass which is equidistant from the outmost edge of the
        // contour. Consider this point as the best match for contour central point.
        CalcLineParams(circle, centroid);
        float score = FindContourCenter(diskCenter, circle, diskContour, mu, minRadius, maxRadius);

        // When user clicks a point in the main window, discard detected features
        // that are far away from it, similar to manual selection of stars in PHD2.
        Point2f circlePoint = { roiRect.x + diskCenter.x, roiRect.y + diskCenter.y };
        if (activeRoiLimits && (norm(clickedPoint - circlePoint) > distanceRoiMax))
            score = 0;

        // Refine the best fit
        if (score > 0.01)
        {
            float searchRadius = 20 * m_eccentricity + 3;
            int threadCount = RefineDiskCenter(score, diskCenter, diskContour, minRadius, maxRadius, searchRadius);
            maxThreadsCount = max(maxThreadsCount, threadCount);
            if (score > bestScore * 0.8)
                threadCount = RefineDiskCenter(score, diskCenter, diskContour, minRadius, maxRadius, 0.5, 0.1);
            maxThreadsCount = max(maxThreadsCount, threadCount);
        }

        // Select best fit based on highest score
        if (score > bestScore)
        {
            bestScore = score;
            bestDiskCenter = diskCenter;
            bestCentroid = centroid;
            bestContour = diskContour;
            bestCircle = circle;
        }
        contourMatchingCount++;
    }

    // Update stats window
    Debug.Write(wxString::Format("End detection of solar system object (t=%d): r=%.1f, x=%.1f, y=%.1f, "
                                 "score=%.3f, contours=%d/%d, threads=%d\n",
                                 m_SolarSystemObjWatchdog.Time(), bestDiskCenter.radius, roiRect.x + bestDiskCenter.x,
                                 roiRect.y + bestDiskCenter.y, bestScore, contourMatchingCount, contourAllCount,
                                 maxThreadsCount));
    pFrame->pStatsWin->UpdatePlanetFeatureCount(_T("Contours/points"), contourMatchingCount, bestContour.size());
    pFrame->pStatsWin->UpdatePlanetScore(("Fitting score"), bestScore);

    // For use by visual aid for parameter tuning
    if (VisualElementsEnabled())
    {
        m_syncLock.Lock();
        m_roiRect = roiRect;
        m_diskContour = bestContour;
        m_centoid_x = bestCentroid.x;
        m_centoid_y = bestCentroid.y;
        m_sm_circle_x = bestCircle.x;
        m_sm_circle_y = bestCircle.y;
        m_syncLock.Unlock();
    }

    if (bestDiskCenter.radius > 0)
    {
        m_center_x = roiRect.x + bestDiskCenter.x;
        m_center_y = roiRect.y + bestDiskCenter.y;
        m_radius = cvRound(bestDiskCenter.radius);
        m_searchRegion = m_radius;
        return true;
    }

    return false;
}

void SolarSystemObject::UpdateDetectionErrorInSimulator(Point2f& clickedPoint)
{
    if (pCamera && pCamera->Name == "Simulator")
    {
        bool errUnknown = true;
        bool clicked = (m_prevClickedPoint != clickedPoint);

        if (m_detected)
        {
            if (m_cameraSimulationRefPointValid)
            {
                m_simulationZeroOffset = false;
                m_cameraSimulationRefPointValid = false;
                m_origPoint = Point2f(m_center_x, m_center_y);
            }
            else if (!m_simulationZeroOffset && !clicked)
            {
                Point2f delta = Point2f(m_center_x, m_center_y) - m_origPoint;
                pFrame->pStatsWin->UpdatePlanetError(_T("Detection error"),
                                                     norm(delta - (m_cameraSimulationMove - m_cameraSimulationRefPoint)));
                errUnknown = false;
            }
        }

        if (errUnknown)
            pFrame->pStatsWin->UpdatePlanetError(_T("Detection error"), -1);

        if (clicked)
        {
            RestartSimulatorErrorDetection();
        }
    }
}

// Find object in the given image
bool SolarSystemObject::FindSolarSystemObject(const usImage *pImage, bool autoSelect)
{
    m_SolarSystemObjWatchdog.Start();

    // Default error status message
    m_statusMsg = _("Object not found");

    // Skip detection when paused
    if (m_paramDetectionPaused)
    {
        m_syncLock.Lock();
        m_detected = false;
        m_detectionCounter = 0;
        m_diskContour.clear();
        m_syncLock.Unlock();
        return false;
    }

    // Auto select star was requested
    if (autoSelect)
    {
        m_clicked_x = 0;
        m_clicked_y = 0;
        m_userLClick = false;
        m_detectionCounter = 0;
        RestartSimulatorErrorDetection();
    }
    Point2f clickedPoint = Point2f(m_clicked_x, m_clicked_y);

    // Use ROI for CPU time optimization
    bool roiActive = false;
    int minRadius = (int) Get_minRadius();
    int maxRadius = (int) Get_maxRadius();
    int roiRadius = (int) (maxRadius * 3 / 2.0 + 0.5);
    int roiOffsetX = 0;
    int roiOffsetY = 0;
    Mat FullFrame(pImage->Size.GetHeight(), pImage->Size.GetWidth(), CV_16UC1, pImage->ImageData);

    // Refuse to process images larger than 4096x4096 and request to use camera
    // binning
    if (FullFrame.cols > 4096 || FullFrame.rows > 4096)
    {
        Debug.Write(wxString::Format("Find solar system object: image is too large %dx%d\n", FullFrame.cols, FullFrame.rows));
        pFrame->Alert(_("ERROR: camera frame size exceeds maximum limit. Please "
                        "apply binning to reduce the frame size."),
                      wxICON_ERROR);
        m_syncLock.Lock();
        m_detected = false;
        m_detectionCounter = 0;
        m_diskContour.clear();
        m_syncLock.Unlock();
        return false;
    }

    // Limit image processing to ROI when enabled
    Mat RoiFrame;
    Rect roiRect(0, 0, pImage->Size.GetWidth(), pImage->Size.GetHeight());
    if (!autoSelect && GetRoiEnableState() && m_detected && (m_center_x < m_frameWidth) && (m_center_y < m_frameHeight) &&
        (m_frameWidth == pImage->Size.GetWidth()) && (m_frameHeight == pImage->Size.GetHeight()))
    {
        float fraction = (m_userLClick && (m_detectionCounter <= 4)) ? (1.0 - m_detectionCounter / 4.0) : 0.0;
        int x = cvRound(m_clicked_x * fraction + m_center_x * (1.0 - fraction));
        int y = cvRound(m_clicked_y * fraction + m_center_y * (1.0 - fraction));
        roiOffsetX = wxMax(0, x - roiRadius);
        roiOffsetY = wxMax(0, y - roiRadius);
        int w = wxMin(roiRadius * 2, pImage->Size.GetWidth() - roiOffsetX);
        int h = wxMin(roiRadius * 2, pImage->Size.GetHeight() - roiOffsetY);
        roiRect = Rect(roiOffsetX, roiOffsetY, w, h);
        RoiFrame = FullFrame(roiRect);
        roiActive = true;
    }
    else
    {
        RoiFrame = FullFrame;
    }

    // Make sure to use 8-bit gray image for feature detection
    // pImage always has 16-bit pixels, but depending on camera bpp
    // we should properly scale the image.
    Mat img8;
    int bppFactor = (pImage->BitsPerPixel >= 8) ? 1 << (pImage->BitsPerPixel - 8) : 1;
    RoiFrame.convertTo(img8, CV_8U, 1.0 / bppFactor);

    // Save latest frame dimensions
    m_frameWidth = pImage->Size.GetWidth();
    m_frameHeight = pImage->Size.GetHeight();

    // ROI current state and limit
    bool activeRoiLimits = m_userLClick && GetRoiEnableState();
    float distanceRoiMax = maxRadius * 3 / 2.0;

    bool detectionResult = false;
    try
    {
        // Do slight image blurring to decrease noise impact on results
        Mat imgFiltered;
        GaussianBlur(img8, imgFiltered, cv::Size(3, 3), 1.5);

        // Find object depending on the selected detection mode
        detectionResult = FindOrbisCenter(imgFiltered, minRadius, maxRadius, roiActive, clickedPoint, roiRect, activeRoiLimits,
                                          distanceRoiMax);

        // Calculate sharpness of the image
        if (m_measuringSharpnessMode)
            m_focusSharpness = CalcSharpness(FullFrame, clickedPoint, detectionResult);

        // Update detection time stats
        pFrame->pStatsWin->UpdatePlanetDetectionTime(m_SolarSystemObjWatchdog.Time());

        if (detectionResult)
        {
            m_detected = true;
            if (m_detectionCounter++ > 3)
            {
                // Smooth search region to avoid sudden jumps in star find stats
                m_searchRegion = cvRound(m_searchRegion * 0.3 + m_prevSearchRegion * 0.7);

                // Forget about the clicked point after a few successful detections
                m_userLClick = false;
            }
            m_prevSearchRegion = m_searchRegion;
        }
        if (m_measuringSharpnessMode || detectionResult)
            m_unknownHFD = false;
    }
    catch (const wxString& msg)
    {
        POSSIBLY_UNUSED(msg);
        Debug.Write(wxString::Format("Find solar system object: exception %s\n", msg));
    }
    catch (const cv::Exception& ex)
    {
        // Handle OpenCV exceptions
        Debug.Write(wxString::Format("Find solar system object: OpenCV exception %s\n", ex.what()));
        pFrame->Alert(_("ERROR: exception occurred during image processing: change "
                        "detection parameters"),
                      wxICON_ERROR);
    }
    catch (...)
    {
        // Handle any other exceptions
        Debug.Write("Find solar system object: unknown exception\n");
        pFrame->Alert(_("ERROR: unknown exception occurred in solar system object detection"), wxICON_ERROR);
    }

    // For simulated camera, calculate detection error by comparing with the
    // simulated position
    UpdateDetectionErrorInSimulator(clickedPoint);

    // Update data shared with other thread
    m_syncLock.Lock();
    m_roiRect = roiRect;
    if (!detectionResult)
    {
        m_detected = false;
        m_detectionCounter = 0;
        m_diskContour.clear();
    }
    m_roiActive = roiActive;
    m_prevClickedPoint = clickedPoint;
    m_syncLock.Unlock();

    return detectionResult;
}
