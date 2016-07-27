/*
 *  star_profile.cpp
 *  PHD Guiding
 *
 *  Created by Sylvain Girard
 *  Copyright (c) 2013 Sylvain Girard
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
 *    Neither the name of Bret McKee, Dad Dog Development Ltd, nor the names of its
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
 *
 */

#include "phd.h"

BEGIN_EVENT_TABLE(ProfileWindow, wxWindow)
    EVT_PAINT(ProfileWindow::OnPaint)
    EVT_LEFT_DOWN(ProfileWindow::OnLClick)
END_EVENT_TABLE()

enum
{
    HALFW = 10,
    FULLW = 2 * HALFW + 1,
};

ProfileWindow::ProfileWindow(wxWindow *parent) :
    wxWindow(parent,wxID_ANY,wxDefaultPosition,wxDefaultSize, wxFULL_REPAINT_ON_RESIZE,_("Profile"))
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    this->visible = false;
    this->mode = 0; // 2D profile
    this->SetBackgroundStyle(wxBG_STYLE_CUSTOM);
    this->data = new unsigned short[FULLW * FULLW];  // 21x21 subframe
}

ProfileWindow::~ProfileWindow()
{
    delete[] data;
}

void ProfileWindow::OnLClick(wxMouseEvent& WXUNUSED(mevent))
{
    this->mode = this->mode + 1;
    if (this->mode > 2) this->mode = 0;
    Refresh();
}

void ProfileWindow::SetState(bool is_active)
{
    this->visible = is_active;
    if (is_active)
        Refresh();
}

void ProfileWindow::UpdateData(usImage *pImg, float xpos, float ypos)
{
    if (this->data == NULL) return;
    int xstart = ROUNDF(xpos) - HALFW;
    int ystart = ROUNDF(ypos) - HALFW;
    if (xstart < 0) xstart = 0;
    else if (xstart > (pImg->Size.GetWidth() - (FULLW + 1)))
        xstart = pImg->Size.GetWidth() - (FULLW + 1);
    if (ystart < 0) ystart = 0;
    else if (ystart > (pImg->Size.GetHeight() - (FULLW + 1)))
        ystart = pImg->Size.GetHeight() - (FULLW + 1);

    int x,y;
    unsigned short *uptr = this->data;
    const int xrowsize = pImg->Size.GetWidth();
    for (x = 0; x < FULLW; x++)
        horiz_profile[x] = vert_profile[x] = midrow_profile[x] = 0;
    for (y = 0; y < FULLW; y++) {
        for (x = 0; x < FULLW; x++, uptr++) {
            *uptr = *(pImg->ImageData + xstart + x + (ystart + y) * xrowsize);
            horiz_profile[x] += (int) *uptr;
            vert_profile[y] += (int) *uptr;
        }
    }
    uptr = this->data + (FULLW * HALFW);
    for (x = 0; x < FULLW; x++, uptr++)
        midrow_profile[x] = (int) *uptr;
    if (this->visible)
        Refresh();
}

