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

WorkerThread::WorkerThread(MyFrame *pFrame)
    :wxThread(wxTHREAD_JOINABLE)
{
    m_pFrame = pFrame;
    Debug.Write("WorkerThread constructor called\n");
}

WorkerThread::~WorkerThread(void)
{
    Debug.Write("WorkerThread destructor called\n");
}

void WorkerThread::SetStatusText(const wxString& statusMessage, int field)
{
    wxThreadEvent event = wxThreadEvent(wxEVT_THREAD, MYFRAME_WORKER_THREAD_SET_STATUS_TEXT);
    event.SetString(statusMessage);
    event.SetInt(field);
    wxQueueEvent(m_pFrame, event.Clone());
}

/*************      Terminate      **************************/

void WorkerThread::EnqueueWorkerThreadTerminateRequest(void)
{
    WORKER_THREAD_REQUEST message;
    memset(&message, 0, sizeof(message));

    message.request = REQUEST_TERMINATE;
    wxMessageQueueError queueError = m_workerQueue.Post(message);
}

/*************      Expose      **************************/

void WorkerThread::EnqueueWorkerThreadExposeRequest(usImage *pImage, double exposureDuration)
{
    WORKER_THREAD_REQUEST message;
    memset(&message, 0, sizeof(message));

    Debug.Write("Enqueuing Expose request\n");

    message.request = REQUEST_EXPOSE;
    message.args.expose.pImage = pImage;
    message.args.expose.exposureDuration = exposureDuration;
    wxMessageQueueError queueError = m_workerQueue.Post(message);
}

bool WorkerThread::HandleExpose(ARGS_EXPOSE *pArgs)
{
    bool bError = false;
    
    try
    {
        wxMilliSleep(pFrame->GetTimeLapse());

        if (CurrentGuideCamera->HasNonGUICaptureFull())
        {
            Debug.Write(wxString::Format("Handling exposure in thread\n"));

            pArgs->pImage->InitDate();

            if (CurrentGuideCamera->CaptureFull(pArgs->exposureDuration, *pArgs->pImage))
            {
                throw ERROR_INFO("CaptureFull failed");
            }

            switch (pFrame->GetNoiseReductionMethod())
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
            Debug.Write(wxString::Format("Handling exposure in myFrame\n"));

            MyFrame::PHD_EXPOSE_REQUEST request;
            request.pImage = pArgs->pImage;
            request.exposureDuration = pArgs->exposureDuration;

            wxCommandEvent evt(PHD_EXPOSE_EVENT, GetId());
            evt.SetClientData(&request);
            wxQueueEvent(pFrame, evt.Clone());

            // wait for the request to complete
            request.semaphore.Wait();

            bError = request.bError;
        }
        Debug.Write(wxString::Format("Exposure complete\n"));
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
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

void WorkerThread::EnqueueWorkerThreadGuideRequest(GUIDE_DIRECTION guideDirection, double guideDuration, const wxString &statusMessage)
{
    WORKER_THREAD_REQUEST message;
    memset(&message, 0, sizeof(message));

    Debug.Write(wxString::Format("Enqueuing Guide request for direction %d\n", message.args.guide.guideDirection));

    message.request = REQUEST_GUIDE;
    message.args.guide.guideDirection = guideDirection;
    message.args.guide.guideDuration  = guideDuration;
    message.args.guide.statusMessage  = statusMessage;
    wxMessageQueueError queueError    = m_workerQueue.Post(message);
}


bool WorkerThread::HandleGuide(ARGS_GUIDE *pArgs)
{
    bool bError = false;
    
    try
    {
        if (pArgs->guideDirection != NONE && pArgs->guideDuration > 0)
        {
            SetStatusText(pArgs->statusMessage, 1);

            if (pScope->HasNonGUIGuide())
            {
                Debug.Write(wxString::Format("Handling %d guide in thread\n", pArgs->guideDirection));
                bError = pScope->NonGUIGuide(pArgs->guideDirection, pArgs->guideDuration);
            }
            else
            {
                Debug.Write(wxString::Format("Handling %d guide in myFrame\n", pArgs->guideDirection));

                // we don't have a non-gui guide function, wo we send this to the 
                // main frame routine that handles guides requests
                MyFrame::PHD_GUIDE_REQUEST request;
                request.guideDirection = pArgs->guideDirection;
                request.guideDuration  = pArgs->guideDuration;

                wxCommandEvent evt(PHD_GUIDE_EVENT, GetId());
                evt.SetClientData(&request);
                wxQueueEvent(pFrame, evt.Clone());

                // wait for the request to complete
                request.semaphore.Wait();


                bError = request.bError;
            }

            Debug.Write(wxString::Format("Guide complete\n"));
        }
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
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

    Debug.Write("WorkerThread::Entry() begins\n");

    while (!bDone)
    {
        WORKER_THREAD_REQUEST message;
        wxMessageQueueError queueError = m_workerQueue.Receive(message);

        if (queueError != wxMSGQUEUE_NO_ERROR)
        {
            wxLogError("Worker thread message queue receive failed");
            break;
        }

        Debug.Write(wxString::Format("worker thread servicing request %d\n", message.request));

        switch(message.request)
        {
            bool bError;

            case REQUEST_NONE:
                break;
            case REQUEST_TERMINATE:
                bDone = true;
                break;
            case REQUEST_EXPOSE:
                bError = HandleExpose(&message.args.expose);
                SendWorkerThreadExposeComplete(message.args.expose.pImage, bError);
                break;
            case REQUEST_GUIDE:
                bError = HandleGuide(&message.args.guide);
                SendWorkerThreadGuideComplete(bError);
                break;
        }

        Debug.Write(wxString::Format("worker thread done servicing request %d\n", message.request));
        bDone |= TestDestroy();
    }

    Debug.Write("WorkerThread::Entry() ends\n");
    Debug.Flush();

    return (wxThread::ExitCode)0;
}
