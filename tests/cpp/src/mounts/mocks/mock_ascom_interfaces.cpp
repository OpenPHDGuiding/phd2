/*
 * mock_ascom_interfaces.cpp
 * PHD Guiding - Mount Module Tests
 *
 * Implementation of mock ASCOM interface objects
 */

#include "mock_ascom_interfaces.h"

#ifdef _WIN32

// Static instance declarations
MockIDispatch* MockIDispatch::instance = nullptr;
MockASCOMTelescope* MockASCOMTelescope::instance = nullptr;
MockASCOMChooser* MockASCOMChooser::instance = nullptr;

// MockASCOMManager static members
MockIDispatch* MockASCOMManager::mockDispatch = nullptr;
MockASCOMTelescope* MockASCOMManager::mockTelescope = nullptr;
MockASCOMChooser* MockASCOMManager::mockChooser = nullptr;

// MockIDispatch implementation
MockIDispatch* MockIDispatch::GetInstance() {
    return instance;
}

void MockIDispatch::SetInstance(MockIDispatch* inst) {
    instance = inst;
}

// MockASCOMTelescope implementation
MockASCOMTelescope* MockASCOMTelescope::GetInstance() {
    return instance;
}

void MockASCOMTelescope::SetInstance(MockASCOMTelescope* inst) {
    instance = inst;
}

// MockASCOMChooser implementation
MockASCOMChooser* MockASCOMChooser::GetInstance() {
    return instance;
}

void MockASCOMChooser::SetInstance(MockASCOMChooser* inst) {
    instance = inst;
}

#endif // _WIN32

std::unique_ptr<ASCOMSimulator> MockASCOMManager::simulator = nullptr;

// ASCOMSimulator implementation
void ASCOMSimulator::SetupTelescope(const TelescopeInfo& info) {
    telescopeInfo = info;
}

void ASCOMSimulator::SetupChooser(const ChooserInfo& info) {
    chooserInfo = info;
}

ASCOMSimulator::TelescopeInfo ASCOMSimulator::GetTelescopeInfo() const {
    return telescopeInfo;
}

ASCOMSimulator::ChooserInfo ASCOMSimulator::GetChooserInfo() const {
    return chooserInfo;
}

bool ASCOMSimulator::ConnectTelescope() {
    if (telescopeInfo.shouldFail) {
        telescopeInfo.lastError = "Connection failed";
        return false;
    }
    
    telescopeInfo.isConnected = true;
    telescopeInfo.lastError = "";
    return true;
}

bool ASCOMSimulator::DisconnectTelescope() {
    telescopeInfo.isConnected = false;
    telescopeInfo.isSlewing = false;
    telescopeInfo.isPulseGuiding = false;
    return true;
}

bool ASCOMSimulator::IsConnected() const {
    return telescopeInfo.isConnected;
}

void ASCOMSimulator::SetPosition(double ra, double dec) {
    telescopeInfo.ra = ra;
    telescopeInfo.dec = dec;
    
    // Simple coordinate transformation for alt-az (simplified)
    telescopeInfo.azimuth = ra * 15.0; // Convert hours to degrees
    telescopeInfo.altitude = dec;
}

void ASCOMSimulator::GetPosition(double& ra, double& dec) const {
    ra = telescopeInfo.ra;
    dec = telescopeInfo.dec;
}

bool ASCOMSimulator::StartSlew(double targetRA, double targetDec) {
    if (!telescopeInfo.isConnected || !telescopeInfo.canSlew || telescopeInfo.shouldFail) {
        telescopeInfo.lastError = "Cannot slew";
        return false;
    }
    
    telescopeInfo.isSlewing = true;
    return true;
}

bool ASCOMSimulator::IsSlewing() const {
    return telescopeInfo.isSlewing;
}

void ASCOMSimulator::CompleteSlew() {
    telescopeInfo.isSlewing = false;
}

void ASCOMSimulator::AbortSlew() {
    telescopeInfo.isSlewing = false;
}

bool ASCOMSimulator::StartPulseGuide(int direction, int duration) {
    if (!telescopeInfo.isConnected || !telescopeInfo.canPulseGuide || telescopeInfo.shouldFail) {
        telescopeInfo.lastError = "Cannot pulse guide";
        return false;
    }
    
    telescopeInfo.isPulseGuiding = true;
    return true;
}

bool ASCOMSimulator::IsPulseGuiding() const {
    return telescopeInfo.isPulseGuiding;
}

void ASCOMSimulator::CompletePulseGuide() {
    telescopeInfo.isPulseGuiding = false;
}

wxString ASCOMSimulator::ChooseDevice(const wxString& deviceType) {
    if (chooserInfo.shouldFail) {
        return wxEmptyString;
    }
    
    return chooserInfo.selectedDevice;
}

wxArrayString ASCOMSimulator::GetAvailableDevices() const {
    return chooserInfo.availableDevices;
}

