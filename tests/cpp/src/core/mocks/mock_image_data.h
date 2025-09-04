/*
 * mock_image_data.h
 * PHD Guiding - Core Module Tests
 *
 * Mock objects for image data generation and processing
 * Provides synthetic image data, star patterns, and noise simulation
 */

#ifndef MOCK_IMAGE_DATA_H
#define MOCK_IMAGE_DATA_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <vector>
#include <memory>
#include <random>
#include <cmath>

// Forward declarations
class ImageDataSimulator;

// Mock image data generator
class MockImageDataGenerator {
public:
    // Image creation
    MOCK_METHOD2(CreateImage, std::vector<unsigned short>(int width, int height));
    MOCK_METHOD3(CreateImageWithNoise, std::vector<unsigned short>(int width, int height, double noiseLevel));
    MOCK_METHOD4(CreateImageWithStar, std::vector<unsigned short>(int width, int height, double starX, double starY));
    MOCK_METHOD5(CreateImageWithMultipleStars, std::vector<unsigned short>(int width, int height, 
                                                                          const std::vector<double>& starX, 
                                                                          const std::vector<double>& starY));
    
    // Star pattern generation
    MOCK_METHOD4(CreateGaussianStar, std::vector<unsigned short>(int size, double amplitude, double sigma, double background));
    MOCK_METHOD5(CreateMoffatStar, std::vector<unsigned short>(int size, double amplitude, double alpha, double beta, double background));
    MOCK_METHOD3(CreateSaturatedStar, std::vector<unsigned short>(int size, double amplitude, double background));
    
    // Noise generation
    MOCK_METHOD2(AddGaussianNoise, void(std::vector<unsigned short>& image, double sigma));
    MOCK_METHOD2(AddPoissonNoise, void(std::vector<unsigned short>& image, double gain));
    MOCK_METHOD3(AddReadoutNoise, void(std::vector<unsigned short>& image, double bias, double readNoise));
    
    // Image patterns
    MOCK_METHOD2(CreateFlatField, std::vector<unsigned short>(int width, int height));
    MOCK_METHOD3(CreateDarkFrame, std::vector<unsigned short>(int width, int height, double darkCurrent));
    MOCK_METHOD3(CreateBiasFrame, std::vector<unsigned short>(int width, int height, double bias));
    MOCK_METHOD4(CreateGradient, std::vector<unsigned short>(int width, int height, double startValue, double endValue));
    
    // Defect simulation
    MOCK_METHOD3(AddHotPixels, void(std::vector<unsigned short>& image, int count, double intensity));
    MOCK_METHOD3(AddColdPixels, void(std::vector<unsigned short>& image, int count, double intensity));
    MOCK_METHOD2(AddCosmicRays, void(std::vector<unsigned short>& image, int count));
    MOCK_METHOD3(AddDefectMap, void(std::vector<unsigned short>& image, const std::vector<wxPoint>& defects, double value));
    
    // Image statistics
    MOCK_METHOD1(CalculateMean, double(const std::vector<unsigned short>& image));
    MOCK_METHOD1(CalculateStdDev, double(const std::vector<unsigned short>& image));
    MOCK_METHOD1(CalculateMedian, unsigned short(const std::vector<unsigned short>& image));
    MOCK_METHOD1(CalculateMAD, unsigned short(const std::vector<unsigned short>& image));
    MOCK_METHOD1(FindMinMax, std::pair<unsigned short, unsigned short>(const std::vector<unsigned short>& image));
    
    // Helper methods for testing
    MOCK_METHOD1(SetRandomSeed, void(unsigned int seed));
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(ValidateImageSize, bool(int width, int height));
    
    static MockImageDataGenerator* instance;
    static MockImageDataGenerator* GetInstance();
    static void SetInstance(MockImageDataGenerator* inst);
};

// Mock FITS file operations
class MockFITSOperations {
public:
    // File I/O
    MOCK_METHOD2(LoadFITSFile, bool(const wxString& filename, std::vector<unsigned short>& data));
    MOCK_METHOD4(SaveFITSFile, bool(const wxString& filename, const std::vector<unsigned short>& data, 
                                   int width, int height));
    MOCK_METHOD3(LoadFITSHeader, bool(const wxString& filename, std::map<wxString, wxString>& header));
    MOCK_METHOD3(SaveFITSHeader, bool(const wxString& filename, const std::map<wxString, wxString>& header));
    
