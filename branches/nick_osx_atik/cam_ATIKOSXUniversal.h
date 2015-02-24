//
//  cam_ATIKOSXUniversal.h
//  PHD
//
//  Created by Nick Kitchener on 22/01/2015.
//  Copyright (c) 2015 Nick Kitchener. All rights reserved.
//

#ifndef __PHD__cam_ATIKOSXUniversal__
#define __PHD__cam_ATIKOSXUniversal__

#include "camera.h"


#include "ATIKLinuxDrivers.h"
#include "ATIKLinuxLegacyDrivers.h"
#include "Imager100.h"


class cam_ATIKOSXUniversalConfigDialog : public wxDialog {
    bool _debugEnabled;
    bool _previewModeEnabled;
    bool _titanModeEnabled;
    uint8_t _binning;
    bool _FIFOModeEnabled;
    std::map<std::string, uint32_t> _mapping;
    wxGrid* _gridView;
    char *_driverVersionCStr;
public:

    void setDriverVersion(char* version) { _driverVersionCStr = version; }
    bool isDebugEnabled() { return _debugEnabled; }
    void setDebugEnabled(bool v) { _debugEnabled = v; }
    
    bool isPreviewModeEnabled() { return _previewModeEnabled; }
    void setPreviewModeEnabled(bool v) { _previewModeEnabled = v; }
    bool isTitanModeEnabled() { return _titanModeEnabled; }
    void setTitanModeEnabled(bool v) { _titanModeEnabled=v; }
    uint8_t binningMode() { return _binning; }
    void setBinningMode(uint8_t v) { _binning = v; }
    bool isFIFOModeEnabled() { return _FIFOModeEnabled; }
    void setFIFOModeEnabled(bool v) { _FIFOModeEnabled = v; }
    void returnMapping(std::map<std::string,uint32_t>& mapping) { mapping=_mapping; /* = std::map<std::string,uint32_t>(_mapping); */}
    void setMapping(std::map<std::string,uint32_t>& v) { _mapping = std::map<std::string,uint32_t>(v); }
    
    cam_ATIKOSXUniversalConfigDialog(wxWindow *parent);
    void updateAgainstParameters();
    void OnOkClick(wxCommandEvent& evt);
    wxCheckBox* _checkBoxPreviewMode;
    wxCheckBox* _checkBoxAutoDarkAdjMode;
    wxSlider*   _sliderImageBinning;
    wxCheckBox* _checkBoxFIFOMode;
    wxCheckBox* _checkBoxDebugEnabled;
    wxStaticText* _staticTextDriverInfo;
    
    wxSlider *newSlider(wxWindow *parent, int val, int minval, int maxval, const wxString& tooltip);
    
    wxSpinCtrlDouble *newSpinner(wxWindow *parent, double val, double minval, double maxval, double inc,
                                 const wxString& tooltip);
    wxCheckBox *newCheckBox(wxWindow *parent, bool val, const wxString& label, const wxString& tooltip);
    void addTableEntryPair(wxWindow *parent, wxFlexGridSizer *pTable, const wxString& label, wxWindow *pControl);
};

class Camera_ATIKOSXUniversal : public GuideCamera {
public:
    static void supportedCameras(wxArrayString& list);
    static int choiceFind(const wxString& choiceFind);
    
    virtual bool    Capture(int duration, usImage& img, wxRect subframe = wxRect(0,0,0,0), bool recon=false);
    virtual bool HasNonGuiCapture(void);
    bool    Connect();
    bool    Disconnect();
    //  void    InitCapture();
    
    void ShowPropertyDialog();
    
    bool    ST4PulseGuideScope(int direction, int duration);
    void    ClearGuidePort();
    bool    Color;
    bool    HSModel;
    Camera_ATIKOSXUniversal();
    Camera_ATIKOSXUniversal(uint16_t busIdentity);
private:
    //  bool GenericCapture(int duration, usImage& img, int xsize, int ysize, int xpos, int ypos);
    bool                    _debugEnabled;
    uint16_t                _userSelectedBusId;
    ATIKLinuxDrivers*         _driversModern;
    ATIKLinuxLegacyDrivers*   _driversLegacy;
    Imager100*              _imager;
    bool                    _isLegacy;
    
    bool                    _previewModeEnabled;
    bool                    _titanModeEnabled;
    uint8_t                 _binning;
    bool                    _fifoEnabled;
    std::map<std::string,uint32_t> _legacyFTDIMappings;
    
    
    
    bool ST4HasNonGuiMove(void);
    
    void updateRegisteredFTDIChipIDs();
    void loadProfile();
    void storeProfile();
};

#endif /* defined(__PHD__cam_ATIKOSXUniversal__) */
