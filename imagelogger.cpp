/*
 *  imagelogger.cpp
 *  PHD2 Guiding
 *
 *  Created by Andy Galasso
 *  Copyright (c) 2017 openphdguiding.org
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
 *    Neither the name of OpenPHDGuiding.org nor the names of its
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
#include "imagelogger.h"

enum { SAVE_IMAGES = 2 }; // number of images to log preceding and following the trigger image

struct IL
{
    usImage *saved_image[SAVE_IMAGES];

    int imagesToLog;
    int eventNumber;
    ImageLoggerSettings settings;
    wxString debugLogDir;
    wxString imageLoggingRoot = "PHD2_Diag_Frames";
    wxString subdir;

    void Init()
    {
        for (int i = 0; i < SAVE_IMAGES; i++)
            saved_image[i] = 0;

        imagesToLog = 0;
        eventNumber = 1;

        settings.logFramesOverThreshRel = false;
        settings.logFramesOverThreshPx = false;
        settings.logFramesDropped = false;
    }

    void Destroy()
    {
        for (int i = 0; i < SAVE_IMAGES; i++)
            delete saved_image[i];
    }

    void SaveImage(usImage *img)
    {
        delete saved_image[0];
        for (int i = 1; i < SAVE_IMAGES; i++)
            saved_image[i - 1] = saved_image[i];
        saved_image[SAVE_IMAGES - 1] = img;
    }

    void LogImage(const usImage *img)
    {
        Debug.Write(wxString::Format("ImgLogger: LogImage event %u frame %u\n", eventNumber, img->FrameNum));

        wxString t = wxDateTime(img->ImgStartTime).Format(_T("%Y-%m-%d_%H%M%S"));
        wxString filename = wxString::Format("event%03d_%05d_%s.fit", eventNumber, img->FrameNum, t);

        wxString dir = Debug.GetLogDir();
        if (dir != debugLogDir)
        {
            // first time through or debug log changed
            debugLogDir = dir;
            subdir = dir + PATHSEPSTR + imageLoggingRoot + PATHSEPSTR + wxGetApp().GetInitTime().Format("CameraFrames_%Y-%m-%d-%H%M%S");
            if (!wxFileName::Mkdir(subdir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))
            {
                Debug.Write(wxString::Format("Error: Could not create frame logging directory %s\n", subdir));
                debugLogDir = wxEmptyString; // so we try again
                return;
            }
        }

        img->Save(wxFileName(subdir, filename).GetFullPath());
    }

    void LogSavedImages()
    {
        for (int i = 0; i < SAVE_IMAGES; i++)
            if (saved_image[i])
                LogImage(saved_image[i]);
    }

    void BeginLogging(const usImage *img)
    {
        if (imagesToLog == 0)
            LogSavedImages(); // previous images, excluding the current image
        LogImage(img);
        imagesToLog = SAVE_IMAGES;
    }

    void ContinueLogging(const usImage *img)
    {
        if (imagesToLog > 0)
        {
            LogImage(img);
            if (--imagesToLog == 0)
                ++eventNumber;
        }
    }
};

static IL s_il;

void ImageLogger::Init()
{
    s_il.Init();
}

void ImageLogger::Destroy()
{
    Debug.RemoveOldDirectories("CameraFrames*", 30);
    s_il.Destroy();
}

void ImageLogger::GetSettings(ImageLoggerSettings *settings)
{
    *settings = s_il.settings;
}

void ImageLogger::ApplySettings(const ImageLoggerSettings& settings)
{
    Debug.Write(wxString::Format("ImgLogger: Settings LogEnabled=%d Log Rel=%d, %.2f Log Px=%d, %.2f LogFrameDrop=%d\n",
        settings.loggingEnabled,
        settings.logFramesOverThreshRel, settings.logFramesOverThreshRel ? settings.guideErrorThreshRel : 0.,
        settings.logFramesOverThreshPx, settings.logFramesOverThreshPx ? settings.guideErrorThreshPx : 0.,
        settings.logFramesDropped));

    s_il.settings = settings;
}

void ImageLogger::SaveImage(usImage *img)
{
    s_il.SaveImage(img);
}

void ImageLogger::LogImage(const usImage *img, const FrameDroppedInfo& info)
{
    if (s_il.settings.loggingEnabled && s_il.settings.logFramesDropped &&
        pFrame->pGuider->IsCalibratingOrGuiding() && !pFrame->pGuider->IsPaused())
    {
        Debug.Write(wxString::Format("ImgLogger: star lost (%d) frame %u event %u\n", info.starError, img->FrameNum, s_il.eventNumber));
        s_il.BeginLogging(img);
        return;
    }

    s_il.ContinueLogging(img);
}

void ImageLogger::LogImage(const usImage *img, double distance)
{
    if (s_il.settings.loggingEnabled && (s_il.settings.logFramesOverThreshRel || s_il.settings.logFramesOverThreshPx) &&
        pFrame->pGuider->IsGuiding() && !pFrame->pGuider->IsPaused() &&
        !PhdController::IsSettling())
    {
        enum { MIN_FRAMES_FOR_STATS = 10 };
        unsigned int frameCount = pFrame->m_frameCounter; // do not use img->FrameNum since the image may have been captured before guiding started

        if (frameCount >= MIN_FRAMES_FOR_STATS)
        {
            double curErr = wxMax(pFrame->pGuider->CurrentError(), 0.001); // prevent divide by zero
            double relErr = distance / curErr;
            double threshPx = s_il.settings.logFramesOverThreshPx ? s_il.settings.guideErrorThreshPx : 99.;
            double threshRel = s_il.settings.logFramesOverThreshRel ? s_il.settings.guideErrorThreshRel : 99.;

            bool logit = false;

            if (s_il.settings.logFramesOverThreshPx && s_il.settings.logFramesOverThreshRel)
            {
                if (distance > threshPx && relErr > threshRel)
                    logit = true;
            }
            else if (s_il.settings.logFramesOverThreshPx && distance > threshPx)
                logit = true;
            else if (s_il.settings.logFramesOverThreshRel && relErr > threshRel)
                logit = true;

            if (logit)
            {
                Debug.Write(wxString::Format("ImgLogger: large offset frame %u event %u dist px %.2f vs %.2f rel %.2f vs %.2f cur %.2f\n",
                    img->FrameNum, s_il.eventNumber, distance, threshPx, relErr, threshRel, curErr));

                s_il.BeginLogging(img);
                return;
            }
        }
    }

    s_il.ContinueLogging(img);
}
