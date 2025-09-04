/*
 * mock_phd_components.cpp
 * PHD Guiding
 *
 * Mock implementations of PHD2 components for testing
 */

#include "mock_phd_components.h"
#include <wx/string.h>

// Global mock instances for testing
MockCamera* g_mockCamera = nullptr;
MockMount* g_mockMount = nullptr;
MockGuider* g_mockGuider = nullptr;
MockFrame* g_mockFrame = nullptr;
MockConfig* g_mockConfig = nullptr;

// Mock Camera implementation
MockCamera::MockCamera() 
    : Connected(false)
    , FrameSize(1024, 768)
    , CurrentDefectMap(nullptr)
    , m_darkCount(0)
    , m_minExp(0.0)
    , m_maxExp(0.0)
{
}

void MockCamera::GetDarkLibraryProperties(int* numDarks, double* minExp, double* maxExp)
{
    *numDarks = m_darkCount;
    *minExp = m_minExp;
    *maxExp = m_maxExp;
}

void MockCamera::ClearDarks()
{
    m_darkCount = 0;
    m_minExp = 0.0;
    m_maxExp = 0.0;
}

void MockCamera::ClearDefectMap()
{
    CurrentDefectMap = nullptr;
}

void MockCamera::SetDarkLibraryProperties(int count, double minExp, double maxExp)
{
    m_darkCount = count;
    m_minExp = minExp;
    m_maxExp = maxExp;
}

// Mock Mount implementation
MockMount::MockMount()
    : m_connected(false)
    , m_calibrated(false)
{
}

bool MockMount::IsConnected() const
{
    return m_connected;
}

bool MockMount::IsCalibrated() const
{
    return m_calibrated;
}

void MockMount::SetConnected(bool connected)
{
    m_connected = connected;
}

void MockMount::SetCalibrated(bool calibrated)
{
    m_calibrated = calibrated;
}

// Mock Guider implementation
MockGuider::MockGuider()
    : m_calibrating(false)
    , m_guiding(false)
    , m_locked(false)
    , m_currentPosition(512, 384)
{
}

bool MockGuider::IsCalibratingOrGuiding() const
{
    return m_calibrating || m_guiding;
}

bool MockGuider::IsLocked() const
{
    return m_locked;
}

PHD_Point MockGuider::CurrentPosition() const
{
    return m_currentPosition;
}

void MockGuider::SetCalibrating(bool calibrating)
{
    m_calibrating = calibrating;
}

void MockGuider::SetGuiding(bool guiding)
{
    m_guiding = guiding;
}

void MockGuider::SetLocked(bool locked)
{
    m_locked = locked;
}

void MockGuider::SetCurrentPosition(const PHD_Point& pos)
{
    m_currentPosition = pos;
}

// Mock Frame implementation
MockFrame::MockFrame()
    : pGuider(nullptr)
{
}

bool MockFrame::LoadDarkLibrary()
{
    // Simulate successful dark library loading
    if (g_mockCamera && g_mockCamera->Connected)
    {
        g_mockCamera->SetDarkLibraryProperties(5, 1.0, 15.0);
        return true;
    }
    return false;
}

bool MockFrame::LoadDefectMapHandler(bool enable)
{
    // Simulate defect map loading
    if (g_mockCamera && g_mockCamera->Connected && enable)
    {
        g_mockCamera->CurrentDefectMap = (void*)0x12345678; // Mock pointer
        return true;
    }
    else if (!enable)
    {
        g_mockCamera->CurrentDefectMap = nullptr;
        return true;
    }
    return false;
}

// Mock Config implementation
MockConfig::MockConfig()
    : m_currentProfileId(1)
{
}

int MockConfig::GetCurrentProfileId() const
{
    return m_currentProfileId;
}

void MockConfig::SetCurrentProfileId(int id)
{
    m_currentProfileId = id;
}

// Test helper functions
void InitializeMockComponents()
{
    g_mockCamera = new MockCamera();
    g_mockMount = new MockMount();
    g_mockGuider = new MockGuider();
    g_mockFrame = new MockFrame();
    g_mockConfig = new MockConfig();
    
    // Set up default connected state
    g_mockCamera->Connected = true;
    g_mockMount->SetConnected(true);
    g_mockMount->SetCalibrated(true);
    g_mockFrame->pGuider = g_mockGuider;
}

void CleanupMockComponents()
{
    delete g_mockCamera;
    delete g_mockMount;
    delete g_mockGuider;
    delete g_mockFrame;
    delete g_mockConfig;
    
    g_mockCamera = nullptr;
    g_mockMount = nullptr;
    g_mockGuider = nullptr;
    g_mockFrame = nullptr;
    g_mockConfig = nullptr;
}

void ResetMockComponentsToDefaults()
{
    if (g_mockCamera)
    {
        g_mockCamera->Connected = true;
        g_mockCamera->ClearDarks();
        g_mockCamera->ClearDefectMap();
    }
    
    if (g_mockMount)
    {
        g_mockMount->SetConnected(true);
        g_mockMount->SetCalibrated(true);
    }
    
    if (g_mockGuider)
    {
        g_mockGuider->SetCalibrating(false);
        g_mockGuider->SetGuiding(false);
        g_mockGuider->SetLocked(false);
        g_mockGuider->SetCurrentPosition(PHD_Point(512, 384));
    }
    
    if (g_mockConfig)
    {
        g_mockConfig->SetCurrentProfileId(1);
    }
}

// Mock global variables that would normally be defined in PHD2
MockCamera* pCamera = nullptr;
MockMount* pMount = nullptr;
MockGuider* pGuider = nullptr;
MockFrame* pFrame = nullptr;
MockConfig* pConfig = nullptr;

// Initialize mock globals for testing
void SetupMockGlobals()
{
    pCamera = g_mockCamera;
    pMount = g_mockMount;
    pGuider = g_mockGuider;
    pFrame = g_mockFrame;
    pConfig = g_mockConfig;
}
