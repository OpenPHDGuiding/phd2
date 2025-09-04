/*
 * mock_com_interfaces.h
 * PHD Guiding - Communication Module Tests
 *
 * Mock objects for COM interfaces used in Windows communication tests
 * Provides controllable behavior for COM automation and dispatch operations
 */

#ifndef MOCK_COM_INTERFACES_H
#define MOCK_COM_INTERFACES_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>

#ifdef _WIN32
#include <windows.h>
#include <oleauto.h>
#include <comdef.h>
#include <objbase.h>
#include <winerror.h>
#else
// Define minimal COM types for non-Windows platforms (for compilation)
typedef void* IDispatch;
typedef void* IUnknown;
typedef long HRESULT;
typedef unsigned long ULONG;
typedef struct tagVARIANT VARIANT;
typedef struct tagDISPPARAMS DISPPARAMS;
typedef struct tagEXCEPINFO EXCEPINFO;
typedef unsigned int UINT;
typedef long DISPID;
typedef unsigned short VARTYPE;
typedef long LCID;
typedef unsigned short WORD;

#define S_OK 0L
#define E_FAIL 0x80004005L
#define DISP_E_UNKNOWNNAME 0x80020006L
#define DISP_E_MEMBERNOTFOUND 0x80020003L
#define VT_EMPTY 0
#define VT_I4 3
#define VT_R8 5
#define VT_BSTR 8
#define VT_BOOL 11
#define DISPATCH_METHOD 0x1
#define DISPATCH_PROPERTYGET 0x2
#define DISPATCH_PROPERTYPUT 0x4
#endif

#include <string>
#include <map>
#include <memory>
#include <vector>

// Forward declarations
class ComObjectSimulator;

// Mock IDispatch interface
class MockIDispatch {
public:
    // IUnknown methods
    MOCK_METHOD2(QueryInterface, HRESULT(const IID& riid, void** ppvObject));
    MOCK_METHOD0(AddRef, ULONG());
    MOCK_METHOD0(Release, ULONG());
    
    // IDispatch methods
    MOCK_METHOD1(GetTypeInfoCount, HRESULT(UINT* pctinfo));
    MOCK_METHOD3(GetTypeInfo, HRESULT(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo));
    MOCK_METHOD5(GetIDsOfNames, HRESULT(const IID& riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId));
    MOCK_METHOD8(Invoke, HRESULT(DISPID dispIdMember, const IID& riid, LCID lcid, WORD wFlags, 
                                DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr));
    
    // Helper methods for testing
    MOCK_METHOD2(SetProperty, void(const std::string& name, const VARIANT& value));
    MOCK_METHOD1(GetProperty, VARIANT(const std::string& name));
    MOCK_METHOD3(InvokeMethod, HRESULT(const std::string& name, const std::vector<VARIANT>& args, VARIANT* result));
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SetFailureCode, void(HRESULT code));
    
    static MockIDispatch* instance;
    static MockIDispatch* GetInstance();
    static void SetInstance(MockIDispatch* inst);
};

// Mock COM object factory
class MockComObjectFactory {
public:
    MOCK_METHOD2(CreateInstance, HRESULT(const std::string& progId, IDispatch** ppDispatch));
    MOCK_METHOD1(CoInitialize, HRESULT(void* reserved));
    MOCK_METHOD0(CoUninitialize, void());
    MOCK_METHOD2(CLSIDFromProgID, HRESULT(const std::wstring& progId, CLSID* clsid));
    MOCK_METHOD5(CoCreateInstance, HRESULT(const CLSID& clsid, IUnknown* outer, DWORD context, const IID& iid, void** object));
    
    // Helper methods for testing
    MOCK_METHOD2(RegisterObject, void(const std::string& progId, MockIDispatch* object));
    MOCK_METHOD1(UnregisterObject, void(const std::string& progId));
    MOCK_METHOD1(SetInitializationResult, void(HRESULT result));
    
    static MockComObjectFactory* instance;
    static MockComObjectFactory* GetInstance();
    static void SetInstance(MockComObjectFactory* inst);
};

// VARIANT helper for testing
class VariantHelper {
public:
    static VARIANT CreateEmpty();
    static VARIANT CreateInt(long value);
    static VARIANT CreateDouble(double value);
    static VARIANT CreateString(const std::string& value);
    static VARIANT CreateBool(bool value);
    static void Clear(VARIANT& var);
    static std::string ToString(const VARIANT& var);
    static bool IsEqual(const VARIANT& var1, const VARIANT& var2);
    static VARIANT Copy(const VARIANT& var);
};

// COM object simulator for comprehensive testing
class ComObjectSimulator {
public:
    struct ComProperty {
        VARIANT value;
        bool isReadOnly;
        bool exists;
        
        ComProperty() : isReadOnly(false), exists(false) {
            value = VariantHelper::CreateEmpty();
        }
        
        ~ComProperty() {
            VariantHelper::Clear(value);
        }
    };
    
    struct ComMethod {
        std::vector<VARTYPE> parameterTypes;
        VARIANT returnValue;
        HRESULT resultCode;
        bool exists;
        
        ComMethod() : resultCode(S_OK), exists(false) {
            returnValue = VariantHelper::CreateEmpty();
        }
        
        ~ComMethod() {
            VariantHelper::Clear(returnValue);
        }
    };
    
    // Object management
    void CreateObject(const std::string& progId);
    void DestroyObject(const std::string& progId);
    bool ObjectExists(const std::string& progId) const;
    
    // Property management
    void SetProperty(const std::string& progId, const std::string& name, const VARIANT& value, bool readOnly = false);
    VARIANT GetProperty(const std::string& progId, const std::string& name) const;
    bool PropertyExists(const std::string& progId, const std::string& name) const;
    void RemoveProperty(const std::string& progId, const std::string& name);
    
