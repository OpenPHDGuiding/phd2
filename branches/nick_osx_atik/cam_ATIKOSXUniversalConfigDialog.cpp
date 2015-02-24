//
//  cam_ATIKOSXUniversalConfigDialog.cpp
//  PHD
//
//  Created by Nick Kitchener on 30/01/2015.
//  Copyright (c) 2015 open-phd-guiding. All rights reserved.
//
#include "phd.h"
#include "camera.h"
#include "cam_ATIKOSXUniversal.h"
#include <sstream>

// Camera Modes
// [x] Preview Mode
// [x] Titan autodark plantary Mode (Titan only)
//
// Camera Binning
// [n] x [n] Binning
//
// Legacy Camera
// [x] FIFO Enabled
// Legacy FTDIChipMappings
// [ [n]->[n] ]
// [ [n]->[n] ]
//
// [OK] [Cancel]

cam_ATIKOSXUniversalConfigDialog::cam_ATIKOSXUniversalConfigDialog(wxWindow *parent )
: wxDialog(parent, wxID_ANY, _("ATIK Camera Configuration"))
{
    _mapping = std::map<std::string, uint32_t>();
    _previewModeEnabled = false;
    _titanModeEnabled = false;
    _FIFOModeEnabled = true;
    _binning = 1;
    _driverVersionCStr = (char*) std::string().c_str();
    
    
    wxBoxSizer *pVSizer = new wxBoxSizer(wxVERTICAL);
    
    // Driver info and (debug?)
    
    wxStaticBoxSizer *pDriverGroup = new wxStaticBoxSizer(wxVERTICAL, this, _("Driver"));
    pVSizer->Add(pDriverGroup, wxSizerFlags().Border(wxALL, 10).Expand());
    
    wxFlexGridSizer *pDriverLayout = new wxFlexGridSizer(2, 1, 15, 15);
    pDriverGroup->Add(pDriverLayout);
    
    
    wxString driverInfo = _("Driver Info Here");
    _staticTextDriverInfo = new wxStaticText(this,1, driverInfo, wxDefaultPosition, wxDefaultSize, 0, driverInfo);
    pDriverLayout->Add(_staticTextDriverInfo, wxALL);

    _checkBoxDebugEnabled = newCheckBox(this, 1, _("Enable Debug Logging"), _("Enable Debug Logging"));
    pDriverLayout->Add(_checkBoxDebugEnabled, wxALL);
    
    
    wxStaticBoxSizer *pCamGroup = new wxStaticBoxSizer(wxVERTICAL, this, _("Camera"));
    pVSizer->Add(pCamGroup, wxSizerFlags().Border(wxALL, 10).Expand());
    
    wxFlexGridSizer *pCamLayout = new wxFlexGridSizer(3, 1, 15, 15);
    pCamGroup->Add(pCamLayout);

    _checkBoxPreviewMode = newCheckBox(this, 1, _("Enable Preview Mode"), _("Enable Preview Mode"));
    _checkBoxAutoDarkAdjMode = newCheckBox(this, 1, _("Auto-dark Adjust Mode"), _("Enable Automatic darkness adjustment (Titan Camera only)"));
    
    // until I've sorted out the binning and OpenPHD return..
    //_sliderImageBinning = newSlider(this, 1 , 1, 4, _("Image Binning"));
  
    
    pCamLayout->Add(_checkBoxPreviewMode, wxALL);
    pCamLayout->Add(_checkBoxAutoDarkAdjMode, wxALL);
    //pCamLayout->Add(_sliderImageBinning, wxALL);
    
    // Legacy
    
    wxStaticBoxSizer *pLegacyGroup = new wxStaticBoxSizer(wxVERTICAL, this, _("Legacy"));
    pVSizer->Add(pLegacyGroup, wxSizerFlags().Border(wxALL, 10).Expand());
    
    wxFlexGridSizer *pLegacyLayout = new wxFlexGridSizer(2, 1, 15, 15);
    pLegacyGroup->Add(pLegacyLayout);

    _checkBoxFIFOMode = newCheckBox(this, 1, _("Enable FIFO"), _("Enable FIFO"));
    pLegacyLayout->Add(_checkBoxFIFOMode, wxALL);
    
    _gridView = new wxGrid(this,
                                  -1,
                                  wxDefaultPosition,
                                  wxDefaultSize );
    
    _gridView->CreateGrid(4, 2);
    _gridView->SetRowLabelSize(0);
    pLegacyLayout->Add(_gridView);
   
    _gridView->SetColLabelValue(0, _("Serial Num"));
    _gridView->SetColLabelValue(1, _("FTDIChipID"));
    
    // build table from legacy array..
    
    uint8_t i=0;
    for(std::map<std::string, uint32_t>::iterator it=_mapping.begin(); it!=_mapping.end(); it++) {
        _gridView->SetCellValue(i,0, _(it->first.c_str()));
        char varChar[255];
        sprintf(varChar, "%x", it->second);
        std::string vString = std::string(varChar);
        _gridView->SetCellValue(i,1, _(vString));
    }


//    
//    // Now deal with the buttons
    wxBoxSizer *pButtonSizer = new wxBoxSizer( wxHORIZONTAL );
    wxButton *pBtn = new wxButton(this, wxID_OK, _("OK"));
    pBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &cam_ATIKOSXUniversalConfigDialog::OnOkClick, this);
    pButtonSizer->Add(pBtn, wxSizerFlags(0).Align(0).Border(wxALL, 10));
    pButtonSizer->Add(new wxButton( this, wxID_CANCEL, _("Cancel") ),
                      wxSizerFlags(0).Align(0).Border(wxALL, 10));
    
    //position the buttons centered with no border
    pVSizer->Add( pButtonSizer, wxSizerFlags(0).Center() );

    SetSizerAndFit(pVSizer);

}

