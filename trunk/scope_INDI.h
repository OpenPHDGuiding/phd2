/*
 *  scope_INDI.cpp
 *  PHD Guiding
 *
 *  Ported by Hans Lambermont in 2014 from tele_INDI.h which has Copyright (c) 2009 Geoffrey Hausheer.
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
 *    Neither the name of Craig Stark, Stark Labs nor the names of its
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
#ifdef  GUIDE_INDI

struct indi_t;
struct indi_prop_t;

class ScopeINDI : public Scope {
public:
    ScopeINDI(void);

    virtual bool Connect(void);

    virtual bool Disconnect(void);

    virtual MOVE_RESULT Guide(GUIDE_DIRECTION direction, int duration);

private:
	struct indi_prop_t *coord_set_prop;
	struct indi_prop_t *abort_prop;
	struct indi_prop_t *moveNS;
	struct indi_prop_t *moveEW;
	struct indi_prop_t *pulseGuideNS;
	struct indi_prop_t *pulseGuideEW;
	bool    ready;

public:
	bool     modal;
    wxString serial_port;
	bool     CaptureFull(int duration, usImage& img, bool recon);	// Captures a full-res shot
	void     InitCapture() { return; }
	void     ShowPropertyDialog();
	void     CheckState();
	void     NewProp(struct indi_prop_t *iprop);
    void     StartMove(int direction);
    void     StopMove(int direction);
	void     PulseGuide(int direction, int duration);
	bool     IsReady() {return ready;};
	bool     CanPulseGuide() { return pulseGuideNS && pulseGuideEW;};
	void     DoGuiding(int direction, int duration_msec);
};

#endif /* GUIDE_INDI */
