/*
 * mock_ascom_interfaces.h
 * PHD Guiding - Mount Module Tests
 *
 * Mock objects for ASCOM telescope interfaces
 * Provides controllable behavior for COM automation and ASCOM drivers
 */

#ifndef MOCK_ASCOM_INTERFACES_H
#define MOCK_ASCOM_INTERFACES_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>

#ifdef _WIN32
#include <windows.h>
#include <oleauto.h>
#include <comdef.h>
#endif

#include <memory>
#include <vector>
#include <map>

// Forward declarations
class ASCOMSimulator;

#ifdef _WIN32

// Mock IDispatch interface for ASCOM objects
class MockIDispatch {
public:
    // IUnknown methods
    MOCK_METHOD2(QueryInterface, HRESULT(REFIID riid, void** ppvObject));
    MOCK_METHOD0(AddRef, ULONG());
    MOCK_METHOD0(Release, ULONG());
    
    // IDispatch methods
    MOCK_METHOD1(GetTypeInfoCount, HRESULT(UINT* pctinfo));
    MOCK_METHOD3(GetTypeInfo, HRESULT(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo));
    MOCK_METHOD5(GetIDsOfNames, HRESULT(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId));
    MOCK_METHOD8(Invoke, HRESULT(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, 
                                DISPPARAMS* pDispParams, VARIANT* pVarResult, 
                                EXCEPINFO* pExcepInfo, UINT* puArgErr));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SetProperty, void(const wxString& name, const VARIANT& value));
    MOCK_METHOD1(GetProperty, VARIANT(const wxString& name));
    
    static MockIDispatch* instance;
    static MockIDispatch* GetInstance();
    static void SetInstance(MockIDispatch* inst);
};

// Mock ASCOM telescope interface
class MockASCOMTelescope {
public:
    // Connection management
    MOCK_METHOD0(get_Connected, bool());
    MOCK_METHOD1(put_Connected, void(bool connected));
    MOCK_METHOD0(get_Name, wxString());
    MOCK_METHOD0(get_Description, wxString());
    MOCK_METHOD0(get_DriverInfo, wxString());
    MOCK_METHOD0(get_DriverVersion, wxString());
    
    // Telescope capabilities
    MOCK_METHOD0(get_CanSlew, bool());
    MOCK_METHOD0(get_CanSlewAsync, bool());
    MOCK_METHOD0(get_CanPulseGuide, bool());
    MOCK_METHOD0(get_CanSetTracking, bool());
    MOCK_METHOD0(get_CanSetPierSide, bool());
    MOCK_METHOD0(get_CanSetDeclinationRate, bool());
    MOCK_METHOD0(get_CanSetRightAscensionRate, bool());
    
    // Position and tracking
    MOCK_METHOD0(get_RightAscension, double());
    MOCK_METHOD0(get_Declination, double());
    MOCK_METHOD0(get_Azimuth, double());
    MOCK_METHOD0(get_Altitude, double());
    MOCK_METHOD0(get_Tracking, bool());
    MOCK_METHOD1(put_Tracking, void(bool tracking));
    
    // Slewing operations
    MOCK_METHOD2(SlewToCoordinates, void(double ra, double dec));
    MOCK_METHOD2(SlewToCoordinatesAsync, void(double ra, double dec));
    MOCK_METHOD0(AbortSlew, void());
    MOCK_METHOD0(get_Slewing, bool());
    
    // Pulse guiding
    MOCK_METHOD2(PulseGuide, void(int direction, int duration));
    MOCK_METHOD0(get_IsPulseGuiding, bool());
    
    // Mount state
    MOCK_METHOD0(get_SideOfPier, int());
    MOCK_METHOD0(get_UTCDate, DATE());
    MOCK_METHOD0(get_SiderealTime, double());
    MOCK_METHOD0(get_SiteLatitude, double());
    MOCK_METHOD0(get_SiteLongitude, double());
    MOCK_METHOD0(get_SiteElevation, double());
    
