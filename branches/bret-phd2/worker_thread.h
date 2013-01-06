/*
 *  worker_thread.h
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

#ifndef WORKER_THREAD_H_INCLUDED
#define WORKER_THREAD_H_INCLUDED

class MyFrame;

class WorkerThread: public wxThread
{
    MyFrame *m_pFrame;
public:
    WorkerThread(MyFrame *pFrame);
    ~WorkerThread(void);

private:
    wxThread::ExitCode Entry();

    // a helper routine to set the status by sending a message
    // to m_pFrame
    void SetStatusText(const wxString& statusMessage, int field);

    /*
     * A worker thread is used only for long running tasks:
     *   - take an image
     *   - guide
     * 
     */

    /*
     *
     * These are the routines for processing requests on the worker 
     * thread.  For most requests there are 4 routines:
     * - a routine to post a request on the worker thread queue (EnqueueWorkerThread...)
     *      - arguments are passed in a struct for ARGS_....
     * - a routine called from Guider::Entry() which does the work Handle...)
     * - a routine called when the work is done that enqueues an event (SendWorkerThread...)
     * - the routine that is called in response to the receipt of that event (OnWorkerThread...)
     *
     */

    /*************      Terminate      **************************/
public:
    void EnqueueWorkerThreadTerminateRequest(void);
protected:
    // there is no struct ARGS_TERMINATE
    // there is no HandleTerminate(ARGS_TERMINATE *pArgs) routine
    // there is no void SendWorkerTerminiateComplete(bool bError);
    // there is no void OnWorkerThreadTerminateComplete(wxThreadEvent& event);

    /*************      Expose      **************************/
public:
    void EnqueueWorkerThreadExposeRequest(usImage *pImage, double exposureDuration);
protected:
    struct ARGS_EXPOSE
    {
        usImage *pImage;
        double exposureDuration;
    };
    bool HandleExpose(ARGS_EXPOSE *pArgs);
    void SendWorkerThreadExposeComplete(usImage *pImage, bool bError);
    // in the frame class: void MyFrame::OnWorkerThreadExposeComplete(wxThreadEvent& event);

    /*************      Guide       **************************/
public:
    void EnqueueWorkerThreadGuideRequest(GUIDE_DIRECTION direction, double exposureDuration, const wxString& statusMessage);
protected:
    struct ARGS_GUIDE
    {
        GUIDE_DIRECTION guideDirection;
        double          guideDuration;
        wxString        statusMessage;
    };
    bool HandleGuide(ARGS_GUIDE *pArgs);
    void SendWorkerThreadGuideComplete(bool bError);
    // in the frame class: void MyFrame::OnWorkerThreadGuideComplete(wxThreadEvent& event);

    // types and routines for the server->worker message queue

    enum WORKER_REQUEST_TYPE
    {
        REQUEST_NONE,       // not used
        REQUEST_TERMINATE,
        REQUEST_EXPOSE,
        REQUEST_GUIDE,
    };

    /*
     * a union containing all the possible argument types for thread work
     * requests
     */
    struct WORKER_REQUEST_ARGS
    {
        // there is no ARGS_TERMINATE terminate;
        ARGS_EXPOSE expose;
        ARGS_GUIDE  guide;
    };

    /*
     * this struct is passed through the message queue to the worker thread
     * to request work
     */

    struct WORKER_THREAD_REQUEST
    {
        WORKER_REQUEST_TYPE request;
        WORKER_REQUEST_ARGS args;
    };

    wxMessageQueue<WORKER_THREAD_REQUEST> m_workerQueue;
};

#endif /* WORKER_THREAD_H_INCLUDED */
