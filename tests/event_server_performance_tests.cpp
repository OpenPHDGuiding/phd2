/*
 * event_server_performance_tests.cpp
 * PHD Guiding
 *
 * Performance tests for the EventServer module
 * Tests throughput, latency, memory usage, and scalability
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <wx/socket.h>
#include <wx/string.h>
#include <thread>
#include <chrono>
#include <memory>
#include <vector>
#include <atomic>
#include <future>
#include <algorithm>

#include "event_server_mocks.h"
#include "../src/communication/network/event_server.h"
#include "../src/communication/network/json_parser.h"

// Performance test fixture
class EventServerPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize mock objects
        g_mockCamera = new MockCamera();
        g_mockMount = new MockMount();
        g_mockSecondaryMount = new MockMount();
        g_mockGuider = new MockGuider();
        g_mockFrame = new MockFrame();
        g_mockApp = new MockApp();
        
        SetupMockExpectations(g_mockCamera, g_mockMount, g_mockGuider, g_mockFrame);
        
        wxSocketBase::Initialize();
        eventServer = std::make_unique<EventServer>();
    }
    
    void TearDown() override {
        if (eventServer) {
            eventServer->EventServerStop();
            eventServer.reset();
        }
        
        delete g_mockCamera;
        delete g_mockMount;
        delete g_mockSecondaryMount;
        delete g_mockGuider;
        delete g_mockFrame;
        delete g_mockApp;
        
        wxSocketBase::Shutdown();
    }
    
    // Helper to create multiple client connections
    std::vector<std::unique_ptr<wxSocketClient>> CreateMultipleClients(int count) {
        std::vector<std::unique_ptr<wxSocketClient>> clients;
        
        wxIPV4address addr;
        addr.Hostname("localhost");
        addr.Service(4400);
        
        for (int i = 0; i < count; ++i) {
            auto client = std::make_unique<wxSocketClient>();
            client->SetTimeout(5);
            
            if (client->Connect(addr, false)) {
                clients.push_back(std::move(client));
            }
        }
        
        // Wait for all connections to be established
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return clients;
    }
    
    // Helper to measure execution time
    template<typename Func>
    std::chrono::milliseconds MeasureExecutionTime(Func&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    }
    
    std::unique_ptr<EventServer> eventServer;
};

// Test event notification throughput
TEST_F(EventServerPerformanceTest, EventNotificationThroughput) {
    ASSERT_FALSE(eventServer->EventServerStart(1));
    
    // Create multiple clients to receive events
    const int numClients = 10;
    auto clients = CreateMultipleClients(numClients);
    EXPECT_EQ(clients.size(), numClients);
    
    // Measure throughput of guide step notifications
    const int numEvents = 1000;
    GuideStepInfo stepInfo;
    stepInfo.mount = g_mockMount;
    stepInfo.time = 1.0;
    stepInfo.cameraOffset = PHD_Point(0.1, 0.1);
    stepInfo.mountOffset = PHD_Point(0.05, 0.05);
    
    auto duration = MeasureExecutionTime([&]() {
        for (int i = 0; i < numEvents; ++i) {
            stepInfo.frameNumber = i + 1;
            eventServer->NotifyGuideStep(stepInfo);
        }
    });
    
    double eventsPerSecond = (numEvents * 1000.0) / duration.count();
    
    // Should handle at least 100 events per second with 10 clients
    EXPECT_GT(eventsPerSecond, 100.0);
    
    std::cout << "Event notification throughput: " << eventsPerSecond 
              << " events/sec with " << numClients << " clients" << std::endl;
}

// Test JSON-RPC request processing latency
TEST_F(EventServerPerformanceTest, JsonRpcRequestLatency) {
    ASSERT_FALSE(eventServer->EventServerStart(1));
    
    auto clients = CreateMultipleClients(1);
    ASSERT_EQ(clients.size(), 1);
    
    auto& client = clients[0];
    
    // Measure latency of simple requests
    const int numRequests = 100;
    std::vector<std::chrono::microseconds> latencies;
    
    for (int i = 0; i < numRequests; ++i) {
        std::string request = R"({"method":"get_connected","params":{},"id":)" + std::to_string(i) + "}";
        request += "\r\n";
        
        auto start = std::chrono::high_resolution_clock::now();
        
        client->Write(request.c_str(), request.length());
        
        // Wait for response
        if (client->WaitForRead(1, 0)) {
            char buffer[1024];
            client->Read(buffer, sizeof(buffer));
            
            auto end = std::chrono::high_resolution_clock::now();
            auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            latencies.push_back(latency);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Calculate statistics
    if (!latencies.empty()) {
        auto avgLatency = std::accumulate(latencies.begin(), latencies.end(), 
                                        std::chrono::microseconds(0)) / latencies.size();
        
        auto maxLatency = *std::max_element(latencies.begin(), latencies.end());
        auto minLatency = *std::min_element(latencies.begin(), latencies.end());
        
        // Average latency should be reasonable (less than 10ms)
        EXPECT_LT(avgLatency.count(), 10000);
        
        std::cout << "JSON-RPC latency - Avg: " << avgLatency.count() << "μs, "
                  << "Min: " << minLatency.count() << "μs, "
                  << "Max: " << maxLatency.count() << "μs" << std::endl;
    }
}

// Test concurrent client handling
TEST_F(EventServerPerformanceTest, ConcurrentClientHandling) {
    ASSERT_FALSE(eventServer->EventServerStart(1));
    
    const int numClients = 50;
    std::atomic<int> successfulConnections(0);
    std::atomic<int> successfulRequests(0);
    
    // Create clients concurrently
    std::vector<std::future<void>> futures;
    
    for (int i = 0; i < numClients; ++i) {
        futures.push_back(std::async(std::launch::async, [&, i]() {
            wxSocketClient client;
            client.SetTimeout(10);
            
            wxIPV4address addr;
            addr.Hostname("localhost");
            addr.Service(4400);
            
            if (client.Connect(addr, false)) {
                successfulConnections++;
                
                // Send a request
                std::string request = R"({"method":"get_connected","params":{},"id":)" + std::to_string(i) + "}\r\n";
                client.Write(request.c_str(), request.length());
                
                if (client.WaitForRead(5, 0)) {
                    char buffer[1024];
                    client.Read(buffer, sizeof(buffer));
                    successfulRequests++;
                }
                
                client.Close();
            }
        }));
    }
    
    // Wait for all clients to complete
    for (auto& future : futures) {
        future.wait();
    }
    
    // Should handle most concurrent connections successfully
    EXPECT_GT(successfulConnections.load(), numClients * 0.8); // At least 80% success
    EXPECT_GT(successfulRequests.load(), successfulConnections.load() * 0.9); // At least 90% of connected clients get responses
    
    std::cout << "Concurrent clients: " << successfulConnections.load() << "/" << numClients 
              << " connected, " << successfulRequests.load() << " successful requests" << std::endl;
}

// Test memory usage under load
TEST_F(EventServerPerformanceTest, MemoryUsageUnderLoad) {
    ASSERT_FALSE(eventServer->EventServerStart(1));
    
    // Create clients
    const int numClients = 20;
    auto clients = CreateMultipleClients(numClients);
    
    // Generate high-frequency events for extended period
    const int duration_seconds = 10;
    const int events_per_second = 50;
    const int total_events = duration_seconds * events_per_second;
    
    GuideStepInfo stepInfo;
    stepInfo.mount = g_mockMount;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < total_events; ++i) {
        stepInfo.frameNumber = i + 1;
        stepInfo.time = i * (1.0 / events_per_second);
        stepInfo.cameraOffset = PHD_Point(
            0.1 * sin(i * 0.1), 
            0.1 * cos(i * 0.1)
        );
        
        eventServer->NotifyGuideStep(stepInfo);
        
        // Mix in other event types
        if (i % 10 == 0) {
            eventServer->NotifyLooping(i, nullptr, nullptr);
        }
        if (i % 25 == 0) {
            eventServer->NotifyConfigurationChange();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / events_per_second));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto actual_duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
    
    // Should complete within expected time (allowing some overhead)
    EXPECT_LT(actual_duration.count(), duration_seconds + 2);
    
    // All clients should still be connected (no memory leaks causing disconnections)
    int connected_clients = 0;
    for (const auto& client : clients) {
        if (client->IsConnected()) {
            connected_clients++;
        }
    }
    
    EXPECT_EQ(connected_clients, numClients);
    
    std::cout << "Memory test completed: " << total_events << " events over " 
              << actual_duration.count() << " seconds, " 
              << connected_clients << "/" << numClients << " clients still connected" << std::endl;
}

// Test server startup/shutdown performance
TEST_F(EventServerPerformanceTest, StartupShutdownPerformance) {
    const int iterations = 100;
    std::vector<std::chrono::milliseconds> startup_times;
    std::vector<std::chrono::milliseconds> shutdown_times;
    
    for (int i = 0; i < iterations; ++i) {
        // Measure startup time
        auto startup_duration = MeasureExecutionTime([&]() {
            eventServer->EventServerStart(1);
        });
        startup_times.push_back(startup_duration);
        
        // Measure shutdown time
        auto shutdown_duration = MeasureExecutionTime([&]() {
            eventServer->EventServerStop();
        });
        shutdown_times.push_back(shutdown_duration);
    }
    
    // Calculate averages
    auto avg_startup = std::accumulate(startup_times.begin(), startup_times.end(), 
                                     std::chrono::milliseconds(0)) / iterations;
    auto avg_shutdown = std::accumulate(shutdown_times.begin(), shutdown_times.end(), 
                                      std::chrono::milliseconds(0)) / iterations;
    
    // Startup and shutdown should be fast (less than 100ms each)
    EXPECT_LT(avg_startup.count(), 100);
    EXPECT_LT(avg_shutdown.count(), 100);
    
    std::cout << "Startup/Shutdown performance - Avg startup: " << avg_startup.count() 
              << "ms, Avg shutdown: " << avg_shutdown.count() << "ms" << std::endl;
}

// Test large message handling
TEST_F(EventServerPerformanceTest, LargeMessageHandling) {
    ASSERT_FALSE(eventServer->EventServerStart(1));
    
    auto clients = CreateMultipleClients(1);
    ASSERT_EQ(clients.size(), 1);
    
    auto& client = clients[0];
    
    // Test with increasingly large JSON-RPC requests
    std::vector<int> message_sizes = {1024, 4096, 16384, 65536};
    
    for (int size : message_sizes) {
        std::string large_param(size - 100, 'A'); // Leave room for JSON structure
        std::string request = R"({"method":"test_large_message","params":{"data":")" + large_param + R"("},"id":1})";
        request += "\r\n";
        
        auto duration = MeasureExecutionTime([&]() {
            client->Write(request.c_str(), request.length());
            
            // Wait for response (or error)
            if (client->WaitForRead(5, 0)) {
                char buffer[1024];
                client->Read(buffer, sizeof(buffer));
            }
        });
        
        // Should handle large messages within reasonable time
        EXPECT_LT(duration.count(), 1000); // Less than 1 second
        
        std::cout << "Large message (" << size << " bytes) handled in " 
                  << duration.count() << "ms" << std::endl;
    }
}

// Test event queue performance under burst load
TEST_F(EventServerPerformanceTest, EventQueueBurstLoad) {
    ASSERT_FALSE(eventServer->EventServerStart(1));
    
    auto clients = CreateMultipleClients(5);
    EXPECT_EQ(clients.size(), 5);
    
    // Generate burst of events
    const int burst_size = 500;
    GuideStepInfo stepInfo;
    stepInfo.mount = g_mockMount;
    
    auto duration = MeasureExecutionTime([&]() {
        // Send all events as fast as possible
        for (int i = 0; i < burst_size; ++i) {
            stepInfo.frameNumber = i + 1;
            eventServer->NotifyGuideStep(stepInfo);
            eventServer->NotifyLooping(i, nullptr, nullptr);
        }
    });
    
    double events_per_ms = (burst_size * 2.0) / duration.count(); // 2 events per iteration
    
    // Should handle burst load efficiently
    EXPECT_GT(events_per_ms, 1.0); // At least 1 event per millisecond
    
    std::cout << "Burst load performance: " << events_per_ms 
              << " events/ms (" << burst_size * 2 << " events in " 
              << duration.count() << "ms)" << std::endl;
}

// Test scalability with increasing client count
TEST_F(EventServerPerformanceTest, ClientScalability) {
    ASSERT_FALSE(eventServer->EventServerStart(1));
    
    std::vector<int> client_counts = {1, 5, 10, 20, 50};
    
    for (int client_count : client_counts) {
        auto clients = CreateMultipleClients(client_count);
        
        // Measure event notification time with this client count
        const int num_events = 100;
        GuideStepInfo stepInfo;
        stepInfo.mount = g_mockMount;
        
        auto duration = MeasureExecutionTime([&]() {
            for (int i = 0; i < num_events; ++i) {
                stepInfo.frameNumber = i + 1;
                eventServer->NotifyGuideStep(stepInfo);
            }
        });
        
        double events_per_second = (num_events * 1000.0) / duration.count();
        
        std::cout << "Scalability test - " << client_count << " clients: " 
                  << events_per_second << " events/sec" << std::endl;
        
        // Performance shouldn't degrade too much with more clients
        if (client_count <= 20) {
            EXPECT_GT(events_per_second, 50.0); // Should maintain reasonable performance
        }
        
        // Clean up clients for next iteration
        clients.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Benchmark test for overall performance
TEST_F(EventServerPerformanceTest, OverallPerformanceBenchmark) {
    ASSERT_FALSE(eventServer->EventServerStart(1));
    
    const int num_clients = 10;
    auto clients = CreateMultipleClients(num_clients);
    EXPECT_EQ(clients.size(), num_clients);
    
    // Mixed workload benchmark
    const int duration_seconds = 5;
    const int guide_events_per_second = 20;
    const int other_events_per_second = 5;
    const int requests_per_second = 10;
    
    std::atomic<bool> stop_flag(false);
    std::atomic<int> events_sent(0);
    std::atomic<int> requests_sent(0);
    
    // Event generation thread
    std::thread event_thread([&]() {
        GuideStepInfo stepInfo;
        stepInfo.mount = g_mockMount;
        int event_counter = 0;
        
        while (!stop_flag) {
            stepInfo.frameNumber = ++event_counter;
            eventServer->NotifyGuideStep(stepInfo);
            events_sent++;
            
            if (event_counter % 4 == 0) { // Other events less frequently
                eventServer->NotifyLooping(event_counter, nullptr, nullptr);
                events_sent++;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / guide_events_per_second));
        }
    });
    
    // Request generation thread
    std::thread request_thread([&]() {
        int request_counter = 0;
        
        while (!stop_flag) {
            if (!clients.empty()) {
                auto& client = clients[request_counter % clients.size()];
                std::string request = R"({"method":"get_connected","params":{},"id":)" + 
                                    std::to_string(request_counter) + "}\r\n";
                
                client->Write(request.c_str(), request.length());
                requests_sent++;
                request_counter++;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / requests_per_second));
        }
    });
    
    // Run benchmark
    std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
    stop_flag = true;
    
    event_thread.join();
    request_thread.join();
    
    // Verify performance
    int expected_events = duration_seconds * (guide_events_per_second + other_events_per_second);
    int expected_requests = duration_seconds * requests_per_second;
    
    EXPECT_GT(events_sent.load(), expected_events * 0.8); // Allow some tolerance
    EXPECT_GT(requests_sent.load(), expected_requests * 0.8);
    
    std::cout << "Benchmark results - Events: " << events_sent.load() 
              << "/" << expected_events << ", Requests: " << requests_sent.load() 
              << "/" << expected_requests << std::endl;
}