void cam_ATIKOSXUniversalConfigDialog::updateAgainstParameters() {

    _staticTextDriverInfo->SetLabel(_(_driverVersionCStr));
    _checkBoxDebugEnabled->SetValue(_debugEnabled);
    
    _checkBoxPreviewMode->SetValue(_previewModeEnabled);
    _checkBoxAutoDarkAdjMode->SetValue(_titanModeEnabled);
    //_sliderImageBinning->SetValue(_binning);
    
    // Legacy
    _checkBoxFIFOMode->SetValue(_FIFOModeEnabled);
    
    // not sure how to resize in the event of a larger grid.. I'm assuming that a subclassed grid data class
    // still would not do this automatically without a redraw event of some description..
    //_gridView->CreateGrid(4, 2);
    
    // build table from legacy array..
    
    uint8_t i=0;
    for(std::map<std::string, uint32_t>::iterator it=_mapping.begin(); it!=_mapping.end(); it++) {
        _gridView->SetCellValue(i,0, _(it->first.c_str()));
        char varChar[255];
        sprintf(varChar, "%x", it->second);
        std::string vString = std::string(varChar);
        _gridView->SetCellValue(i,1, _(vString));
    }
    
}

void cam_ATIKOSXUniversalConfigDialog::OnOkClick(wxCommandEvent& evt)
{
    bool bOk = true;
    
    _debugEnabled = _checkBoxDebugEnabled->GetValue();
    _previewModeEnabled = _checkBoxPreviewMode->GetValue();
    _titanModeEnabled = _checkBoxAutoDarkAdjMode->GetValue();
    //_binning = _sliderImageBinning->GetValue();
    _FIFOModeEnabled = _checkBoxFIFOMode->GetValue();
    
    int rows = _gridView->GetNumberRows();
    
    for(int r=0; r<rows; r++) {
        wxString wxSerialNumber = _gridView->GetCellValue(r, 0);
        wxString wxFTDIChipID = _gridView->GetCellValue(r, 1);
        std::string serialNumberString = std::string(wxSerialNumber);
        std::string chipIdString = std::string(wxFTDIChipID);
        
        // convert to number
        uint32_t mappingNumber=0;
        try {
            std::istringstream(chipIdString) >> std::hex >> mappingNumber;
            _mapping[serialNumberString] = mappingNumber;
        } catch(...) {
            // failed to convert..
        }
        
    }
    
    if (bOk)
        wxDialog::EndModal(wxID_OK);
}

wxSlider* cam_ATIKOSXUniversalConfigDialog::newSlider(wxWindow *parent, int val, int minval, int maxval, const wxString& tooltip)
{
    wxSlider *pNewCtrl = new wxSlider(parent, wxID_ANY, val, minval, maxval, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_VALUE_LABEL);
    pNewCtrl->SetToolTip(tooltip);
    return pNewCtrl;
}

wxSpinCtrlDouble* cam_ATIKOSXUniversalConfigDialog::newSpinner(wxWindow *parent, double val, double minval, double maxval, double inc,
                                    const wxString& tooltip)
{
    wxSpinCtrlDouble *pNewCtrl = new wxSpinCtrlDouble(parent, wxID_ANY, _T("foo2"), wxPoint(-1, -1),
                                                      wxDefaultSize, wxSP_ARROW_KEYS, minval, maxval, val, inc);
    pNewCtrl->SetDigits(2);
    pNewCtrl->SetToolTip(tooltip);
    return pNewCtrl;
}

wxCheckBox* cam_ATIKOSXUniversalConfigDialog::newCheckBox(wxWindow *parent, bool val, const wxString& label, const wxString& tooltip)
{
    wxCheckBox *pNewCtrl = new wxCheckBox(parent, wxID_ANY, label);
    pNewCtrl->SetValue(val);
    pNewCtrl->SetToolTip(tooltip);
    return pNewCtrl;
}

// Utility function to add the <label, input> pairs to a grid including tool-tips
void cam_ATIKOSXUniversalConfigDialog::addTableEntryPair(wxWindow *parent, wxFlexGridSizer *pTable, const wxString& label, wxWindow *pControl)
{
    wxStaticText *pLabel = new wxStaticText(parent, wxID_ANY, label + _(": "), wxPoint(-1,-1), wxSize(-1,-1));
    pTable->Add(pLabel, 1, wxALL, 5);
    pTable->Add(pControl, 1, wxALL, 5);
}