void ASCOMSimulator::SetTelescopeError(bool error) {
    telescopeInfo.shouldFail = error;
    if (error) {
        telescopeInfo.lastError = "Telescope error simulated";
    } else {
        telescopeInfo.lastError = "";
    }
}

void ASCOMSimulator::SetChooserError(bool error) {
    chooserInfo.shouldFail = error;
}

void ASCOMSimulator::SetConnectionError(bool error) {
    if (error) {
        telescopeInfo.isConnected = false;
        telescopeInfo.lastError = "Connection error";
    }
}

void ASCOMSimulator::Reset() {
    telescopeInfo = TelescopeInfo();
    chooserInfo = ChooserInfo();
    
    SetupDefaultTelescope();
}

void ASCOMSimulator::SetupDefaultTelescope() {
    // Set up default telescope
    telescopeInfo.progID = "Simulator.Telescope";
    telescopeInfo.name = "ASCOM Simulator";
    telescopeInfo.description = "Simulated ASCOM Telescope";
    telescopeInfo.driverVersion = "1.0";
    telescopeInfo.canSlew = true;
    telescopeInfo.canSlewAsync = true;
    telescopeInfo.canPulseGuide = true;
    telescopeInfo.canSetTracking = true;
    
    // Set default position (RA=12h, Dec=45°)
    SetPosition(12.0, 45.0);
    
    // Set default site (Philadelphia)
    telescopeInfo.siteLatitude = 40.0;
    telescopeInfo.siteLongitude = -75.0;
    telescopeInfo.siteElevation = 100.0;
    
    // Set up default chooser
    chooserInfo.availableDevices.Clear();
    chooserInfo.availableDevices.Add("Simulator.Telescope");
    chooserInfo.availableDevices.Add("ASCOM.Simulator.Telescope");
    chooserInfo.availableDevices.Add("ASCOM.DeviceHub.Telescope");
    chooserInfo.selectedDevice = "Simulator.Telescope";
}

#ifdef _WIN32
HRESULT ASCOMSimulator::SimulateGetProperty(const wxString& propertyName, VARIANT* result) {
    if (telescopeInfo.shouldFail) {
        return E_FAIL;
    }
    
    VariantInit(result);
    
    if (propertyName == "Connected") {
        result->vt = VT_BOOL;
        result->boolVal = telescopeInfo.isConnected ? VARIANT_TRUE : VARIANT_FALSE;
    } else if (propertyName == "RightAscension") {
        result->vt = VT_R8;
        result->dblVal = telescopeInfo.ra;
    } else if (propertyName == "Declination") {
        result->vt = VT_R8;
        result->dblVal = telescopeInfo.dec;
    } else if (propertyName == "Tracking") {
        result->vt = VT_BOOL;
        result->boolVal = telescopeInfo.isTracking ? VARIANT_TRUE : VARIANT_FALSE;
    } else if (propertyName == "Slewing") {
        result->vt = VT_BOOL;
        result->boolVal = telescopeInfo.isSlewing ? VARIANT_TRUE : VARIANT_FALSE;
    } else if (propertyName == "CanPulseGuide") {
        result->vt = VT_BOOL;
        result->boolVal = telescopeInfo.canPulseGuide ? VARIANT_TRUE : VARIANT_FALSE;
    } else if (propertyName == "Name") {
        result->vt = VT_BSTR;
        result->bstrVal = SysAllocString(telescopeInfo.name.wc_str());
    } else {
        return DISP_E_MEMBERNOTFOUND;
    }
    
    return S_OK;
}

HRESULT ASCOMSimulator::SimulateSetProperty(const wxString& propertyName, const VARIANT& value) {
    if (telescopeInfo.shouldFail) {
        return E_FAIL;
    }
    
    if (propertyName == "Connected") {
        if (value.vt == VT_BOOL) {
            bool connected = (value.boolVal == VARIANT_TRUE);
            if (connected) {
                return ConnectTelescope() ? S_OK : E_FAIL;
            } else {
                return DisconnectTelescope() ? S_OK : E_FAIL;
            }
        }
    } else if (propertyName == "Tracking") {
        if (value.vt == VT_BOOL) {
            telescopeInfo.isTracking = (value.boolVal == VARIANT_TRUE);
            return S_OK;
        }
    }
    
    return DISP_E_MEMBERNOTFOUND;
}

HRESULT ASCOMSimulator::SimulateMethodCall(const wxString& methodName, const VARIANT* params, int paramCount, VARIANT* result) {
    if (telescopeInfo.shouldFail) {
        return E_FAIL;
    }
    
    if (methodName == "PulseGuide") {
        if (paramCount == 2 && params[0].vt == VT_I4 && params[1].vt == VT_I4) {
            int direction = params[0].lVal;
            int duration = params[1].lVal;
            return StartPulseGuide(direction, duration) ? S_OK : E_FAIL;
        }
    } else if (methodName == "SlewToCoordinates") {
        if (paramCount == 2 && params[0].vt == VT_R8 && params[1].vt == VT_R8) {
            double ra = params[0].dblVal;
            double dec = params[1].dblVal;
            return StartSlew(ra, dec) ? S_OK : E_FAIL;
        }
    } else if (methodName == "AbortSlew") {
        AbortSlew();
        return S_OK;
    }
    
    return DISP_E_MEMBERNOTFOUND;
}
#endif // _WIN32