    // Image information
    MOCK_METHOD1(GetImageDimensions, wxSize(const wxString& filename));
    MOCK_METHOD1(GetBitsPerPixel, int(const wxString& filename));
    MOCK_METHOD1(GetImageType, wxString(const wxString& filename));
    
    // Error handling
    MOCK_METHOD0(GetLastError, wxString());
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    
    static MockFITSOperations* instance;
    static MockFITSOperations* GetInstance();
    static void SetInstance(MockFITSOperations* inst);
};

// Image data simulator for comprehensive testing
class ImageDataSimulator {
public:
    struct StarInfo {
        double x, y;           // Position
        double amplitude;      // Peak intensity
        double sigma;          // Gaussian width
        double background;     // Local background
        bool isSaturated;      // Saturation flag
        
        StarInfo(double x = 0, double y = 0, double amp = 1000, double sig = 2.0, double bg = 100, bool sat = false)
            : x(x), y(y), amplitude(amp), sigma(sig), background(bg), isSaturated(sat) {}
    };
    
    struct NoiseInfo {
        double readNoise;      // Read noise (electrons)
        double darkCurrent;    // Dark current (electrons/sec)
        double gain;           // Camera gain (electrons/ADU)
        double bias;           // Bias level (ADU)
        double temperature;    // Sensor temperature (C)
        
        NoiseInfo() : readNoise(5.0), darkCurrent(0.1), gain(1.0), bias(100.0), temperature(-10.0) {}
    };
    
    struct ImageInfo {
        int width, height;
        unsigned short minADU, maxADU;
        double exposureTime;   // seconds
        wxDateTime timestamp;
        wxString filter;
        
        ImageInfo() : width(1024), height(768), minADU(0), maxADU(65535), 
                     exposureTime(1.0), filter("Clear") {
            timestamp = wxDateTime::Now();
        }
    };
    
    // Image generation
    std::vector<unsigned short> CreateSyntheticImage(const ImageInfo& info, 
                                                    const std::vector<StarInfo>& stars,
                                                    const NoiseInfo& noise);
    std::vector<unsigned short> CreateFlatField(const ImageInfo& info, double uniformity = 0.95);
    std::vector<unsigned short> CreateDarkFrame(const ImageInfo& info, const NoiseInfo& noise);
    std::vector<unsigned short> CreateBiasFrame(const ImageInfo& info, const NoiseInfo& noise);
    
    // Star pattern generation
    void AddGaussianStar(std::vector<unsigned short>& image, int width, int height, const StarInfo& star);
    void AddMoffatStar(std::vector<unsigned short>& image, int width, int height, const StarInfo& star, 
                      double alpha = 2.0, double beta = 2.5);
    void AddSaturatedStar(std::vector<unsigned short>& image, int width, int height, const StarInfo& star);
    
    // Noise simulation
    void AddGaussianNoise(std::vector<unsigned short>& image, double sigma);
    void AddPoissonNoise(std::vector<unsigned short>& image, double gain);
    void AddReadoutNoise(std::vector<unsigned short>& image, const NoiseInfo& noise);
    void AddDarkCurrent(std::vector<unsigned short>& image, const ImageInfo& info, const NoiseInfo& noise);
    
    // Defect simulation
    void AddHotPixels(std::vector<unsigned short>& image, int width, int height, int count, double intensity);
    void AddColdPixels(std::vector<unsigned short>& image, int width, int height, int count, double intensity);
    void AddCosmicRays(std::vector<unsigned short>& image, int width, int height, int count);
    void AddDefectMap(std::vector<unsigned short>& image, int width, int height, 
                     const std::vector<wxPoint>& defects, double value);
    
    // Image transformations
    void ApplyGain(std::vector<unsigned short>& image, double gain);
    void ApplyOffset(std::vector<unsigned short>& image, int offset);
    void ApplyGamma(std::vector<unsigned short>& image, double gamma);
    void ClampValues(std::vector<unsigned short>& image, unsigned short minVal, unsigned short maxVal);
    
    // Image analysis
    double CalculateMean(const std::vector<unsigned short>& image);
    double CalculateStdDev(const std::vector<unsigned short>& image);
    unsigned short CalculateMedian(const std::vector<unsigned short>& image);
    unsigned short CalculateMAD(const std::vector<unsigned short>& image);
    std::pair<unsigned short, unsigned short> FindMinMax(const std::vector<unsigned short>& image);
    
