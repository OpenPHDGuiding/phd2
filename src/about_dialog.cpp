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

// clang-format off
wxBEGIN_EVENT_TABLE(AboutDialog, wxDialog)
    EVT_HTML_LINK_CLICKED(ABOUT_LINK,AboutDialog::OnLink)
wxEND_EVENT_TABLE();
// clang-format on

AboutDialog::AboutDialog()
    : wxDialog(pFrame, wxID_ANY, wxString::Format(_("About %s"), APPNAME), wxDefaultPosition, wxDefaultSize,
               wxCAPTION | wxCLOSE_BOX)
{
    SetBackgroundColour(*wxWHITE);

    wxBoxSizer *pSizer = new wxBoxSizer(wxHORIZONTAL);

#include "icons/phd2_64.png.h"
    wxBitmap phd2(wxBITMAP_PNG_FROM_DATA(phd2_64));
    wxStaticBitmap *pImage = new wxStaticBitmap(this, wxID_ANY, phd2);

    wxFileSystem::AddHandler(new wxMemoryFSHandler);
    wxMemoryFSHandler::AddFile("about.html",
                               wxString::Format("<html><body>"
                                                "<font size=\"5\">"
                                                "<b>%s %s</b></font><br>"
                                                "<table CELLPADDING=0 cellspacing=10><tr><td>"
                                                "<a href=\"https://openphdguiding.org\">PHD2 home - openphdguiding.org</a> "
                                                "</td><td>"
                                                "<a href=\"https://github.com/OpenPHDGuiding\">Source code on GitHub</a>"
                                                "</td></tr></table>"
                                                "<font size=\"2\">"
                                                "Project maintainers<br>"
                                                "<table CELLPADDING=0 cellspacing=10>"
                                                "<tr>"
                                                "<td>Andy Galasso</td>"
                                                "<td>Bruce Waddington</td>"
                                                "</tr>"
                                                "</table>"
                                                "<br>"
                                                "Past maintainers<br>"
                                                "<table CELLPADDING=0 cellspacing=10>"
                                                "<tr>"
                                                "<td>Craig Stark</td>"
                                                "<td>Bret McKee</td>"
                                                "</tr>"
                                                "</table>"
                                                "<br>"
                                                "Credits<br>"
                                                "<table CELLPADDING=0 cellspacing=5>"
                                                "<tr>"
                                                "<td>Bernhard Reutner-Fischer</td>"
                                                "<td>Stefan Elste</td>"
                                                "<td>Geoffrey Hausheer</td>"
                                                "<td>Jared Wellman</td>"
                                                "<td>John Wainwright</td>"
                                                "</tr>"
                                                "<tr>"
                                                "<td>Sylvain Girard</td>"
                                                "<td>Max Chen</td>"
                                                "<td>Carsten Przygoda</td>"
                                                "<td>Hans Lambermont</td>"
                                                "<td>David Ault</td>"
                                                "</tr>"
                                                "<tr>"
                                                "<td>Markus Wieczorek</td>"
                                                "<td>Linkage</td>"
                                                "<td>Robin Glover</td>"
                                                "<td>Patrick Chevalley</td>"
                                                "<td>Scott Edwards</td>"
                                                "</tr>"
                                                "<tr>"
                                                "<td>Eiji Kaneshige</td>"
                                                "<td>Konstantin Menshikoff</td>"
                                                "<td>Jakub Bartas</td>"
                                                "<td>Javier R</td>"
                                                "<td>Oleh Malyi</td>"
                                                "</tr>"
                                                "<tr>"
                                                "<td>Tsung-Chi Wu</td>"
                                                "<td>Raffi Enficiaud</td>"
                                                "<td>Sabin Fota</td>"
                                                "<td>Dylan O'Donnell</td>"
                                                "<td>Katsuhiro Kojima</td>"
                                                "</tr>"
                                                "<tr>"
                                                "<td>Simon Taylor</td>"
                                                "<td>Hallgeir Holien</td>"
                                                "<td>Laurent Schmitz</td>"
                                                "<td>Atushi Sakauchi</td>"
                                                "<td>Giorgio Mazzacurati</td>"
                                                "</tr>"
                                                "<tr>"
                                                "<td>G&#252;nter Scholz</td>"
                                                "<td>Ray Gralak</td>"
                                                "<td>Khalefa Algadi</td>"
                                                "<td>David C. Partridge</td>"
                                                "<td>Matteo Ghellere</td>"
                                                "</tr>"
                                                "<tr>"
                                                "<td>norma</td>"
                                                "<td>Edgar D. Klenske</td>"
                                                "<td>Bernhard Sch&ouml;lkopf</td>"
                                                "<td>Philipp Hennig</td>"
                                                "<td>Stephan Wenninger</td>"
                                                "</tr>"
                                                "<tr>"
                                                "<td>Wagner Trindade</td>"
                                                "<td>Cyril Richard</td>"
                                                "<td>Mattia Verga</td>"
                                                "<td>Iv&#225;n Zabala</td>"
                                                "<td>Ken Self</td>"
                                                "</tr>"
                                                "<tr>"
                                                "<td>Alex Helms</td>"
                                                "<td>Randy Pufahl</td>"
                                                "<td>Jasem Mutlaq</td>"
                                                "<td>Thomas Stibor</td>"
                                                "<td>Ludovic Pollet</td>"
                                                "</tr>"
                                                "<tr>"
                                                "<td>Pawe&#322; Pleskaczy&#324;ski</td>"
                                                "<td>nabePla</td>"
                                                "<td>Philip Peake</td>"
                                                "<td>Manuel Rosales</td>"
                                                "<td>Marcel Greter</td>"
                                                "</tr>"
                                                "<tr>"
                                                "<td>Miquel Recacha</td>"
                                                "<td>Mario Nicotra</td>"
                                                "<td>Gerry Roberts</td>"
                                                "<td>Anthony Hinsinger</td>"
                                                "<td>Radek Kaczorek</td>"
                                                "</tr>"
                                                "<tr>"
                                                "<td>Sebastian Godelet</td>"
                                                "<td>Stanislav Holub</td>"
                                                "<td>Valerio Faiuolo</td>"
                                                "<td>Peter Berbee</td>"
                                                "<td>Paolo Stivanin</td>"
                                                "</tr>"
                                                "<tr>"
                                                "<td>Pawe&#322; Soja</td>"
                                                "<td>ToupTek Photonics Co., Ltd</td>"
                                                "<td>Jarno Paananen</td>"
                                                "<td>Leo Shatz</td>"
                                                "<td>Niels Rackwitz</td>"
                                                "</tr>"
                                                "<tr>"
                                                "<td>Philipp Weber</td>"
                                                "<td>Kirill M. Skorobogatov</td>"
                                                "<td>Lars Berntzon</td>"
                                                "<td>Dale Ghent</td>"
                                                "<td>Ethan Chappel</td>"
                                                "</tr>"
                                                "</table><br>"
                                                "<br>"
                                                "<br>"
                                                "Copyright 2024 PHD2 Developers<br>"
                                                "Copyright 2006-2013 Craig Stark<br>"
                                                "Copyright 2009 Geoffrey Hausheer<br>"
                                                "Copyright 2012-2013 Bret McKee<br>"
                                                "Copyright 2013 Sylvain Girard<br>"
                                                "Copyright 2013-2022 Andy Galasso<br>"
                                                "Copyright 2013-2023 Bruce Waddington<br>"
                                                "Copyright 2014 Hans Lambermont<br>"
                                                "Copyright 2014 Robin Glover<br>"
                                                "Copyright 2014-2017 Max Planck Society<br>"
                                                "Copyright 2017 Ken Self<br>"
                                                "Copyright 2019 Jasem Mutlaq<br>"
                                                "<br>"
                                                "<br>"
                                                "The Predictive PEC guide algorithm is based on<br>"
                                                "<a href=\"http://dx.doi.org/10.1109/TCST.2015.2420629\">Gaussian Process "
                                                "Based Predictive Control<br>for Periodic Error Correction</a>"
                                                "</font>"
                                                "</body></html>",
                                                APPNAME, FULLVER));
    wxHtmlWindow *pHtml;
    pHtml = new wxHtmlWindow(this, ABOUT_LINK, wxDefaultPosition, wxSize(640, 500), wxHW_SCROLLBAR_AUTO);
    pHtml->SetBorders(0);
    pHtml->LoadPage("memory:about.html");
    pHtml->SetSize(pHtml->GetInternalRepresentation()->GetWidth(), pHtml->GetInternalRepresentation()->GetHeight());

    pSizer->Add(pImage, wxSizerFlags(0).Border(wxALL, 10));
    pSizer->Add(pHtml, wxSizerFlags(0).Border(wxALL, 10));

    wxBoxSizer *pTopLevelSizer = new wxBoxSizer(wxVERTICAL);
    pTopLevelSizer->Add(pSizer, wxSizerFlags(0).Expand());
    SetSizerAndFit(pTopLevelSizer);
}

AboutDialog::~AboutDialog(void)
{
    wxMemoryFSHandler::RemoveFile("about.html");
}

void AboutDialog::OnLink(wxHtmlLinkEvent& event)
{
    wxLaunchDefaultBrowser(event.GetLinkInfo().GetHref());
}