    // Method management
    void SetMethod(const std::string& progId, const std::string& name, const std::vector<VARTYPE>& paramTypes, 
                   const VARIANT& returnValue, HRESULT resultCode = S_OK);
    HRESULT InvokeMethod(const std::string& progId, const std::string& name, 
                        const std::vector<VARIANT>& args, VARIANT* result) const;
    bool MethodExists(const std::string& progId, const std::string& name) const;
    void RemoveMethod(const std::string& progId, const std::string& name);
    
    // Error simulation
    void SetObjectError(const std::string& progId, HRESULT error);
    void SetPropertyError(const std::string& progId, const std::string& name, HRESULT error);
    void SetMethodError(const std::string& progId, const std::string& name, HRESULT error);
    
    // Utility methods
    void Reset();
    void SetupDefaultObjects();
    std::vector<std::string> GetRegisteredObjects() const;
    
private:
    std::map<std::string, std::map<std::string, std::unique_ptr<ComProperty>>> objectProperties;
    std::map<std::string, std::map<std::string, std::unique_ptr<ComMethod>>> objectMethods;
    std::map<std::string, HRESULT> objectErrors;
    std::map<std::pair<std::string, std::string>, HRESULT> propertyErrors;
    std::map<std::pair<std::string, std::string>, HRESULT> methodErrors;
    
    void InitializeDefaults();
};

// Helper class to manage all COM mocks
class MockComInterfacesManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
    static MockIDispatch* GetMockIDispatch();
    static MockComObjectFactory* GetMockComObjectFactory();
    static ComObjectSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupTelescopeObject();
    static void SetupCameraObject();
    static void SetupFilterWheelObject();
    static void SetupFocuserObject();
    static void SimulateComInitializationFailure();
    static void SimulateObjectCreationFailure(const std::string& progId);
    static void SimulatePropertyAccessFailure(const std::string& progId, const std::string& property);
    
private:
    static MockIDispatch* mockIDispatch;
    static MockComObjectFactory* mockComObjectFactory;
    static std::unique_ptr<ComObjectSimulator> simulator;
};

// Helper macros for COM testing
#ifdef _WIN32
#define SETUP_COM_MOCKS() MockComInterfacesManager::SetupMocks()
#define TEARDOWN_COM_MOCKS() MockComInterfacesManager::TeardownMocks()
#define RESET_COM_MOCKS() MockComInterfacesManager::ResetMocks()

#define GET_MOCK_IDISPATCH() MockComInterfacesManager::GetMockIDispatch()
#define GET_MOCK_COM_FACTORY() MockComInterfacesManager::GetMockComObjectFactory()
#define GET_COM_SIMULATOR() MockComInterfacesManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_COM_INIT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_COM_FACTORY(), CoInitialize(::testing::_)) \
        .WillOnce(::testing::Return(S_OK))

#define EXPECT_COM_INIT_FAILURE() \
    EXPECT_CALL(*GET_MOCK_COM_FACTORY(), CoInitialize(::testing::_)) \
        .WillOnce(::testing::Return(E_FAIL))

#define EXPECT_CREATE_OBJECT_SUCCESS(progId, dispatch) \
    EXPECT_CALL(*GET_MOCK_COM_FACTORY(), CreateInstance(progId, ::testing::_)) \
        .WillOnce(::testing::DoAll(::testing::SetArgPointee<1>(dispatch), ::testing::Return(S_OK)))

#define EXPECT_CREATE_OBJECT_FAILURE(progId) \
    EXPECT_CALL(*GET_MOCK_COM_FACTORY(), CreateInstance(progId, ::testing::_)) \
        .WillOnce(::testing::Return(E_FAIL))

#define EXPECT_GET_PROPERTY_SUCCESS(name, value) \
    EXPECT_CALL(*GET_MOCK_IDISPATCH(), GetProperty(name)) \
        .WillOnce(::testing::Return(value))

#define EXPECT_SET_PROPERTY_SUCCESS(name, value) \
    EXPECT_CALL(*GET_MOCK_IDISPATCH(), SetProperty(name, ::testing::_)) \
        .WillOnce(::testing::Return())

#define EXPECT_INVOKE_METHOD_SUCCESS(name, result) \
    EXPECT_CALL(*GET_MOCK_IDISPATCH(), InvokeMethod(name, ::testing::_, ::testing::_)) \
        .WillOnce(::testing::DoAll(::testing::SetArgPointee<2>(result), ::testing::Return(S_OK)))

#define EXPECT_INVOKE_METHOD_FAILURE(name, error) \
    EXPECT_CALL(*GET_MOCK_IDISPATCH(), InvokeMethod(name, ::testing::_, ::testing::_)) \
        .WillOnce(::testing::Return(error))

#else
// No-op macros for non-Windows platforms
#define SETUP_COM_MOCKS()
#define TEARDOWN_COM_MOCKS()
#define RESET_COM_MOCKS()
#define GET_MOCK_IDISPATCH() nullptr
#define GET_MOCK_COM_FACTORY() nullptr
#define GET_COM_SIMULATOR() nullptr
#define EXPECT_COM_INIT_SUCCESS()
#define EXPECT_COM_INIT_FAILURE()
#define EXPECT_CREATE_OBJECT_SUCCESS(progId, dispatch)
#define EXPECT_CREATE_OBJECT_FAILURE(progId)
#define EXPECT_GET_PROPERTY_SUCCESS(name, value)
#define EXPECT_SET_PROPERTY_SUCCESS(name, value)
#define EXPECT_INVOKE_METHOD_SUCCESS(name, result)
#define EXPECT_INVOKE_METHOD_FAILURE(name, error)
#endif

#endif // MOCK_COM_INTERFACES_H
