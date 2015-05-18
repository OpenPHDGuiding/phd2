/*
 *  about_dialog.cpp
 *  PHD Guiding
 *
 *  Created by Sylvain Girard.
 *  Copyright (c) 2013 Sylvain Girard.
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

#include "phd.h"
#include "about_dialog.h"
#include <wx/fs_mem.h>
#include <wx/html/htmlwin.h>

BEGIN_EVENT_TABLE(AboutDialog, wxDialog)
    EVT_HTML_LINK_CLICKED(ABOUT_LINK,AboutDialog::OnLink)
END_EVENT_TABLE()

AboutDialog::AboutDialog(void) :
wxDialog(pFrame, wxID_ANY, _T("About ") APPNAME, wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
    SetBackgroundColour(*wxWHITE);

    wxBoxSizer *pSizer = new wxBoxSizer(wxHORIZONTAL);

    #include "icons/phd2_64.png.h"
    wxBitmap phd2(wxBITMAP_PNG_FROM_DATA(phd2_64));
    wxStaticBitmap *pImage = new wxStaticBitmap(this, wxID_ANY, phd2);

    wxFileSystem::AddHandler(new wxMemoryFSHandler);
    wxMemoryFSHandler::AddFile("about.html", wxString::Format(
        "<html><body>"
        "<h3>%s %s</h3>"
        "<a href=\"http://openphdguiding.org\">PHD2 home page - openphdguiding.org</a><br>"
        "<a href=\"https://code.google.com/p/open-phd-guiding/\">PHD2 open source project page</a><br><br>"
        "<font size=\"2\">"
        "Credits:<br>"
        "<table>"
        "<tr>"
        "<td>Craig Stark</td>"
        "<td>Bret McKee</td>"
        "</tr>"
        "<tr>"
        "<td>Andy Galasso</td>"
        "<td>Bernhard Reutner-Fischer</td>"
        "</tr>"
        "<tr>"
        "<td>Stefan Elste</td>"
        "<td>Geoffrey Hausheer</td>"
        "</tr>"
        "<tr>"
        "<td>Jared Wellman</td>"
        "<td>John Wainwright</td>"
        "</tr>"
        "<tr>"
        "<td>Sylvain Girard</td>"
        "<td>Bruce Waddington</td>"
        "</tr>"
        "<tr>"
        "<td>Max Chen</td>"
        "<td>Carsten Przygoda</td>"
        "</tr>"
        "<tr>"
        "<td>Hans Lambermont</td>"
        "<td>David Ault</td>"
        "</tr>"
        "<tr>"
        "<td>Markus Wieczorek</td>"
        "<td>Linkage</td>"
        "</tr>"
        "<tr>"
        "<td>Robin Glover</td>"
        "<td>Patrick Chevalley</td>"
        "</tr>"
        "<tr>"
        "<td>Scott Edwards</td>"
        "<td>Eiji Kaneshige</td>"
        "</tr>"
        "<tr>"
        "<td>Konstantin Menshikoff</td>"
        "<td>Jakub Bartas</td>"
        "</tr>"
        "<tr>"
        "<td>Javier R</td>"
        "<td>Oleh Malyi</td>"
        "</tr>"
        "<tr>"
        "<td>Tsung-Chi Wu</td>"
        "<td>Raffi Enficiaud</td>"
        "</tr>"
        "<tr>"
        "<td>Sabin Fota</td>"
        "<td></td>"
        "</tr>"
        "<tr>"
        "<td>Dylan O'Donnell</td>"
        "<td></td>"
        "</tr>"
        "</table><br>"
        "<br>"
        "<br>"
        "Copyright 2006-2013 Craig Stark<br>"
        "Copyright 2009 Geoffrey Hausheer<br>"
        "Copyright 2012-2013 Bret McKee<br>"
        "Copyright 2013 Sylvain Girard<br>"
        "Copyright 2013-2015 Andy Galasso<br>"
        "Copyright 2013-2014 Bruce Waddington<br>"
        "Copyright 2014 Hans Lambermont<br>"
        "Copyright 2014 Robin Glover<br>"
        "Copyright 2014-2015 Max Planck Society<br>"
        "</font>"
        "</body></html>", APPNAME, FULLVER));
    wxHtmlWindow *pHtml;
    pHtml = new wxHtmlWindow(this, ABOUT_LINK, wxDefaultPosition, wxSize(380, 540), wxHW_SCROLLBAR_AUTO);
    pHtml->SetBorders(0);
    pHtml->LoadPage("memory:about.html");
    pHtml->SetSize(pHtml->GetInternalRepresentation()->GetWidth(), pHtml->GetInternalRepresentation()->GetHeight());

    pSizer->Add(pImage, wxSizerFlags(0).Border(wxALL, 10));
    pSizer->Add(pHtml, wxSizerFlags(0).Border(wxALL, 10));

    wxBoxSizer *pTopLevelSizer = new wxBoxSizer(wxVERTICAL);
    pTopLevelSizer->Add(pSizer, wxSizerFlags(0).Expand());
    pTopLevelSizer->Add(CreateButtonSizer(wxOK), wxSizerFlags(0).Expand().Border(wxALL, 10));
    SetSizerAndFit(pTopLevelSizer);
}

AboutDialog::~AboutDialog(void)
{
    wxMemoryFSHandler::RemoveFile("about.html");
}

void AboutDialog::OnLink(wxHtmlLinkEvent & event)
{
    wxLaunchDefaultBrowser(event.GetLinkInfo().GetHref());
}