// MockASCOMManager implementation
void MockASCOMManager::SetupMocks() {
#ifdef _WIN32
    // Create all mock instances
    mockDispatch = new MockIDispatch();
    mockTelescope = new MockASCOMTelescope();
    mockChooser = new MockASCOMChooser();
    
    // Set static instances
    MockIDispatch::SetInstance(mockDispatch);
    MockASCOMTelescope::SetInstance(mockTelescope);
    MockASCOMChooser::SetInstance(mockChooser);
#endif
    
    // Create simulator
    simulator = std::make_unique<ASCOMSimulator>();
    simulator->SetupDefaultTelescope();
}

void MockASCOMManager::TeardownMocks() {
#ifdef _WIN32
    // Clean up all mock instances
    delete mockDispatch;
    delete mockTelescope;
    delete mockChooser;
    
    // Reset pointers
    mockDispatch = nullptr;
    mockTelescope = nullptr;
    mockChooser = nullptr;
    
    // Reset static instances
    MockIDispatch::SetInstance(nullptr);
    MockASCOMTelescope::SetInstance(nullptr);
    MockASCOMChooser::SetInstance(nullptr);
#endif
    
    // Clean up simulator
    simulator.reset();
}

void MockASCOMManager::ResetMocks() {
#ifdef _WIN32
    if (mockDispatch) {
        testing::Mock::VerifyAndClearExpectations(mockDispatch);
    }
    if (mockTelescope) {
        testing::Mock::VerifyAndClearExpectations(mockTelescope);
    }
    if (mockChooser) {
        testing::Mock::VerifyAndClearExpectations(mockChooser);
    }
#endif
    
    if (simulator) {
        simulator->Reset();
    }
}

// Getter methods
#ifdef _WIN32
MockIDispatch* MockASCOMManager::GetMockDispatch() { return mockDispatch; }
MockASCOMTelescope* MockASCOMManager::GetMockTelescope() { return mockTelescope; }
MockASCOMChooser* MockASCOMManager::GetMockChooser() { return mockChooser; }
#endif
ASCOMSimulator* MockASCOMManager::GetSimulator() { return simulator.get(); }

void MockASCOMManager::SetupConnectedTelescope() {
    if (simulator) {
        simulator->ConnectTelescope();
    }
    
#ifdef _WIN32
    if (mockTelescope) {
        EXPECT_CALL(*mockTelescope, get_Connected())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockTelescope, put_Connected(true))
            .WillRepeatedly(::testing::Return());
        EXPECT_CALL(*mockTelescope, get_Name())
            .WillRepeatedly(::testing::Return(wxString("ASCOM Simulator")));
    }
#endif
}

void MockASCOMManager::SetupTelescopeCapabilities() {
    SetupConnectedTelescope();
    
#ifdef _WIN32
    if (mockTelescope) {
        EXPECT_CALL(*mockTelescope, get_CanSlew())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockTelescope, get_CanSlewAsync())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockTelescope, get_CanPulseGuide())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockTelescope, get_CanSetTracking())
            .WillRepeatedly(::testing::Return(true));
    }
#endif
}

void MockASCOMManager::SetupDeviceChooser() {
#ifdef _WIN32
    if (mockChooser) {
        wxArrayString devices;
        devices.Add("Simulator.Telescope");
        devices.Add("ASCOM.Simulator.Telescope");
        
        EXPECT_CALL(*mockChooser, GetProfiles())
            .WillRepeatedly(::testing::Return(devices));
        EXPECT_CALL(*mockChooser, Choose(::testing::_))
            .WillRepeatedly(::testing::Return(wxString("Simulator.Telescope")));
    }
#endif
}

void MockASCOMManager::SimulateASCOMFailure() {
    if (simulator) {
        simulator->SetTelescopeError(true);
    }
    
#ifdef _WIN32
    if (mockTelescope) {
        EXPECT_CALL(*mockTelescope, put_Connected(true))
            .WillRepeatedly(::testing::Return());
        EXPECT_CALL(*mockTelescope, get_Connected())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockTelescope, PulseGuide(::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return());
    }
#endif
}

void MockASCOMManager::SimulateConnectionFailure() {
    if (simulator) {
        simulator->SetConnectionError(true);
    }
    
#ifdef _WIN32
    if (mockTelescope) {
        EXPECT_CALL(*mockTelescope, put_Connected(true))
            .WillRepeatedly(::testing::Return());
        EXPECT_CALL(*mockTelescope, get_Connected())
            .WillRepeatedly(::testing::Return(false));
    }
    
    if (mockChooser) {
        EXPECT_CALL(*mockChooser, Choose(::testing::_))
            .WillRepeatedly(::testing::Return(wxEmptyString));
    }
#endif
}
