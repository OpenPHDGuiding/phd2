/*
 *  worker_thread.cpp
 *  PHD Guiding
 *
 *  Created by Bret McKee
 *  Copyright (c) 2012 Bret McKee
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
 *    Neither the name of Bret McKee, Dad Dog Development,
 *     Craig Stark, Stark Labs nor the names of its 
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

/*************      Terminate      **************************/

void WorkerThread::EnqueueWorkerThreadTerminateRequest(void)
{
    S_WORKER_THREAD_REQUEST message;
    memset(&message, 0, sizeof(message));

    message.request = REQUEST_TERMINATE;
    wxMessageQueueError queueError = m_workerQueue.Post(message);
}

/*************      Expose      **************************/

void WorkerThread::EnqueueWorkerThreadExposeRequest(usImage *pImage, double exposureDuration)
{
    S_WORKER_THREAD_REQUEST message;
    memset(&message, 0, sizeof(message));

    message.request = REQUEST_EXPOSE;
    message.args.expose.pImage = pImage;
    message.args.expose.exposureDuration = exposureDuration;
    wxMessageQueueError queueError = m_workerQueue.Post(message);
}

bool WorkerThread::HandleExpose(S_ARGS_EXPOSE *pArgs)
{
    bool bError = false;
    
    try
    {
        if (CurrentGuideCamera->HasNonGUICaptureFull())
        {
            pArgs->pImage->InitDate();

            if (CurrentGuideCamera->CaptureFull(pArgs->exposureDuration, *pArgs->pImage))
            {
                throw ERROR_INFO("CaptureFull failed");
            }

            switch (GuideCameraPrefs::NR_mode)
            {
                case NR_NONE:
                    break;
                case NR_2x2MEAN:
                    QuickLRecon(*pArgs->pImage);
                    break;
                case NR_3x3MEDIAN:
                    Median3(*pArgs->pImage);
                    break;
            }
        }
        else
        {
            MyFrame::S_EXPOSE_REQUEST request;
            request.pImage = pArgs->pImage;
            request.exposureDuration = pArgs->exposureDuration;

            wxCommandEvent evt(PHD_EXPOSE_EVENT, GetId());
            evt.SetClientData(&request);
            wxQueueEvent(frame, evt.Clone());

            // wait for the request to complete
            request.semaphore.Wait();

            bError = request.bError;
        }
    }
    catch (char *ErrorMsg)
    {
        POSSIBLY_UNUSED(ErrorMsg);
        bError = true;
    }

    return  bError;
}

void WorkerThread::SendWorkerThreadExposeComplete(usImage *pImage, bool bError)
{
    wxThreadEvent event = wxThreadEvent(wxEVT_THREAD, MYFRAME_WORKER_THREAD_EXPOSE_COMPLETE);
    event.SetPayload<usImage *>(pImage);
    event.SetInt(bError);
    wxQueueEvent(m_pFrame, event.Clone());
}

/*************      Guide       **************************/

void WorkerThread::EnqueueWorkerThreadGuideRequest(GUIDE_DIRECTION guideDirection, double guideDuration)
{
    S_WORKER_THREAD_REQUEST message;
    memset(&message, 0, sizeof(message));

    message.request = REQUEST_GUIDE;
    message.args.guide.guideDirection = guideDirection;
    message.args.guide.guideDuration  = guideDuration;
    wxMessageQueueError queueError    = m_workerQueue.Post(message);
}


bool WorkerThread::HandleGuide(S_ARGS_GUIDE *pArgs)
{
    bool bError = false;
    
    try
    {
        if (pArgs->guideDirection != NONE && pArgs->guideDuration > 0)
        {
            if (pScope->HasNonGUIGuide())
            {
                bError = pScope->NonGUIGuide(pArgs->guideDirection, pArgs->guideDuration);
            }
            else
            {
                // we don't have a non-gui guide function, wo we send this to the 
                // main frame routine that handles guides requests
                MyFrame::S_GUIDE_REQUEST request;
                request.guideDirection = pArgs->guideDirection;
                request.guideDuration  = pArgs->guideDuration;

                wxCommandEvent evt(PHD_GUIDE_EVENT, GetId());
                evt.SetClientData(&request);
                wxQueueEvent(frame, evt.Clone());

                // wait for the request to complete
                request.semaphore.Wait();

                bError = request.bError;
            }
        }
    }
    catch (char *ErrorMsg)
    {
        POSSIBLY_UNUSED(ErrorMsg);
        bError = true;
    }

    return  bError;
}

void WorkerThread::SendWorkerThreadGuideComplete(bool bError)
{
    wxThreadEvent event = wxThreadEvent(wxEVT_THREAD, MYFRAME_WORKER_THREAD_GUIDE_COMPLETE);
    event.SetInt(bError);
    wxQueueEvent(m_pFrame, event.Clone());
}

/*
 * entry point for the background thread
 */
wxThread::ExitCode WorkerThread::Entry()
{
    bool bDone = TestDestroy();

    while (!bDone)
    {
        S_WORKER_THREAD_REQUEST message;
        wxMessageQueueError queueError = m_workerQueue.Receive(message);

        if (queueError != wxMSGQUEUE_NO_ERROR)
        {
            wxLogError("Worker thread message queue receive failed");
            break;
        }

        switch(message.request)
        {
            bool bError;

            case WORKER_THREAD_REQUEST_NONE:
                break;
            case WORKER_THREAD_REQUEST_TERMINATE:
                bDone = true;
                break;
            case WORKER_THREAD_REQUEST_EXPOSE:
                bError = HandleExpose(&message.args.expose);
                SendWorkerThreadExposeComplete(message.args.expose.pImage, bError);
                break;
            case WORKER_THREAD_REQUEST_GUIDE:
                bError = HandleGuide(&message.args.guide);
                SendWorkerThreadGuideComplete(bError);
                break;
        }

        bDone |= TestDestroy();
    }

    return (wxThread::ExitCode)0;
}