    // Configuration
    MOCK_METHOD0(SetupDialog, void());
    MOCK_METHOD0(get_SupportedActions, wxArrayString());
    MOCK_METHOD2(Action, wxString(const wxString& actionName, const wxString& actionParameters));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SetPosition, void(double ra, double dec));
    MOCK_METHOD1(SetTracking, void(bool tracking));
    MOCK_METHOD1(SimulateSlew, void(bool success));
    MOCK_METHOD2(SimulatePulseGuide, void(int direction, bool success));
    
    static MockASCOMTelescope* instance;
    static MockASCOMTelescope* GetInstance();
    static void SetInstance(MockASCOMTelescope* inst);
};

// Mock ASCOM chooser for device selection
class MockASCOMChooser {
public:
    // Device selection
    MOCK_METHOD1(Choose, wxString(const wxString& progID));
    MOCK_METHOD0(GetProfiles, wxArrayString());
    MOCK_METHOD1(GetProfile, wxString(const wxString& progID));
    MOCK_METHOD2(SetProfile, void(const wxString& progID, const wxString& profile));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SetAvailableDevices, void(const wxArrayString& devices));
    MOCK_METHOD1(SetSelectedDevice, void(const wxString& device));
    
    static MockASCOMChooser* instance;
    static MockASCOMChooser* GetInstance();
    static void SetInstance(MockASCOMChooser* inst);
};

#endif // _WIN32

// ASCOM simulator for comprehensive testing
class ASCOMSimulator {
public:
    struct TelescopeInfo {
        wxString progID;
        wxString name;
        wxString description;
        wxString driverVersion;
        bool isConnected;
        bool canSlew;
        bool canSlewAsync;
        bool canPulseGuide;
        bool canSetTracking;
        bool isTracking;
        bool isSlewing;
        bool isPulseGuiding;
        double ra, dec;  // Current position
        double azimuth, altitude;  // Alt-az coordinates
        int sideOfPier;  // 0=East, 1=West
        double siteLatitude, siteLongitude, siteElevation;
        bool shouldFail;
        wxString lastError;
        
        TelescopeInfo() : progID("Simulator.Telescope"), name("ASCOM Simulator"),
                         description("Simulated ASCOM Telescope"), driverVersion("1.0"),
                         isConnected(false), canSlew(true), canSlewAsync(true),
                         canPulseGuide(true), canSetTracking(true), isTracking(false),
                         isSlewing(false), isPulseGuiding(false), ra(0.0), dec(0.0),
                         azimuth(0.0), altitude(0.0), sideOfPier(0),
                         siteLatitude(40.0), siteLongitude(-75.0), siteElevation(100.0),
                         shouldFail(false), lastError("") {}
    };
    
    struct ChooserInfo {
        wxArrayString availableDevices;
        wxString selectedDevice;
        bool shouldFail;
        
        ChooserInfo() : shouldFail(false) {
            availableDevices.Add("Simulator.Telescope");
            availableDevices.Add("ASCOM.Simulator.Telescope");
            selectedDevice = "Simulator.Telescope";
        }
    };
    
    // Component management
    void SetupTelescope(const TelescopeInfo& info);
    void SetupChooser(const ChooserInfo& info);
    
    // State management
    TelescopeInfo GetTelescopeInfo() const;
    ChooserInfo GetChooserInfo() const;
    
    // Connection simulation
    bool ConnectTelescope();
    bool DisconnectTelescope();
    bool IsConnected() const;
    
    // Position simulation
    void SetPosition(double ra, double dec);
    void GetPosition(double& ra, double& dec) const;
    
    // Slewing simulation
    bool StartSlew(double targetRA, double targetDec);
    bool IsSlewing() const;
    void CompleteSlew();
    void AbortSlew();
    
    // Pulse guiding simulation
    bool StartPulseGuide(int direction, int duration);
    bool IsPulseGuiding() const;
    void CompletePulseGuide();
    
