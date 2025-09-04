/*
 * mock_threading.cpp
 * PHD Guiding - Core Module Tests
 *
 * Implementation of mock threading objects
 */

#include "mock_threading.h"
#include <algorithm>

// Static instance declarations
MockWorkerThread* MockWorkerThread::instance = nullptr;
MockMutex* MockMutex::instance = nullptr;
MockCondition* MockCondition::instance = nullptr;
MockCriticalSection* MockCriticalSection::instance = nullptr;

// MockThreadingManager static members
MockWorkerThread* MockThreadingManager::mockWorkerThread = nullptr;
MockMutex* MockThreadingManager::mockMutex = nullptr;
MockCondition* MockThreadingManager::mockCondition = nullptr;
MockCriticalSection* MockThreadingManager::mockCriticalSection = nullptr;
std::unique_ptr<ThreadingSimulator> MockThreadingManager::simulator = nullptr;

// MockWorkerThread implementation
MockWorkerThread* MockWorkerThread::GetInstance() {
    return instance;
}

void MockWorkerThread::SetInstance(MockWorkerThread* inst) {
    instance = inst;
}

// MockMutex implementation
MockMutex* MockMutex::GetInstance() {
    return instance;
}

void MockMutex::SetInstance(MockMutex* inst) {
    instance = inst;
}

// MockCondition implementation
MockCondition* MockCondition::GetInstance() {
    return instance;
}

void MockCondition::SetInstance(MockCondition* inst) {
    instance = inst;
}

// MockCriticalSection implementation
MockCriticalSection* MockCriticalSection::GetInstance() {
    return instance;
}

void MockCriticalSection::SetInstance(MockCriticalSection* inst) {
    instance = inst;
}

// ThreadingSimulator implementation
unsigned long ThreadingSimulator::CreateThread(bool detached) {
    auto thread = std::make_unique<ThreadInfo>();
    thread->threadId = ++nextThreadId;
    thread->isDetached = detached;
    
    unsigned long threadId = thread->threadId;
    threads[threadId] = std::move(thread);
    
    return threadId;
}

bool ThreadingSimulator::StartThread(unsigned long threadId) {
    auto* thread = GetThread(threadId);
    if (thread && !thread->shouldFail) {
        thread->isRunning = true;
        thread->isPaused = false;
        thread->startTime = wxDateTime::Now();
        return true;
    }
    return false;
}

bool ThreadingSimulator::StopThread(unsigned long threadId) {
    auto* thread = GetThread(threadId);
    if (thread) {
        thread->isRunning = false;
        thread->isPaused = false;
        return true;
    }
    return false;
}

bool ThreadingSimulator::PauseThread(unsigned long threadId) {
    auto* thread = GetThread(threadId);
    if (thread && thread->isRunning && !thread->shouldFail) {
        thread->isPaused = true;
        return true;
    }
    return false;
}

bool ThreadingSimulator::ResumeThread(unsigned long threadId) {
    auto* thread = GetThread(threadId);
    if (thread && thread->isRunning && thread->isPaused && !thread->shouldFail) {
        thread->isPaused = false;
        return true;
    }
    return false;
}

bool ThreadingSimulator::DeleteThread(unsigned long threadId) {
    auto it = threads.find(threadId);
    if (it != threads.end()) {
        threads.erase(it);
        return true;
    }
    return false;
}

ThreadingSimulator::ThreadInfo* ThreadingSimulator::GetThread(unsigned long threadId) {
    auto it = threads.find(threadId);
    return (it != threads.end()) ? it->second.get() : nullptr;
}

bool ThreadingSimulator::IsThreadRunning(unsigned long threadId) {
    auto* thread = GetThread(threadId);
    return thread && thread->isRunning && !thread->isPaused;
}

bool ThreadingSimulator::IsThreadPaused(unsigned long threadId) {
    auto* thread = GetThread(threadId);
    return thread && thread->isRunning && thread->isPaused;
}

std::vector<unsigned long> ThreadingSimulator::GetActiveThreads() {
    std::vector<unsigned long> activeThreads;
    for (const auto& pair : threads) {
        if (pair.second->isRunning) {
            activeThreads.push_back(pair.first);
        }
    }
    return activeThreads;
}