    // Star detection simulation
    std::vector<StarInfo> DetectStars(const std::vector<unsigned short>& image, int width, int height,
                                     double threshold = 3.0, int minStars = 1, int maxStars = 100);
    StarInfo MeasureStar(const std::vector<unsigned short>& image, int width, int height,
                        double x, double y, int radius = 10);
    
    // Utility methods
    void SetRandomSeed(unsigned int seed);
    bool ValidateImageSize(int width, int height);
    void Reset();
    void SetupDefaultParameters();
    
    // Test data generation
    std::vector<StarInfo> GenerateRandomStars(int count, int width, int height, 
                                             double minAmplitude = 500, double maxAmplitude = 5000);
    std::vector<wxPoint> GenerateRandomDefects(int count, int width, int height);
    ImageInfo CreateTestImageInfo(int width = 1024, int height = 768);
    NoiseInfo CreateTestNoiseInfo(double readNoise = 5.0, double darkCurrent = 0.1);
    
private:
    std::mt19937 randomGenerator;
    std::uniform_real_distribution<double> uniformDist;
    std::normal_distribution<double> normalDist;
    
    void InitializeRandom();
    double GaussianProfile(double x, double y, double centerX, double centerY, double sigma);
    double MoffatProfile(double x, double y, double centerX, double centerY, double alpha, double beta);
};

// Helper class to manage all image data mocks
class MockImageDataManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
    static MockImageDataGenerator* GetMockGenerator();
    static MockFITSOperations* GetMockFITS();
    static ImageDataSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static std::vector<unsigned short> CreateTestImage(int width = 1024, int height = 768);
    static std::vector<unsigned short> CreateImageWithStar(int width, int height, double x, double y);
    static std::vector<unsigned short> CreateNoisyImage(int width, int height, double noiseLevel);
    static void SetupImageGeneration();
    static void SetupFITSOperations();
    
private:
    static MockImageDataGenerator* mockGenerator;
    static MockFITSOperations* mockFITS;
    static std::unique_ptr<ImageDataSimulator> simulator;
};

// Macros for easier mock setup in tests
#define SETUP_IMAGE_DATA_MOCKS() MockImageDataManager::SetupMocks()
#define TEARDOWN_IMAGE_DATA_MOCKS() MockImageDataManager::TeardownMocks()
#define RESET_IMAGE_DATA_MOCKS() MockImageDataManager::ResetMocks()

#define GET_MOCK_IMAGE_GENERATOR() MockImageDataManager::GetMockGenerator()
#define GET_MOCK_FITS_OPERATIONS() MockImageDataManager::GetMockFITS()
#define GET_IMAGE_SIMULATOR() MockImageDataManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_IMAGE_CREATE_SUCCESS(width, height) \
    EXPECT_CALL(*GET_MOCK_IMAGE_GENERATOR(), CreateImage(width, height)) \
        .WillOnce(::testing::Return(std::vector<unsigned short>(width * height, 100)))

#define EXPECT_STAR_CREATE_SUCCESS(size, amplitude) \
    EXPECT_CALL(*GET_MOCK_IMAGE_GENERATOR(), CreateGaussianStar(size, amplitude, ::testing::_, ::testing::_)) \
        .WillOnce(::testing::Return(std::vector<unsigned short>(size * size, amplitude)))

#define EXPECT_FITS_LOAD_SUCCESS(filename) \
    EXPECT_CALL(*GET_MOCK_FITS_OPERATIONS(), LoadFITSFile(filename, ::testing::_)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_FITS_SAVE_SUCCESS(filename) \
    EXPECT_CALL(*GET_MOCK_FITS_OPERATIONS(), SaveFITSFile(filename, ::testing::_, ::testing::_, ::testing::_)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_IMAGE_STATS_CALCULATION(image, mean, stddev) \
    EXPECT_CALL(*GET_MOCK_IMAGE_GENERATOR(), CalculateMean(image)) \
        .WillOnce(::testing::Return(mean)); \
    EXPECT_CALL(*GET_MOCK_IMAGE_GENERATOR(), CalculateStdDev(image)) \
        .WillOnce(::testing::Return(stddev))

#endif // MOCK_IMAGE_DATA_H