    // Device selection simulation
    wxString ChooseDevice(const wxString& deviceType);
    wxArrayString GetAvailableDevices() const;
    
    // Error simulation
    void SetTelescopeError(bool error);
    void SetChooserError(bool error);
    void SetConnectionError(bool error);
    
    // Utility methods
    void Reset();
    void SetupDefaultTelescope();
    
    // COM simulation helpers
#ifdef _WIN32
    HRESULT SimulateGetProperty(const wxString& propertyName, VARIANT* result);
    HRESULT SimulateSetProperty(const wxString& propertyName, const VARIANT& value);
    HRESULT SimulateMethodCall(const wxString& methodName, const VARIANT* params, int paramCount, VARIANT* result);
#endif
    
private:
    TelescopeInfo telescopeInfo;
    ChooserInfo chooserInfo;
    
    void InitializeDefaults();
};

// Helper class to manage all ASCOM mocks
class MockASCOMManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
#ifdef _WIN32
    static MockIDispatch* GetMockDispatch();
    static MockASCOMTelescope* GetMockTelescope();
    static MockASCOMChooser* GetMockChooser();
#endif
    static ASCOMSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupConnectedTelescope();
    static void SetupTelescopeCapabilities();
    static void SetupDeviceChooser();
    static void SimulateASCOMFailure();
    static void SimulateConnectionFailure();
    
private:
#ifdef _WIN32
    static MockIDispatch* mockDispatch;
    static MockASCOMTelescope* mockTelescope;
    static MockASCOMChooser* mockChooser;
#endif
    static std::unique_ptr<ASCOMSimulator> simulator;
};

// Macros for easier mock setup in tests
#define SETUP_ASCOM_MOCKS() MockASCOMManager::SetupMocks()
#define TEARDOWN_ASCOM_MOCKS() MockASCOMManager::TeardownMocks()
#define RESET_ASCOM_MOCKS() MockASCOMManager::ResetMocks()

#ifdef _WIN32
#define GET_MOCK_DISPATCH() MockASCOMManager::GetMockDispatch()
#define GET_MOCK_ASCOM_TELESCOPE() MockASCOMManager::GetMockTelescope()
#define GET_MOCK_ASCOM_CHOOSER() MockASCOMManager::GetMockChooser()
#endif
#define GET_ASCOM_SIMULATOR() MockASCOMManager::GetSimulator()

// Helper macros for common expectations
#ifdef _WIN32
#define EXPECT_ASCOM_CONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_ASCOM_TELESCOPE(), put_Connected(true)) \
        .WillOnce(::testing::Return())

#define EXPECT_ASCOM_DISCONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_ASCOM_TELESCOPE(), put_Connected(false)) \
        .WillOnce(::testing::Return())

#define EXPECT_ASCOM_PULSE_GUIDE(direction, duration) \
    EXPECT_CALL(*GET_MOCK_ASCOM_TELESCOPE(), PulseGuide(direction, duration)) \
        .WillOnce(::testing::Return())

#define EXPECT_ASCOM_SLEW(ra, dec) \
    EXPECT_CALL(*GET_MOCK_ASCOM_TELESCOPE(), SlewToCoordinates(ra, dec)) \
        .WillOnce(::testing::Return())

#define EXPECT_ASCOM_GET_POSITION(ra, dec) \
    EXPECT_CALL(*GET_MOCK_ASCOM_TELESCOPE(), get_RightAscension()) \
        .WillOnce(::testing::Return(ra)); \
    EXPECT_CALL(*GET_MOCK_ASCOM_TELESCOPE(), get_Declination()) \
        .WillOnce(::testing::Return(dec))

#define EXPECT_ASCOM_CHOOSER_SUCCESS(device) \
    EXPECT_CALL(*GET_MOCK_ASCOM_CHOOSER(), Choose(::testing::_)) \
        .WillOnce(::testing::Return(device))
#endif

#endif // MOCK_ASCOM_INTERFACES_H
