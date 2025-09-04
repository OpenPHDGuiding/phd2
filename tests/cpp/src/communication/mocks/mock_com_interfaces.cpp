/*
 * mock_com_interfaces.cpp
 * PHD Guiding - Communication Module Tests
 *
 * Implementation of mock COM interfaces
 */

#include "mock_com_interfaces.h"
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <comutil.h>
#pragma comment(lib, "comsuppw.lib")
#endif

// Static instance declarations
MockIDispatch* MockIDispatch::instance = nullptr;
MockComObjectFactory* MockComObjectFactory::instance = nullptr;

// MockComInterfacesManager static members
MockIDispatch* MockComInterfacesManager::mockIDispatch = nullptr;
MockComObjectFactory* MockComInterfacesManager::mockComObjectFactory = nullptr;
std::unique_ptr<ComObjectSimulator> MockComInterfacesManager::simulator = nullptr;

// MockIDispatch implementation
MockIDispatch* MockIDispatch::GetInstance() {
    return instance;
}

void MockIDispatch::SetInstance(MockIDispatch* inst) {
    instance = inst;
}

// MockComObjectFactory implementation
MockComObjectFactory* MockComObjectFactory::GetInstance() {
    return instance;
}

void MockComObjectFactory::SetInstance(MockComObjectFactory* inst) {
    instance = inst;
}

// VariantHelper implementation
VARIANT VariantHelper::CreateEmpty() {
    VARIANT var;
#ifdef _WIN32
    VariantInit(&var);
#else
    memset(&var, 0, sizeof(var));
#endif
    return var;
}

VARIANT VariantHelper::CreateInt(long value) {
    VARIANT var = CreateEmpty();
#ifdef _WIN32
    var.vt = VT_I4;
    var.lVal = value;
#endif
    return var;
}

VARIANT VariantHelper::CreateDouble(double value) {
    VARIANT var = CreateEmpty();
#ifdef _WIN32
    var.vt = VT_R8;
    var.dblVal = value;
#endif
    return var;
}

VARIANT VariantHelper::CreateString(const std::string& value) {
    VARIANT var = CreateEmpty();
#ifdef _WIN32
    var.vt = VT_BSTR;
    var.bstrVal = _bstr_t(value.c_str()).Detach();
#endif
    return var;
}

VARIANT VariantHelper::CreateBool(bool value) {
    VARIANT var = CreateEmpty();
#ifdef _WIN32
    var.vt = VT_BOOL;
    var.boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
#endif
    return var;
}

void VariantHelper::Clear(VARIANT& var) {
#ifdef _WIN32
    VariantClear(&var);
#else
    memset(&var, 0, sizeof(var));
#endif
}

std::string VariantHelper::ToString(const VARIANT& var) {
#ifdef _WIN32
    switch (var.vt) {
        case VT_I4:
            return std::to_string(var.lVal);
        case VT_R8:
            return std::to_string(var.dblVal);
        case VT_BSTR:
            return std::string(_bstr_t(var.bstrVal));
        case VT_BOOL:
            return var.boolVal ? "true" : "false";
        case VT_EMPTY:
        default:
            return "";
    }
#else
    return "";
#endif
}

bool VariantHelper::IsEqual(const VARIANT& var1, const VARIANT& var2) {
#ifdef _WIN32
    if (var1.vt != var2.vt) return false;
    
    switch (var1.vt) {
        case VT_I4:
            return var1.lVal == var2.lVal;
        case VT_R8:
            return var1.dblVal == var2.dblVal;
        case VT_BSTR:
            return wcscmp(var1.bstrVal, var2.bstrVal) == 0;
        case VT_BOOL:
            return var1.boolVal == var2.boolVal;
        case VT_EMPTY:
            return true;
        default:
            return false;
    }
#else
    return true; // Simplified for non-Windows
#endif
}

VARIANT VariantHelper::Copy(const VARIANT& var) {
    VARIANT copy = CreateEmpty();
#ifdef _WIN32
    VariantCopy(&copy, &var);
#endif
    return copy;
}

// ComObjectSimulator implementation
void ComObjectSimulator::CreateObject(const std::string& progId) {
    objectProperties[progId] = std::map<std::string, std::unique_ptr<ComProperty>>();
    objectMethods[progId] = std::map<std::string, std::unique_ptr<ComMethod>>();
    objectErrors[progId] = S_OK;
}

void ComObjectSimulator::DestroyObject(const std::string& progId) {
    objectProperties.erase(progId);
    objectMethods.erase(progId);
    objectErrors.erase(progId);
}

bool ComObjectSimulator::ObjectExists(const std::string& progId) const {
    return objectProperties.find(progId) != objectProperties.end();
}

void ComObjectSimulator::SetProperty(const std::string& progId, const std::string& name, 
                                    const VARIANT& value, bool readOnly) {
    if (!ObjectExists(progId)) {
        CreateObject(progId);
    }
    
    auto property = std::make_unique<ComProperty>();
    property->value = VariantHelper::Copy(value);
    property->isReadOnly = readOnly;
    property->exists = true;
    
    objectProperties[progId][name] = std::move(property);
}

