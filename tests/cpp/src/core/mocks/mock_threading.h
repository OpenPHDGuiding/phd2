/*
 * mock_threading.h
 * PHD Guiding - Core Module Tests
 *
 * Mock objects for threading and synchronization
 * Provides controllable behavior for worker threads, mutexes, and condition variables
 */

#ifndef MOCK_THREADING_H
#define MOCK_THREADING_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <wx/thread.h>
#include <wx/event.h>
#include <memory>
#include <queue>
#include <functional>

// Forward declarations
class ThreadingSimulator;

// Mock worker thread
class MockWorkerThread {
public:
    // Thread lifecycle
    MOCK_METHOD0(Create, wxThreadError());
    MOCK_METHOD0(Run, wxThreadError());
    MOCK_METHOD0(Delete, void());
    MOCK_METHOD0(Kill, wxThreadError());
    MOCK_METHOD0(Pause, wxThreadError());
    MOCK_METHOD0(Resume, wxThreadError());
    MOCK_METHOD1(Wait, void(wxThreadWait waitMode));
    
    // Thread state
    MOCK_METHOD0(IsRunning, bool());
    MOCK_METHOD0(IsPaused, bool());
    MOCK_METHOD0(IsDetached, bool());
    MOCK_METHOD0(GetId, unsigned long());
    MOCK_METHOD0(GetPriority, unsigned int());
    MOCK_METHOD1(SetPriority, void(unsigned int priority));
    
    // Task management
    MOCK_METHOD1(QueueTask, bool(const std::function<void()>& task));
    MOCK_METHOD0(GetQueueSize, size_t());
    MOCK_METHOD0(ClearQueue, void());
    MOCK_METHOD1(SetMaxQueueSize, void(size_t maxSize));
    
    // Event handling
    MOCK_METHOD1(PostEvent, bool(wxEvent& event));
    MOCK_METHOD2(CallAfter, void(const std::function<void()>& func, int delayMs));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SimulateTaskCompletion, void(bool success));
    MOCK_METHOD0(SimulateThreadExit, void());
    
    static MockWorkerThread* instance;
    static MockWorkerThread* GetInstance();
    static void SetInstance(MockWorkerThread* inst);
};

// Mock mutex for synchronization
class MockMutex {
public:
    // Mutex operations
    MOCK_METHOD0(Lock, wxMutexError());
    MOCK_METHOD0(TryLock, wxMutexError());
    MOCK_METHOD0(Unlock, wxMutexError());
    MOCK_METHOD0(IsLocked, bool());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SimulateDeadlock, void(bool deadlock));
    
    static MockMutex* instance;
    static MockMutex* GetInstance();
    static void SetInstance(MockMutex* inst);
};

// Mock condition variable
class MockCondition {
public:
    // Condition operations
    MOCK_METHOD1(Wait, wxCondError(wxMutex& mutex));
    MOCK_METHOD2(WaitTimeout, wxCondError(wxMutex& mutex, unsigned long timeoutMs));
    MOCK_METHOD0(Signal, wxCondError());
    MOCK_METHOD0(Broadcast, wxCondError());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD0(SimulateSignal, void());
    
    static MockCondition* instance;
    static MockCondition* GetInstance();
    static void SetInstance(MockCondition* inst);
};

// Mock critical section
class MockCriticalSection {
public:
    // Critical section operations
    MOCK_METHOD0(Enter, void());
    MOCK_METHOD0(Leave, void());
    MOCK_METHOD0(TryEnter, bool());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    
    static MockCriticalSection* instance;
    static MockCriticalSection* GetInstance();
    static void SetInstance(MockCriticalSection* inst);
};

// Threading simulator for comprehensive testing
class ThreadingSimulator {
public:
    struct ThreadInfo {
        unsigned long threadId;
        bool isRunning;
        bool isPaused;
        bool isDetached;
        unsigned int priority;
        std::queue<std::function<void()>> taskQueue;
        size_t maxQueueSize;
        bool shouldFail;
        wxDateTime startTime;
        
        ThreadInfo() : threadId(0), isRunning(false), isPaused(false), 
                      isDetached(false), priority(50), maxQueueSize(100), 
                      shouldFail(false) {
            startTime = wxDateTime::Now();
        }
    };
    
    struct MutexInfo {
        bool isLocked;
        unsigned long ownerThreadId;
        int lockCount;
        bool shouldFail;
        bool simulateDeadlock;
        
        MutexInfo() : isLocked(false), ownerThreadId(0), lockCount(0), 
                     shouldFail(false), simulateDeadlock(false) {}
    };
    
    struct ConditionInfo {
        std::queue<unsigned long> waitingThreads;
        bool shouldFail;
        
        ConditionInfo() : shouldFail(false) {}
    };
    
    // Thread management
    unsigned long CreateThread(bool detached = false);
    bool StartThread(unsigned long threadId);
    bool StopThread(unsigned long threadId);
    bool PauseThread(unsigned long threadId);
    bool ResumeThread(unsigned long threadId);
    bool DeleteThread(unsigned long threadId);
    
