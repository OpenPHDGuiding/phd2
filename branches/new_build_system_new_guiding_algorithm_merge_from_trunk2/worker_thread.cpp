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
    : wxThread(wxTHREAD_JOINABLE),
      m_interruptRequested(0),
      m_killable(true)
{
    m_pFrame = pFrame;
    Debug.AddLine("WorkerThread constructor called");
}

WorkerThread::~WorkerThread(void)
{
    Debug.AddLine("WorkerThread destructor called");
}

void WorkerThread::EnqueueMessage(const WORKER_THREAD_REQUEST& message)
{
    wxMessageQueueError queueError;

    if (message.request == REQUEST_EXPOSE)
    {
        queueError = m_lowPriorityQueue.Post(message);
    }
    else
    {
        queueError = m_highPriorityQueue.Post(message);
    }

    assert(queueError == wxMSGQUEUE_NO_ERROR);

    queueError = m_wakeupQueue.Post(true);

    assert(queueError == wxMSGQUEUE_NO_ERROR);
}

/*************      Terminate      **************************/

void WorkerThread::EnqueueWorkerThreadTerminateRequest(void)
{
    m_interruptRequested = INT_STOP | INT_TERMINATE;

    WORKER_THREAD_REQUEST message;
    memset(&message, 0, sizeof(message));

    message.request = REQUEST_TERMINATE;
    EnqueueMessage(message);
}

/*************      Expose      **************************/

void WorkerThread::EnqueueWorkerThreadExposeRequest(usImage *pImage, int exposureDuration, const wxRect& subframe)
{
    m_interruptRequested &= ~INT_STOP;

    WORKER_THREAD_REQUEST message;
    memset(&message, 0, sizeof(message));

    Debug.AddLine("Enqueuing Expose request");

    message.request                      = REQUEST_EXPOSE;
    message.args.expose.pImage           = pImage;
    message.args.expose.exposureDuration = exposureDuration;
    message.args.expose.subframe         = subframe;
    message.args.expose.pSemaphore       = NULL;

    EnqueueMessage(message);
}

unsigned int WorkerThread::MilliSleep(int ms, unsigned int checkInterrupts)
{
    enum { MAX_SLEEP = 100 };

    if (ms <= MAX_SLEEP)
    {
        if (ms > 0)
            wxMilliSleep(ms);
        return WorkerThread::InterruptRequested() & checkInterrupts;
    }

    WorkerThread *thr = WorkerThread::This();
    wxStopWatch swatch;

    long elapsed = 0;
    do {
        wxMilliSleep(wxMin((long) ms - elapsed, (long) MAX_SLEEP));
        unsigned int val = thr ? (thr->m_interruptRequested & checkInterrupts) : 0;
        if (val)
            return val;
        elapsed = swatch.Time();
    } while (elapsed < ms);

    return 0;
}

