/*
 *  v4lcontrol.cpp
 *  PHD Guiding
 *
 *  Created by Steffen Elste
 *  Copyright (c) 2010 Steffen Elste.
 *  All rights reserved.
 *
 *  This source code is distributed under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Craig Stark, Stark Labs nor the names of its contributors may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "v4lcontrol.h"


V4LControl::V4LControl(int fd, const struct v4l2_queryctrl &ctrl)
	: fd(fd),
	  cid(ctrl.id),
	  type(ctrl.type),
	  defaultValue(ctrl.default_value),
	  min(ctrl.minimum),
	  max(ctrl.maximum),
	  step(ctrl.step) {

	value = defaultValue;
	name = wxString((const char*)ctrl.name, *wxConvCurrent);

	if (V4L2_CTRL_TYPE_MENU == type)
		enumerateMenuControls(ctrl);
}

bool V4LControl::update() {
	bool result = false;
    struct v4l2_control c;

    c.id = this->cid;

    switch (this->type) {
		case V4L2_CTRL_TYPE_INTEGER:
		    if (value < min)
		        value = min;
		    if (value > max)
		    	value = max;
		    if (step > 1) {
		    	int mod = (value - min)%step;
		        if (mod > step/2) {
		            value += step-mod;
		        } else {
		            value -= mod;
		        }
		    }
			break;
		case V4L2_CTRL_TYPE_BOOLEAN:
		    break;
		case V4L2_CTRL_TYPE_MENU:
			if (value < min)
				value = min;
			if (value > max)
				value = max;
			break;
		default:
			break;
    }

    c.value = value;

    if (-1 != v4l2_ioctl(fd, VIDIOC_S_CTRL, &c)) {
    	// Check the current settings ...
    	memset(&c, 0, sizeof(c));

    	c.id = this->cid;
    	if (0 == v4l2_ioctl(fd, VIDIOC_G_CTRL, &c) && c.value == this->value)
    		result = true;
    }

    return result;
}

bool V4LControl::reset() {
	bool result = false;
    struct v4l2_control c;

    c.id = this->cid;
    c.value = defaultValue;

    if (-1 != v4l2_ioctl(fd, VIDIOC_S_CTRL, &c)) {
    	// Check the current settings ...
    	memset(&c, 0, sizeof(c));

    	c.id = this->cid;
    	if (0 == v4l2_ioctl(fd, VIDIOC_G_CTRL, &c)) {
    		this->value = this->defaultValue;

        	result = true;
    	}
    }

    return result;
}

void V4LControl::enumerateMenuControls(const struct v4l2_queryctrl &ctrl) {
	struct v4l2_querymenu menu = {0};

    menu.id = ctrl.id;
    for (menu.index = ctrl.minimum; menu.index <= ctrl.maximum; menu.index++) {
    	if (0 == ioctl(fd, VIDIOC_QUERYMENU, &menu)) {
    		choices.Add(wxString((const char *)menu.name, *wxConvCurrent));
    	}
    }
}