    // Thread state
    ThreadInfo* GetThread(unsigned long threadId);
    bool IsThreadRunning(unsigned long threadId);
    bool IsThreadPaused(unsigned long threadId);
    std::vector<unsigned long> GetActiveThreads();
    
    // Task management
    bool QueueTask(unsigned long threadId, const std::function<void()>& task);
    bool ExecuteNextTask(unsigned long threadId);
    size_t GetQueueSize(unsigned long threadId);
    void ClearQueue(unsigned long threadId);
    
    // Mutex management
    int CreateMutex();
    bool LockMutex(int mutexId, unsigned long threadId);
    bool TryLockMutex(int mutexId, unsigned long threadId);
    bool UnlockMutex(int mutexId, unsigned long threadId);
    MutexInfo* GetMutex(int mutexId);
    
    // Condition variable management
    int CreateCondition();
    bool WaitCondition(int conditionId, int mutexId, unsigned long threadId, unsigned long timeoutMs = 0);
    bool SignalCondition(int conditionId);
    bool BroadcastCondition(int conditionId);
    ConditionInfo* GetCondition(int conditionId);
    
    // Event simulation
    void SimulateThreadCompletion(unsigned long threadId, bool success = true);
    void SimulateTaskCompletion(unsigned long threadId, bool success = true);
    void SimulateDeadlock(int mutexId);
    void SimulateTimeout(int conditionId);
    
    // Error simulation
    void SetThreadError(unsigned long threadId, bool error);
    void SetMutexError(int mutexId, bool error);
    void SetConditionError(int conditionId, bool error);
    
    // Utility methods
    void Reset();
    void SetupDefaultThreading();
    
    // Statistics
    int GetActiveThreadCount();
    int GetActiveMutexCount();
    int GetActiveConditionCount();
    
private:
    std::map<unsigned long, std::unique_ptr<ThreadInfo>> threads;
    std::map<int, std::unique_ptr<MutexInfo>> mutexes;
    std::map<int, std::unique_ptr<ConditionInfo>> conditions;
    
    unsigned long nextThreadId;
    int nextMutexId;
    int nextConditionId;
    
    void InitializeDefaults();
    bool CheckDeadlock(int mutexId, unsigned long threadId);
};

// Helper class to manage all threading mocks
class MockThreadingManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
    static MockWorkerThread* GetMockWorkerThread();
    static MockMutex* GetMockMutex();
    static MockCondition* GetMockCondition();
    static MockCriticalSection* GetMockCriticalSection();
    static ThreadingSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupWorkerThread();
    static void SetupSynchronization();
    static void SimulateThreadFailure();
    static void SimulateDeadlock();
    static void SimulateHighLoad();
    
private:
    static MockWorkerThread* mockWorkerThread;
    static MockMutex* mockMutex;
    static MockCondition* mockCondition;
    static MockCriticalSection* mockCriticalSection;
    static std::unique_ptr<ThreadingSimulator> simulator;
};

// Macros for easier mock setup in tests
#define SETUP_THREADING_MOCKS() MockThreadingManager::SetupMocks()
#define TEARDOWN_THREADING_MOCKS() MockThreadingManager::TeardownMocks()
#define RESET_THREADING_MOCKS() MockThreadingManager::ResetMocks()

#define GET_MOCK_WORKER_THREAD() MockThreadingManager::GetMockWorkerThread()
#define GET_MOCK_MUTEX() MockThreadingManager::GetMockMutex()
#define GET_MOCK_CONDITION() MockThreadingManager::GetMockCondition()
#define GET_MOCK_CRITICAL_SECTION() MockThreadingManager::GetMockCriticalSection()
#define GET_THREADING_SIMULATOR() MockThreadingManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_THREAD_CREATE_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_WORKER_THREAD(), Create()) \
        .WillOnce(::testing::Return(wxTHREAD_NO_ERROR))

#define EXPECT_THREAD_RUN_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_WORKER_THREAD(), Run()) \
        .WillOnce(::testing::Return(wxTHREAD_NO_ERROR))

#define EXPECT_MUTEX_LOCK_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_MUTEX(), Lock()) \
        .WillOnce(::testing::Return(wxMUTEX_NO_ERROR))

#define EXPECT_MUTEX_UNLOCK_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_MUTEX(), Unlock()) \
        .WillOnce(::testing::Return(wxMUTEX_NO_ERROR))

#define EXPECT_CONDITION_WAIT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_CONDITION(), Wait(::testing::_)) \
        .WillOnce(::testing::Return(wxCOND_NO_ERROR))

#define EXPECT_CONDITION_SIGNAL_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_CONDITION(), Signal()) \
        .WillOnce(::testing::Return(wxCOND_NO_ERROR))

#define EXPECT_TASK_QUEUE_SUCCESS(task) \
    EXPECT_CALL(*GET_MOCK_WORKER_THREAD(), QueueTask(::testing::_)) \
        .WillOnce(::testing::Return(true))

#endif // MOCK_THREADING_H
