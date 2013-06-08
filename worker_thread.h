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

/*
 * There are two worker threads in PHD.  The primary thread handles all exposure requests,
 * and move requests for the first mount.  The secondary thread handles move requests for the
 * second mount, so that on systems with two mounts (probably an AO and a telescope), the
 * second mount can be moving while we image and guide with the first mount.
 *
 * The worker threads have three queues, one for move requests (higher priority)
 * and one for exposure requests (lower priority) and one "wakeup queue". The wx queue
 * routines do not have a way to wait on multiple queues, so there is no easy way
 * to implement the dual queue priority model without a third queue.
 *
 * When something is enqueued on either of the work queues, a dummy message is
 * also enqueued on the wakeup queue, which wakes the thread up.  It then finds
 * the work item by looking first on the high priority queue and then the low
 * priority queue.
 *
 */

class WorkerThread: public wxThread
{
    MyFrame *m_pFrame;
public:
    WorkerThread(MyFrame *pFrame);
    ~WorkerThread(void);

private:
    wxThread::ExitCode Entry();

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
    // there is no HandleTerminate(ARGS_TERMINATE *pArgs) routine
    // there is no void SendWorkerTerminiateComplete(bool bError);
    // there is no void OnWorkerThreadTerminateComplete(wxThreadEvent& event);

    /*************      Expose      **************************/
public:
    void EnqueueWorkerThreadExposeRequest(usImage *pImage, double exposureDuration, const wxRect& subframe);
protected:
    bool HandleExpose(MyFrame::EXPOSE_REQUEST *pArgs);
    void SendWorkerThreadExposeComplete(usImage *pImage, bool bError);
    // in the frame class: void MyFrame::OnWorkerThreadExposeComplete(wxThreadEvent& event);

    /*************      Guide       **************************/
public:
    void EnqueueWorkerThreadMoveRequest(Mount *pMount, const PHD_Point& vectorEndpoint, bool normalMove);
    void EnqueueWorkerThreadMoveRequest(Mount *pMount, const GUIDE_DIRECTION direction);
protected:
    bool HandleMove(MyFrame::PHD_MOVE_REQUEST *pArgs);
    void SendWorkerThreadMoveComplete(Mount *pMount, bool bError);
    // in the frame class: void MyFrame::OnWorkerThreadGuideComplete(wxThreadEvent& event);

    // types and routines for the server->worker message queue

    enum WORKER_REQUEST_TYPE
    {
        REQUEST_NONE,       // not used
        REQUEST_TERMINATE,
        REQUEST_EXPOSE,
        REQUEST_MOVE,
    };

    /*
     * a union containing all the possible argument types for thread work
     * requests
     */
    struct WORKER_REQUEST_ARGS
    {
        // there is no ARGS_TERMINATE terminate;
        MyFrame::EXPOSE_REQUEST expose;
        MyFrame::PHD_MOVE_REQUEST   move;
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

    void EnqueueMessage(const WORKER_THREAD_REQUEST& message);

    wxMessageQueue<bool> m_wakeupQueue;
    wxMessageQueue<WORKER_THREAD_REQUEST> m_highPriorityQueue;
    wxMessageQueue<WORKER_THREAD_REQUEST> m_lowPriorityQueue;
};

#endif /* WORKER_THREAD_H_INCLUDED */