bool WorkerThread::HandleExpose(MyFrame::EXPOSE_REQUEST *pArgs)
{
    bool bError = false;

    try
    {
        if (WorkerThread::MilliSleep(m_pFrame->GetTimeLapse(), INT_ANY))
        {
            throw ERROR_INFO("Time lapse interrupted");
        }

        if (pCamera->HasNonGuiCapture())
        {
            Debug.AddLine("Handling exposure in thread");

            pArgs->pImage->InitImgStartTime();

            if (pCamera->Capture(pArgs->exposureDuration, *pArgs->pImage, pArgs->subframe, true))
            {
                throw ERROR_INFO("Capture failed");
            }
        }
        else
        {
            Debug.AddLine("Handling exposure in myFrame");

            wxSemaphore semaphore;
            pArgs->pSemaphore = &semaphore;

            wxCommandEvent evt(REQUEST_EXPOSURE_EVENT, GetId());
            evt.SetClientData(pArgs);
            wxQueueEvent(m_pFrame, evt.Clone());

            // wait for the request to complete
            pArgs->pSemaphore->Wait();

            bError = pArgs->bError;
            pArgs->pSemaphore = NULL;
        }
        Debug.AddLine("Exposure complete");

        if (!bError)
        {
            switch (m_pFrame->GetNoiseReductionMethod())
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

           pArgs->pImage->CalcStats();
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return  bError;
}

void WorkerThread::SendWorkerThreadExposeComplete(usImage *pImage, bool bError)
{
    wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD, MYFRAME_WORKER_THREAD_EXPOSE_COMPLETE);
    event->SetPayload<usImage *>(pImage);
    event->SetInt(bError);
    wxQueueEvent(m_pFrame, event);
}

/*************      Move       **************************/

void WorkerThread::EnqueueWorkerThreadMoveRequest(Mount *pMount, const PHD_Point& vectorEndpoint, bool normalMove)
{
    m_interruptRequested &= ~INT_STOP;

    WORKER_THREAD_REQUEST message;
    memset(&message, 0, sizeof(message));

    Debug.AddLine(wxString::Format("Enqueuing Move request for %s (%.2f, %.2f)", pMount->GetMountClassName(), vectorEndpoint.X, vectorEndpoint.Y));

    message.request                   = REQUEST_MOVE;
    message.args.move.pMount          = pMount;
    message.args.move.calibrationMove = false;
    message.args.move.vectorEndpoint  = vectorEndpoint;
    message.args.move.normalMove      = normalMove;
    message.args.move.pSemaphore      = NULL;

    EnqueueMessage(message);
}

void WorkerThread::EnqueueWorkerThreadMoveRequest(Mount *pMount, const GUIDE_DIRECTION direction, int duration)
{
    m_interruptRequested &= ~INT_STOP;

    WORKER_THREAD_REQUEST message;
    memset(&message, 0, sizeof(message));

    Debug.AddLine("Enqueuing Calibration Move request for direction %d", direction);

    message.request                   = REQUEST_MOVE;
    message.args.move.pMount          = pMount;
    message.args.move.calibrationMove = true;
    message.args.move.direction       = direction;
    message.args.move.duration        = duration;
    message.args.move.normalMove      = true;
    message.args.move.pSemaphore      = NULL;

    EnqueueMessage(message);
}

Mount::MOVE_RESULT WorkerThread::HandleMove(MyFrame::PHD_MOVE_REQUEST *pArgs)
{
    Mount::MOVE_RESULT result = Mount::MOVE_OK;

    try
    {
        if (pArgs->pMount->HasNonGuiMove())
        {
            Debug.AddLine(wxString::Format("Handling move in thread for %s dir=%d",
                    pArgs->pMount->GetMountClassName(),
                    pArgs->direction));

            if (pArgs->calibrationMove)
            {
                Debug.AddLine("calibration move");

                result = pArgs->pMount->CalibrationMove(pArgs->direction, pArgs->duration);
                if (result != Mount::MOVE_OK)
                {
                    throw ERROR_INFO("CalibrationMove failed");
                }
            }
            else
            {
                Debug.AddLine(wxString::Format("endpoint = (%.2f, %.2f)",
                    pArgs->vectorEndpoint.X, pArgs->vectorEndpoint.Y));

                result = pArgs->pMount->Move(pArgs->vectorEndpoint, pArgs->normalMove);
                if (result != Mount::MOVE_OK)
                {
                    throw ERROR_INFO("Move failed");
                }
            }
        }
        else
        {
            // we don't have a non-gui guide function, so we send this to the
            // main frame routine that handles guides requests

            Debug.AddLine("Sending move to myFrame");

            wxSemaphore semaphore;
            pArgs->pSemaphore = &semaphore;

            wxCommandEvent evt(REQUEST_MOUNT_MOVE_EVENT, GetId());
            evt.SetClientData(pArgs);
            wxQueueEvent(m_pFrame, evt.Clone());

            // wait for the request to complete
            pArgs->pSemaphore->Wait();

            pArgs->pSemaphore = NULL;
            result = pArgs->moveResult;

            if (result != Mount::MOVE_OK)
            {
                throw ERROR_INFO("myFrame handled move failed");
            }
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        if (result == Mount::MOVE_OK)
            result = Mount::MOVE_ERROR;
    }

    Debug.AddLine(wxString::Format("move complete, result=%d", result));

    return result;
}

void WorkerThread::SendWorkerThreadMoveComplete(Mount *pMount, Mount::MOVE_RESULT moveResult)
{
    wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD, MYFRAME_WORKER_THREAD_MOVE_COMPLETE);
    event->SetInt(moveResult);
    event->SetPayload<Mount *>(pMount);
    wxQueueEvent(m_pFrame, event);
}

/*
 * entry point for the background thread
 */
wxThread::ExitCode WorkerThread::Entry()
{
    bool bDone = TestDestroy();

    Debug.AddLine("WorkerThread::Entry() begins");

#if defined(__WINDOWS__)
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    Debug.AddLine("worker thread CoInitializeEx returns %x", hr);
#endif

    while (!bDone)
    {
        WORKER_THREAD_REQUEST message;
        bool dummy;
        wxMessageQueueError queueError = m_wakeupQueue.Receive(dummy);

        Debug.AddLine("Worker thread wakes up");

        assert(queueError == wxMSGQUEUE_NO_ERROR);

        queueError = m_highPriorityQueue.ReceiveTimeout(0, message);

        if (queueError == wxMSGQUEUE_TIMEOUT)
        {
            queueError = m_lowPriorityQueue.ReceiveTimeout(0, message);
            assert(queueError != wxMSGQUEUE_TIMEOUT);
        }

        assert(queueError == wxMSGQUEUE_NO_ERROR);

        switch(message.request)
        {
            bool bError;

            case REQUEST_NONE:
                Debug.AddLine("worker thread servicing REQUEST_NONE");
                break;
            case REQUEST_TERMINATE:
                Debug.AddLine("worker thread servicing REQUEST_TERMINATE");
                bDone = true;
                break;
            case REQUEST_EXPOSE:
                Debug.AddLine("worker thread servicing REQUEST_EXPOSE %d",
                    message.args.expose.exposureDuration);
                bError = HandleExpose(&message.args.expose);
                SendWorkerThreadExposeComplete(message.args.expose.pImage, bError);
                break;
            case REQUEST_MOVE: {
                Debug.AddLine(wxString::Format("worker thread servicing REQUEST_MOVE %s dir %d (%.2f, %.2f)",
                    message.args.move.pMount->GetMountClassName(), message.args.move.direction,
                    message.args.move.vectorEndpoint.X, message.args.move.vectorEndpoint.Y));
                Mount::MOVE_RESULT moveResult = HandleMove(&message.args.move);
                SendWorkerThreadMoveComplete(message.args.move.pMount, moveResult);
                break;
            }
            default:
                Debug.AddLine("worker thread servicing unknown request %d", message.request);
                break;
        }

        Debug.AddLine("worker thread done servicing request");
        bDone |= TestDestroy();
    }

    Debug.AddLine("WorkerThread::Entry() ends");
    Debug.Flush();

    return (wxThread::ExitCode)0;
}