void ProfileWindow::OnPaint(wxPaintEvent& WXUNUSED(evt))
{
    wxAutoBufferedPaintDC dc(this);

    wxPoint Prof[FULLW];

    dc.SetBackground(wxColour(10,30,30));
    dc.Clear();
    if (!pFrame || !pFrame->pGuider || pFrame->pGuider->GetState() == STATE_UNINITIALIZED) return;

    const int xsize = this->GetSize().GetX();
    const int ysize = this->GetSize().GetY();

#if defined (__APPLE__)
    const wxFont& smallFont = *wxSMALL_FONT;
#else
    const wxFont& smallFont = *wxSWISS_FONT;
#endif
    dc.SetFont(smallFont);
    int smallFontHeight = dc.GetTextExtent("0").GetHeight();

    bool inFocusingMode = (ysize > xsize/2 + 20);

    wxFont largeFont;
    int largeFontHeight;
    int labelTextHeight;

    if (inFocusingMode) {
        //todo: Tuning the scaling factor
    	int scale = ysize / 50;
        largeFont = smallFont.Scaled(scale);

        dc.SetFont(largeFont);
        largeFontHeight = dc.GetTextExtent("0").GetHeight();
        dc.SetFont(smallFont);
        labelTextHeight = 5 + smallFontHeight + largeFontHeight + 5;
    }
    else {
        labelTextHeight = 5 + smallFontHeight + 5;
    }

    wxPen RedPen;
    //  GreyDashPen = wxPen(wxColour(200,200,200),1, wxDOT);
    //  BluePen = wxPen(wxColour(100,100,255));
    RedPen = wxPen(wxColour(255,0,0));

    int i;
    int *profptr;
    wxString profileLabel;
    switch (this->mode) {  // Figure which profile to use
    case 0: // mid-row
    default:
        profptr = midrow_profile;
        profileLabel = _("Mid row");
        break;
    case 1: // avg row
        profptr = horiz_profile;
        profileLabel = _("Avg row");
        break;
    case 2:
        profptr = vert_profile;
        profileLabel = _("Avg col");
        break;
    }

    float fwhm = 0;

    // Figure max and min
    int Prof_Min, Prof_Max, Prof_Mid;
    Prof_Min = Prof_Max = *profptr;

    for (i = 1; i < FULLW; i++) {
        if (*(profptr + i) < Prof_Min)
            Prof_Min = *(profptr + i);
        else if (*(profptr + i) > Prof_Max)
            Prof_Max = *(profptr + i);
    }
    if (Prof_Min < Prof_Max)
    {
        Prof_Mid = (Prof_Max - Prof_Min) / 2 + Prof_Min;
        // Figure the actual points in the window
        float Prof_Range = (float)(Prof_Max - Prof_Min) / (float)(ysize - labelTextHeight - 5);
        if (!Prof_Range) Prof_Range = 1;
        int wprof = (xsize - 15) / 2 - 5;
        wprof /= 20;
        for (i = 0; i < FULLW; i++)
            Prof[i] = wxPoint(5 + i * wprof, ysize - labelTextHeight - ((float)(*(profptr + i) - Prof_Min) / Prof_Range));

        // fwhm
        int x1 = 0;
        int x2 = 0;
        int profval;
        int profvalprec;
        for (i = 1; i < FULLW; i++)
        {
            profval = *(profptr + i);
            profvalprec = *(profptr + i - 1);
            if (profvalprec <= Prof_Mid && profval >= Prof_Mid)
                x1 = i;
            else if (profvalprec >= Prof_Mid && profval <= Prof_Mid)
                x2 = i;
        }
        profval = *(profptr + x1);
        profvalprec = *(profptr + x1 - 1);
        float f1 = (float)x1 - (float)(profval - Prof_Mid) / (float)(profval - profvalprec);
        profval = *(profptr + x2);
        profvalprec = *(profptr + x2 - 1);
        float f2 = (float)x2 - (float)(profvalprec - Prof_Mid) / (float)(profvalprec - profval);
        fwhm = f2 - f1;

        // Draw it
        dc.SetPen(RedPen);
        dc.DrawLines(FULLW, Prof);
    }

    //dc.SetTextForeground(wxColour(100,100,255));
    dc.SetTextForeground(wxColour(255,0,0));

    unsigned int peak = pFrame->pGuider->StarPeakADU();
    if (peak) {
        dc.DrawText(_("Peak"), 3, 3);
        dc.DrawText(wxString::Format("%u", peak), 3, 3 + smallFontHeight);
    }

    float hfd = pFrame->pGuider->HFD();
	if (hfd != 0.f) {
        float hfdArcSec = hfd * pFrame->GetCameraPixelScale();
        if (inFocusingMode) {
            dc.DrawText(wxString::Format(_("%s FWHM: %.2f"), profileLabel, fwhm), 5, ysize - labelTextHeight + 5);
            int x = 5;
            wxString s(_("HFD: "));
            dc.DrawText(s, x, ysize - largeFontHeight / 2 - smallFontHeight / 2);
            x += dc.GetTextExtent(s).GetWidth();

            dc.SetFont(largeFont);
            s = wxString::Format(_T("%.2f"), hfd);
			dc.DrawText(s, x, ysize - largeFontHeight);
            x += dc.GetTextExtent(s).GetWidth();

            dc.SetFont(smallFont);
            s = wxString::Format(_T("  %.2f\""), hfdArcSec);
            dc.DrawText(s, x, ysize - largeFontHeight / 2 - smallFontHeight / 2);
        }
        else {
			dc.DrawText(wxString::Format(_("%s FWHM: %.2f, HFD: %.2f (%.2f\")"), profileLabel, fwhm, hfd, hfdArcSec), 5, ysize - smallFontHeight - 5);
		}
    }
    else {
        dc.DrawText(wxString::Format(_("%s FWHM: %.2f"), profileLabel, fwhm), 5, ysize - smallFontHeight - 5);
    }

	// JBW: draw zoomed guidestar subframe (todo: make constants symbolic)
    wxImage* img = pFrame->pGuider->DisplayedImage();
    double scaleFactor = pFrame->pGuider->ScaleFactor();
    if (img) {
        int xoffset = (xsize - 15) / 2;
        int width = xsize - xoffset - 5;
        if (width > ysize + 5) width = ysize - 5;
        int midwidth = width / 2;
        // grab width(30) px box around lock pos, scale by 2 & display next to profile
        double LockX = pFrame->pGuider->LockPosition().X * scaleFactor;
        double LockY = pFrame->pGuider->LockPosition().Y * scaleFactor;
        double dStarX = LockX - pFrame->pGuider->CurrentPosition().X * scaleFactor;
        double dStarY = LockY - pFrame->pGuider->CurrentPosition().Y * scaleFactor;
        // grab the subframe
        wxBitmap dBmp(*img);
        wxBitmap subDBmp = dBmp.GetSubBitmap(wxRect(ROUND(LockX)-15, ROUND(LockY)-15, 30, 30));
        wxImage subDImg = subDBmp.ConvertToImage();
        // scale by 2
        wxBitmap zoomedDBmp(subDImg.Rescale(width, width, wxIMAGE_QUALITY_HIGH));
        wxMemoryDC tmpMdc;
        tmpMdc.SelectObject(zoomedDBmp);
        // blit into profile DC
        dc.Blit(xoffset, 0, width, width, &tmpMdc, 0, 0, wxCOPY, false);
        // lines for the lock pos + red dot at star centroid
        dc.SetPen(wxPen(wxColor(0,200,0),1,wxDOT));
        dc.DrawLine(xoffset, midwidth, xoffset + width, midwidth);
        dc.DrawLine(xoffset + midwidth, 0, xoffset + midwidth, width);
        // and a small cross at the centroid
        double starX = xoffset + midwidth - dStarX * (width / 30.0) + 1, starY = midwidth - dStarY * (width / 30.0) + 1;
        if (starX >= xoffset) {
            dc.SetPen(RedPen);
            dc.DrawLine(starX - 3, starY, starX + 3, starY);
            dc.DrawLine(starX, starY - 3, starX, starY + 3);
        }
    }
}