VARIANT ComObjectSimulator::GetProperty(const std::string& progId, const std::string& name) const {
    auto objIt = objectProperties.find(progId);
    if (objIt != objectProperties.end()) {
        auto propIt = objIt->second.find(name);
        if (propIt != objIt->second.end() && propIt->second->exists) {
            return VariantHelper::Copy(propIt->second->value);
        }
    }
    return VariantHelper::CreateEmpty();
}

bool ComObjectSimulator::PropertyExists(const std::string& progId, const std::string& name) const {
    auto objIt = objectProperties.find(progId);
    if (objIt != objectProperties.end()) {
        auto propIt = objIt->second.find(name);
        return propIt != objIt->second.end() && propIt->second->exists;
    }
    return false;
}

void ComObjectSimulator::RemoveProperty(const std::string& progId, const std::string& name) {
    auto objIt = objectProperties.find(progId);
    if (objIt != objectProperties.end()) {
        objIt->second.erase(name);
    }
}

void ComObjectSimulator::SetMethod(const std::string& progId, const std::string& name, 
                                  const std::vector<VARTYPE>& paramTypes, const VARIANT& returnValue, HRESULT resultCode) {
    if (!ObjectExists(progId)) {
        CreateObject(progId);
    }
    
    auto method = std::make_unique<ComMethod>();
    method->parameterTypes = paramTypes;
    method->returnValue = VariantHelper::Copy(returnValue);
    method->resultCode = resultCode;
    method->exists = true;
    
    objectMethods[progId][name] = std::move(method);
}

HRESULT ComObjectSimulator::InvokeMethod(const std::string& progId, const std::string& name, 
                                        const std::vector<VARIANT>& args, VARIANT* result) const {
    auto objIt = objectMethods.find(progId);
    if (objIt != objectMethods.end()) {
        auto methodIt = objIt->second.find(name);
        if (methodIt != objIt->second.end() && methodIt->second->exists) {
            const auto& method = methodIt->second;
            
            // Check parameter count
            if (args.size() != method->parameterTypes.size()) {
                return DISP_E_BADPARAMCOUNT;
            }
            
            // Copy return value if provided
            if (result) {
                *result = VariantHelper::Copy(method->returnValue);
            }
            
            return method->resultCode;
        }
    }
    return DISP_E_MEMBERNOTFOUND;
}

bool ComObjectSimulator::MethodExists(const std::string& progId, const std::string& name) const {
    auto objIt = objectMethods.find(progId);
    if (objIt != objectMethods.end()) {
        auto methodIt = objIt->second.find(name);
        return methodIt != objIt->second.end() && methodIt->second->exists;
    }
    return false;
}

void ComObjectSimulator::RemoveMethod(const std::string& progId, const std::string& name) {
    auto objIt = objectMethods.find(progId);
    if (objIt != objectMethods.end()) {
        objIt->second.erase(name);
    }
}

void ComObjectSimulator::SetObjectError(const std::string& progId, HRESULT error) {
    objectErrors[progId] = error;
}

void ComObjectSimulator::SetPropertyError(const std::string& progId, const std::string& name, HRESULT error) {
    propertyErrors[std::make_pair(progId, name)] = error;
}

void ComObjectSimulator::SetMethodError(const std::string& progId, const std::string& name, HRESULT error) {
    methodErrors[std::make_pair(progId, name)] = error;
}

void ComObjectSimulator::Reset() {
    objectProperties.clear();
    objectMethods.clear();
    objectErrors.clear();
    propertyErrors.clear();
    methodErrors.clear();
    
    SetupDefaultObjects();
}

void ComObjectSimulator::SetupDefaultObjects() {
    // Set up common ASCOM objects for testing
    CreateObject("ASCOM.Simulator.Telescope");
    SetProperty("ASCOM.Simulator.Telescope", "Connected", VariantHelper::CreateBool(false));
    SetProperty("ASCOM.Simulator.Telescope", "RightAscension", VariantHelper::CreateDouble(0.0));
    SetProperty("ASCOM.Simulator.Telescope", "Declination", VariantHelper::CreateDouble(0.0));
    SetMethod("ASCOM.Simulator.Telescope", "SlewToCoordinates", {VT_R8, VT_R8}, VariantHelper::CreateEmpty());
    
    CreateObject("ASCOM.Simulator.Camera");
    SetProperty("ASCOM.Simulator.Camera", "Connected", VariantHelper::CreateBool(false));
    SetProperty("ASCOM.Simulator.Camera", "CameraXSize", VariantHelper::CreateInt(1024));
    SetProperty("ASCOM.Simulator.Camera", "CameraYSize", VariantHelper::CreateInt(768));
    SetMethod("ASCOM.Simulator.Camera", "StartExposure", {VT_R8, VT_BOOL}, VariantHelper::CreateEmpty());
    
    CreateObject("ASCOM.Simulator.FilterWheel");
    SetProperty("ASCOM.Simulator.FilterWheel", "Connected", VariantHelper::CreateBool(false));
    SetProperty("ASCOM.Simulator.FilterWheel", "Position", VariantHelper::CreateInt(0));
    SetMethod("ASCOM.Simulator.FilterWheel", "SetPosition", {VT_I4}, VariantHelper::CreateEmpty());
}