bool ThreadingSimulator::QueueTask(unsigned long threadId, const std::function<void()>& task) {
    auto* thread = GetThread(threadId);
    if (thread && !thread->shouldFail && thread->taskQueue.size() < thread->maxQueueSize) {
        thread->taskQueue.push(task);
        return true;
    }
    return false;
}

bool ThreadingSimulator::ExecuteNextTask(unsigned long threadId) {
    auto* thread = GetThread(threadId);
    if (thread && thread->isRunning && !thread->isPaused && !thread->taskQueue.empty()) {
        auto task = thread->taskQueue.front();
        thread->taskQueue.pop();
        
        try {
            task();
            return true;
        } catch (...) {
            return false;
        }
    }
    return false;
}

size_t ThreadingSimulator::GetQueueSize(unsigned long threadId) {
    auto* thread = GetThread(threadId);
    return thread ? thread->taskQueue.size() : 0;
}

void ThreadingSimulator::ClearQueue(unsigned long threadId) {
    auto* thread = GetThread(threadId);
    if (thread) {
        while (!thread->taskQueue.empty()) {
            thread->taskQueue.pop();
        }
    }
}

int ThreadingSimulator::CreateMutex() {
    auto mutex = std::make_unique<MutexInfo>();
    int mutexId = ++nextMutexId;
    mutexes[mutexId] = std::move(mutex);
    return mutexId;
}

bool ThreadingSimulator::LockMutex(int mutexId, unsigned long threadId) {
    auto* mutex = GetMutex(mutexId);
    if (!mutex || mutex->shouldFail) {
        return false;
    }
    
    if (CheckDeadlock(mutexId, threadId)) {
        return false; // Deadlock detected
    }
    
    if (!mutex->isLocked) {
        mutex->isLocked = true;
        mutex->ownerThreadId = threadId;
        mutex->lockCount = 1;
        return true;
    } else if (mutex->ownerThreadId == threadId) {
        // Recursive lock
        mutex->lockCount++;
        return true;
    }
    
    return false; // Already locked by another thread
}

bool ThreadingSimulator::TryLockMutex(int mutexId, unsigned long threadId) {
    auto* mutex = GetMutex(mutexId);
    if (!mutex || mutex->shouldFail) {
        return false;
    }
    
    if (!mutex->isLocked) {
        mutex->isLocked = true;
        mutex->ownerThreadId = threadId;
        mutex->lockCount = 1;
        return true;
    }
    
    return false; // Already locked
}

bool ThreadingSimulator::UnlockMutex(int mutexId, unsigned long threadId) {
    auto* mutex = GetMutex(mutexId);
    if (!mutex || mutex->shouldFail) {
        return false;
    }
    
    if (mutex->isLocked && mutex->ownerThreadId == threadId) {
        mutex->lockCount--;
        if (mutex->lockCount == 0) {
            mutex->isLocked = false;
            mutex->ownerThreadId = 0;
        }
        return true;
    }
    
    return false; // Not locked by this thread
}

ThreadingSimulator::MutexInfo* ThreadingSimulator::GetMutex(int mutexId) {
    auto it = mutexes.find(mutexId);
    return (it != mutexes.end()) ? it->second.get() : nullptr;
}

int ThreadingSimulator::CreateCondition() {
    auto condition = std::make_unique<ConditionInfo>();
    int conditionId = ++nextConditionId;
    conditions[conditionId] = std::move(condition);
    return conditionId;
}

bool ThreadingSimulator::WaitCondition(int conditionId, int mutexId, unsigned long threadId, unsigned long timeoutMs) {
    auto* condition = GetCondition(conditionId);
    auto* mutex = GetMutex(mutexId);
    
    if (!condition || !mutex || condition->shouldFail) {
        return false;
    }
    
    // Add thread to waiting list
    condition->waitingThreads.push(threadId);
    
    // Release mutex
    UnlockMutex(mutexId, threadId);
    
    // In real implementation, would wait for signal or timeout
    // For simulation, we'll assume immediate success unless timeout is 0
    if (timeoutMs == 0) {
        return false; // Immediate timeout
    }
    
    // Re-acquire mutex
    return LockMutex(mutexId, threadId);
}

