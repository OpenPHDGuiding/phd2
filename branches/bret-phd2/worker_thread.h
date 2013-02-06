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
 * There are two worker threads.  The primary thread handles all exposure requests, and
 * move requests for the first mount.  The secondary thread handles move requests for the
 * second mount, so that on systems with two mounts (probably an AO and a telescope), the
 * second mount can be moving while we image and guide with the first mount.
 *
 * The primary worker thread provides ordering - basically the upper levels enqueue things
 * in order and it processes them in order.  If there is only one mount the queue can
 * look like:
 *
 * PrimaryWorkerThread's queue:
 * - mount 1 RA guide request
 * - mount 1 Dec guide request
 * - expose request
 *
 * SecondaryWorkerThread's queue:
 *  <empty>
 *
 * The requests are handled in order by the primary worker. Both the guide requests will
 * have finished before the expose request, so that the mount does not move while we take
 * the exposure.
 *
 * If there are 2 mounts, the requests to the second mount get executed by the
 * secondary thread, so the queues can look like this:
 *
 * PrimaryWorkerThread's queue:
 * - mount 1 RA guide request
 * - mount 1 Dec guide request
 * - expose request
 *
 * SecondaryWorkerThread's queue:
 * - mount 2 RA guide request
 * - mount 2 Dec guide request
 *
 *  In this case, the to mount 2 guide requests can run while the expose request runs - the
 *  theory is that the AO can keep ahead of the scope's motion.
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
    struct ARGS_EXPOSE
    {
        usImage *pImage;
        double exposureDuration;
        wxRect subframe;
    };
    bool HandleExpose(ARGS_EXPOSE *pArgs);
    void SendWorkerThreadExposeComplete(usImage *pImage, bool bError);
    // in the frame class: void MyFrame::OnWorkerThreadExposeComplete(wxThreadEvent& event);

    /*************      Guide       **************************/
public:
    void EnqueueWorkerThreadMoveRequest(Mount *pMount, const Point& currentLocation, const Point& desiredLocation);
    void EnqueueWorkerThreadMoveRequest(Mount *pMount, const GUIDE_DIRECTION direction);
protected:
    struct ARGS_MOVE
    {
        Mount           *pMount;
        bool            calibrationMove;
        GUIDE_DIRECTION direction;
        Point           currentLocation;
        Point           desiredLocation;
    };
    bool HandleMove(ARGS_MOVE *pArgs);
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
        ARGS_EXPOSE expose;
        ARGS_MOVE  move;
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