std::vector<std::string> ComObjectSimulator::GetRegisteredObjects() const {
    std::vector<std::string> objects;
    for (const auto& pair : objectProperties) {
        objects.push_back(pair.first);
    }
    return objects;
}

// MockComInterfacesManager implementation
void MockComInterfacesManager::SetupMocks() {
#ifdef _WIN32
    // Create all mock instances
    mockIDispatch = new MockIDispatch();
    mockComObjectFactory = new MockComObjectFactory();
    
    // Set static instances
    MockIDispatch::SetInstance(mockIDispatch);
    MockComObjectFactory::SetInstance(mockComObjectFactory);
    
    // Create simulator
    simulator = std::make_unique<ComObjectSimulator>();
    simulator->SetupDefaultObjects();
#endif
}

void MockComInterfacesManager::TeardownMocks() {
#ifdef _WIN32
    // Clean up all mock instances
    delete mockIDispatch;
    delete mockComObjectFactory;
    
    // Reset pointers
    mockIDispatch = nullptr;
    mockComObjectFactory = nullptr;
    
    // Reset static instances
    MockIDispatch::SetInstance(nullptr);
    MockComObjectFactory::SetInstance(nullptr);
    
    // Clean up simulator
    simulator.reset();
#endif
}

void MockComInterfacesManager::ResetMocks() {
#ifdef _WIN32
    if (mockIDispatch) {
        testing::Mock::VerifyAndClearExpectations(mockIDispatch);
    }
    if (mockComObjectFactory) {
        testing::Mock::VerifyAndClearExpectations(mockComObjectFactory);
    }
    
    if (simulator) {
        simulator->Reset();
    }
#endif
}

// Getter methods
MockIDispatch* MockComInterfacesManager::GetMockIDispatch() { return mockIDispatch; }
MockComObjectFactory* MockComInterfacesManager::GetMockComObjectFactory() { return mockComObjectFactory; }
ComObjectSimulator* MockComInterfacesManager::GetSimulator() { return simulator.get(); }

void MockComInterfacesManager::SetupTelescopeObject() {
#ifdef _WIN32
    if (simulator) {
        simulator->CreateObject("ASCOM.Simulator.Telescope");
        simulator->SetProperty("ASCOM.Simulator.Telescope", "Connected", VariantHelper::CreateBool(true));
        simulator->SetProperty("ASCOM.Simulator.Telescope", "CanPulseGuide", VariantHelper::CreateBool(true));
    }
#endif
}

void MockComInterfacesManager::SetupCameraObject() {
#ifdef _WIN32
    if (simulator) {
        simulator->CreateObject("ASCOM.Simulator.Camera");
        simulator->SetProperty("ASCOM.Simulator.Camera", "Connected", VariantHelper::CreateBool(true));
        simulator->SetProperty("ASCOM.Simulator.Camera", "HasShutter", VariantHelper::CreateBool(true));
    }
#endif
}

void MockComInterfacesManager::SetupFilterWheelObject() {
#ifdef _WIN32
    if (simulator) {
        simulator->CreateObject("ASCOM.Simulator.FilterWheel");
        simulator->SetProperty("ASCOM.Simulator.FilterWheel", "Connected", VariantHelper::CreateBool(true));
        simulator->SetProperty("ASCOM.Simulator.FilterWheel", "Names", VariantHelper::CreateString("Red,Green,Blue,Clear"));
    }
#endif
}

void MockComInterfacesManager::SetupFocuserObject() {
#ifdef _WIN32
    if (simulator) {
        simulator->CreateObject("ASCOM.Simulator.Focuser");
        simulator->SetProperty("ASCOM.Simulator.Focuser", "Connected", VariantHelper::CreateBool(true));
        simulator->SetProperty("ASCOM.Simulator.Focuser", "Absolute", VariantHelper::CreateBool(true));
    }
#endif
}

void MockComInterfacesManager::SimulateComInitializationFailure() {
#ifdef _WIN32
    if (mockComObjectFactory) {
        EXPECT_CALL(*mockComObjectFactory, CoInitialize(::testing::_))
            .WillOnce(::testing::Return(E_FAIL));
    }
#endif
}

void MockComInterfacesManager::SimulateObjectCreationFailure(const std::string& progId) {
#ifdef _WIN32
    if (simulator) {
        simulator->SetObjectError(progId, E_FAIL);
    }
#endif
}

void MockComInterfacesManager::SimulatePropertyAccessFailure(const std::string& progId, const std::string& property) {
#ifdef _WIN32
    if (simulator) {
        simulator->SetPropertyError(progId, property, E_FAIL);
    }
#endif
}