bool ThreadingSimulator::SignalCondition(int conditionId) {
    auto* condition = GetCondition(conditionId);
    if (!condition || condition->shouldFail) {
        return false;
    }
    
    if (!condition->waitingThreads.empty()) {
        condition->waitingThreads.pop(); // Wake up one waiting thread
    }
    
    return true;
}

bool ThreadingSimulator::BroadcastCondition(int conditionId) {
    auto* condition = GetCondition(conditionId);
    if (!condition || condition->shouldFail) {
        return false;
    }
    
    // Wake up all waiting threads
    while (!condition->waitingThreads.empty()) {
        condition->waitingThreads.pop();
    }
    
    return true;
}

ThreadingSimulator::ConditionInfo* ThreadingSimulator::GetCondition(int conditionId) {
    auto it = conditions.find(conditionId);
    return (it != conditions.end()) ? it->second.get() : nullptr;
}

void ThreadingSimulator::SimulateThreadCompletion(unsigned long threadId, bool success) {
    auto* thread = GetThread(threadId);
    if (thread) {
        thread->isRunning = false;
        thread->isPaused = false;
        if (!success) {
            thread->shouldFail = true;
        }
    }
}

void ThreadingSimulator::SimulateTaskCompletion(unsigned long threadId, bool success) {
    if (success) {
        ExecuteNextTask(threadId);
    } else {
        auto* thread = GetThread(threadId);
        if (thread && !thread->taskQueue.empty()) {
            thread->taskQueue.pop(); // Remove failed task
        }
    }
}

void ThreadingSimulator::SimulateDeadlock(int mutexId) {
    auto* mutex = GetMutex(mutexId);
    if (mutex) {
        mutex->simulateDeadlock = true;
    }
}

void ThreadingSimulator::SetThreadError(unsigned long threadId, bool error) {
    auto* thread = GetThread(threadId);
    if (thread) {
        thread->shouldFail = error;
    }
}

void ThreadingSimulator::SetMutexError(int mutexId, bool error) {
    auto* mutex = GetMutex(mutexId);
    if (mutex) {
        mutex->shouldFail = error;
    }
}

void ThreadingSimulator::SetConditionError(int conditionId, bool error) {
    auto* condition = GetCondition(conditionId);
    if (condition) {
        condition->shouldFail = error;
    }
}

void ThreadingSimulator::Reset() {
    threads.clear();
    mutexes.clear();
    conditions.clear();
    
    nextThreadId = 0;
    nextMutexId = 0;
    nextConditionId = 0;
    
    SetupDefaultThreading();
}

void ThreadingSimulator::SetupDefaultThreading() {
    // Create default main thread
    CreateThread(false);
    
    // Create default mutex and condition
    CreateMutex();
    CreateCondition();
}

int ThreadingSimulator::GetActiveThreadCount() {
    int count = 0;
    for (const auto& pair : threads) {
        if (pair.second->isRunning) {
            count++;
        }
    }
    return count;
}

int ThreadingSimulator::GetActiveMutexCount() {
    return static_cast<int>(mutexes.size());
}

int ThreadingSimulator::GetActiveConditionCount() {
    return static_cast<int>(conditions.size());
}

bool ThreadingSimulator::CheckDeadlock(int mutexId, unsigned long threadId) {
    auto* mutex = GetMutex(mutexId);
    return mutex && mutex->simulateDeadlock;
}

// MockThreadingManager implementation
void MockThreadingManager::SetupMocks() {
    // Create all mock instances
    mockWorkerThread = new MockWorkerThread();
    mockMutex = new MockMutex();
    mockCondition = new MockCondition();
    mockCriticalSection = new MockCriticalSection();
    
    // Set static instances
    MockWorkerThread::SetInstance(mockWorkerThread);
    MockMutex::SetInstance(mockMutex);
    MockCondition::SetInstance(mockCondition);
    MockCriticalSection::SetInstance(mockCriticalSection);
    
    // Create simulator
    simulator = std::make_unique<ThreadingSimulator>();
    simulator->SetupDefaultThreading();
}

void MockThreadingManager::TeardownMocks() {
    // Clean up all mock instances
    delete mockWorkerThread;
    delete mockMutex;
    delete mockCondition;
    delete mockCriticalSection;
    
    // Reset pointers
    mockWorkerThread = nullptr;
    mockMutex = nullptr;
    mockCondition = nullptr;
    mockCriticalSection = nullptr;
    
    // Reset static instances
    MockWorkerThread::SetInstance(nullptr);
    MockMutex::SetInstance(nullptr);
    MockCondition::SetInstance(nullptr);
    MockCriticalSection::SetInstance(nullptr);
    
    // Clean up simulator
    simulator.reset();
}

void MockThreadingManager::ResetMocks() {
    if (mockWorkerThread) {
        testing::Mock::VerifyAndClearExpectations(mockWorkerThread);
    }
    if (mockMutex) {
        testing::Mock::VerifyAndClearExpectations(mockMutex);
    }
    if (mockCondition) {
        testing::Mock::VerifyAndClearExpectations(mockCondition);
    }
    if (mockCriticalSection) {
        testing::Mock::VerifyAndClearExpectations(mockCriticalSection);
    }
    
    if (simulator) {
        simulator->Reset();
    }
}

// Getter methods
MockWorkerThread* MockThreadingManager::GetMockWorkerThread() { return mockWorkerThread; }
MockMutex* MockThreadingManager::GetMockMutex() { return mockMutex; }
MockCondition* MockThreadingManager::GetMockCondition() { return mockCondition; }
MockCriticalSection* MockThreadingManager::GetMockCriticalSection() { return mockCriticalSection; }
ThreadingSimulator* MockThreadingManager::GetSimulator() { return simulator.get(); }

void MockThreadingManager::SetupWorkerThread() {
    if (simulator) {
        unsigned long threadId = simulator->CreateThread(true);
        simulator->StartThread(threadId);
    }
    
    if (mockWorkerThread) {
        EXPECT_CALL(*mockWorkerThread, Create())
            .WillRepeatedly(::testing::Return(wxTHREAD_NO_ERROR));
        EXPECT_CALL(*mockWorkerThread, Run())
            .WillRepeatedly(::testing::Return(wxTHREAD_NO_ERROR));
        EXPECT_CALL(*mockWorkerThread, IsRunning())
            .WillRepeatedly(::testing::Return(true));
    }
}

void MockThreadingManager::SetupSynchronization() {
    if (simulator) {
        simulator->CreateMutex();
        simulator->CreateCondition();
    }
    
    if (mockMutex) {
        EXPECT_CALL(*mockMutex, Lock())
            .WillRepeatedly(::testing::Return(wxMUTEX_NO_ERROR));
        EXPECT_CALL(*mockMutex, Unlock())
            .WillRepeatedly(::testing::Return(wxMUTEX_NO_ERROR));
    }
    
    if (mockCondition) {
        EXPECT_CALL(*mockCondition, Signal())
            .WillRepeatedly(::testing::Return(wxCOND_NO_ERROR));
    }
}

void MockThreadingManager::SimulateThreadFailure() {
    if (mockWorkerThread) {
        EXPECT_CALL(*mockWorkerThread, Create())
            .WillRepeatedly(::testing::Return(wxTHREAD_MISC_ERROR));
        EXPECT_CALL(*mockWorkerThread, Run())
            .WillRepeatedly(::testing::Return(wxTHREAD_MISC_ERROR));
    }
}

void MockThreadingManager::SimulateDeadlock() {
    if (simulator) {
        int mutexId = simulator->CreateMutex();
        simulator->SimulateDeadlock(mutexId);
    }
    
    if (mockMutex) {
        EXPECT_CALL(*mockMutex, Lock())
            .WillRepeatedly(::testing::Return(wxMUTEX_DEAD_LOCK));
    }
}

void MockThreadingManager::SimulateHighLoad() {
    if (simulator) {
        // Create multiple threads to simulate high load
        for (int i = 0; i < 10; ++i) {
            unsigned long threadId = simulator->CreateThread(true);
            simulator->StartThread(threadId);
            
            // Queue multiple tasks
            for (int j = 0; j < 50; ++j) {
                simulator->QueueTask(threadId, []() { /* Dummy task */ });
            }
        }
    }
}
