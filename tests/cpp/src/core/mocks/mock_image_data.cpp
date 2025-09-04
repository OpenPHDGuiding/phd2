/*
 * mock_image_data.cpp
 * PHD Guiding - Core Module Tests
 *
 * Implementation of mock image data objects
 */

#include "mock_image_data.h"
#include <algorithm>
#include <numeric>
#include <cmath>

// Static instance declarations
MockImageDataGenerator* MockImageDataGenerator::instance = nullptr;
MockFITSOperations* MockFITSOperations::instance = nullptr;

// MockImageDataManager static members
MockImageDataGenerator* MockImageDataManager::mockGenerator = nullptr;
MockFITSOperations* MockImageDataManager::mockFITS = nullptr;
std::unique_ptr<ImageDataSimulator> MockImageDataManager::simulator = nullptr;

// MockImageDataGenerator implementation
MockImageDataGenerator* MockImageDataGenerator::GetInstance() {
    return instance;
}

void MockImageDataGenerator::SetInstance(MockImageDataGenerator* inst) {
    instance = inst;
}

// MockFITSOperations implementation
MockFITSOperations* MockFITSOperations::GetInstance() {
    return instance;
}

void MockFITSOperations::SetInstance(MockFITSOperations* inst) {
    instance = inst;
}

// ImageDataSimulator implementation
std::vector<unsigned short> ImageDataSimulator::CreateSyntheticImage(const ImageInfo& info, 
                                                                     const std::vector<StarInfo>& stars,
                                                                     const NoiseInfo& noise) {
    std::vector<unsigned short> image(info.width * info.height, static_cast<unsigned short>(noise.bias));
    
    // Add stars
    for (const auto& star : stars) {
        if (star.isSaturated) {
            AddSaturatedStar(image, info.width, info.height, star);
        } else {
            AddGaussianStar(image, info.width, info.height, star);
        }
    }
    
    // Add noise
    AddReadoutNoise(image, noise);
    AddDarkCurrent(image, info, noise);
    AddPoissonNoise(image, noise.gain);
    
    // Clamp values
    ClampValues(image, info.minADU, info.maxADU);
    
    return image;
}

std::vector<unsigned short> ImageDataSimulator::CreateFlatField(const ImageInfo& info, double uniformity) {
    std::vector<unsigned short> image(info.width * info.height);
    
    // Create a flat field with some non-uniformity
    double centerX = info.width / 2.0;
    double centerY = info.height / 2.0;
    double maxRadius = std::sqrt(centerX * centerX + centerY * centerY);
    
    for (int y = 0; y < info.height; ++y) {
        for (int x = 0; x < info.width; ++x) {
            double dx = x - centerX;
            double dy = y - centerY;
            double radius = std::sqrt(dx * dx + dy * dy);
            double vignetting = 1.0 - (1.0 - uniformity) * (radius / maxRadius);
            
            unsigned short value = static_cast<unsigned short>(30000 * vignetting);
            image[y * info.width + x] = std::max(static_cast<unsigned short>(1000), value);
        }
    }
    
    return image;
}

std::vector<unsigned short> ImageDataSimulator::CreateDarkFrame(const ImageInfo& info, const NoiseInfo& noise) {
    std::vector<unsigned short> image(info.width * info.height, static_cast<unsigned short>(noise.bias));
    
    // Add dark current
    AddDarkCurrent(image, info, noise);
    
    // Add read noise
    AddReadoutNoise(image, noise);
    
    // Add some hot pixels
    AddHotPixels(image, info.width, info.height, 10, 1000);
    
    return image;
}

std::vector<unsigned short> ImageDataSimulator::CreateBiasFrame(const ImageInfo& info, const NoiseInfo& noise) {
    std::vector<unsigned short> image(info.width * info.height, static_cast<unsigned short>(noise.bias));
    
    // Add read noise only
    AddReadoutNoise(image, noise);
    
    return image;
}

void ImageDataSimulator::AddGaussianStar(std::vector<unsigned short>& image, int width, int height, const StarInfo& star) {
    int radius = static_cast<int>(star.sigma * 5); // 5-sigma radius
    int startX = std::max(0, static_cast<int>(star.x) - radius);
    int endX = std::min(width - 1, static_cast<int>(star.x) + radius);
    int startY = std::max(0, static_cast<int>(star.y) - radius);
    int endY = std::min(height - 1, static_cast<int>(star.y) + radius);
    
    for (int y = startY; y <= endY; ++y) {
        for (int x = startX; x <= endX; ++x) {
            double profile = GaussianProfile(x, y, star.x, star.y, star.sigma);
            double value = star.background + star.amplitude * profile;
            
            int index = y * width + x;
            image[index] = static_cast<unsigned short>(std::min(65535.0, 
                                                               static_cast<double>(image[index]) + value));
        }
    }
}

void ImageDataSimulator::AddMoffatStar(std::vector<unsigned short>& image, int width, int height, 
                                      const StarInfo& star, double alpha, double beta) {
    int radius = static_cast<int>(alpha * 10); // Extended radius for Moffat profile
    int startX = std::max(0, static_cast<int>(star.x) - radius);
    int endX = std::min(width - 1, static_cast<int>(star.x) + radius);
    int startY = std::max(0, static_cast<int>(star.y) - radius);
    int endY = std::min(height - 1, static_cast<int>(star.y) + radius);
    
    for (int y = startY; y <= endY; ++y) {
        for (int x = startX; x <= endX; ++x) {
            double profile = MoffatProfile(x, y, star.x, star.y, alpha, beta);
            double value = star.background + star.amplitude * profile;
            
            int index = y * width + x;
            image[index] = static_cast<unsigned short>(std::min(65535.0, 
                                                               static_cast<double>(image[index]) + value));
        }
    }
}

void ImageDataSimulator::AddSaturatedStar(std::vector<unsigned short>& image, int width, int height, const StarInfo& star) {
    int radius = static_cast<int>(star.sigma * 3); // Smaller radius for saturated core
    int startX = std::max(0, static_cast<int>(star.x) - radius);
    int endX = std::min(width - 1, static_cast<int>(star.x) + radius);
    int startY = std::max(0, static_cast<int>(star.y) - radius);
    int endY = std::min(height - 1, static_cast<int>(star.y) + radius);
    
    for (int y = startY; y <= endY; ++y) {
        for (int x = startX; x <= endX; ++x) {
            double dx = x - star.x;
            double dy = y - star.y;
            double distance = std::sqrt(dx * dx + dy * dy);
            
            double value;
            if (distance < star.sigma) {
                value = 65535; // Saturated core
            } else {
                double profile = GaussianProfile(x, y, star.x, star.y, star.sigma);
                value = star.background + star.amplitude * profile;
            }
            
            int index = y * width + x;
            image[index] = static_cast<unsigned short>(std::min(65535.0, value));
        }
    }
}

void ImageDataSimulator::AddGaussianNoise(std::vector<unsigned short>& image, double sigma) {
    std::normal_distribution<double> noise(0.0, sigma);
    
    for (auto& pixel : image) {
        double noiseValue = noise(randomGenerator);
        double newValue = static_cast<double>(pixel) + noiseValue;
        pixel = static_cast<unsigned short>(std::max(0.0, std::min(65535.0, newValue)));
    }
}

void ImageDataSimulator::AddPoissonNoise(std::vector<unsigned short>& image, double gain) {
    for (auto& pixel : image) {
        if (pixel > 0) {
            double electrons = static_cast<double>(pixel) * gain;
            std::poisson_distribution<int> poisson(electrons);
            int noisyElectrons = poisson(randomGenerator);
            double noisyADU = static_cast<double>(noisyElectrons) / gain;
            pixel = static_cast<unsigned short>(std::max(0.0, std::min(65535.0, noisyADU)));
        }
    }
}

void ImageDataSimulator::AddReadoutNoise(std::vector<unsigned short>& image, const NoiseInfo& noise) {
    AddGaussianNoise(image, noise.readNoise / noise.gain);
}

void ImageDataSimulator::AddDarkCurrent(std::vector<unsigned short>& image, const ImageInfo& info, const NoiseInfo& noise) {
    // Temperature-dependent dark current
    double tempFactor = std::pow(2.0, (noise.temperature + 25.0) / 6.0); // Doubling every 6°C
    double darkElectrons = noise.darkCurrent * info.exposureTime * tempFactor;
    double darkADU = darkElectrons / noise.gain;
    
    for (auto& pixel : image) {
        double newValue = static_cast<double>(pixel) + darkADU;
        pixel = static_cast<unsigned short>(std::max(0.0, std::min(65535.0, newValue)));
    }
}

void ImageDataSimulator::AddHotPixels(std::vector<unsigned short>& image, int width, int height, int count, double intensity) {
    std::uniform_int_distribution<int> xDist(0, width - 1);
    std::uniform_int_distribution<int> yDist(0, height - 1);
    
    for (int i = 0; i < count; ++i) {
        int x = xDist(randomGenerator);
        int y = yDist(randomGenerator);
        int index = y * width + x;
        
        double newValue = static_cast<double>(image[index]) + intensity;
        image[index] = static_cast<unsigned short>(std::min(65535.0, newValue));
    }
}

void ImageDataSimulator::AddColdPixels(std::vector<unsigned short>& image, int width, int height, int count, double intensity) {
    std::uniform_int_distribution<int> xDist(0, width - 1);
    std::uniform_int_distribution<int> yDist(0, height - 1);
    
    for (int i = 0; i < count; ++i) {
        int x = xDist(randomGenerator);
        int y = yDist(randomGenerator);
        int index = y * width + x;
        
        double newValue = static_cast<double>(image[index]) - intensity;
        image[index] = static_cast<unsigned short>(std::max(0.0, newValue));
    }
}

void ImageDataSimulator::AddCosmicRays(std::vector<unsigned short>& image, int width, int height, int count) {
    std::uniform_int_distribution<int> xDist(1, width - 2);
    std::uniform_int_distribution<int> yDist(1, height - 2);
    std::uniform_real_distribution<double> intensityDist(5000, 50000);
    
    for (int i = 0; i < count; ++i) {
        int x = xDist(randomGenerator);
        int y = yDist(randomGenerator);
        double intensity = intensityDist(randomGenerator);
        
        // Create a small cosmic ray pattern
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                int index = (y + dy) * width + (x + dx);
                double factor = (dx == 0 && dy == 0) ? 1.0 : 0.3;
                double newValue = static_cast<double>(image[index]) + intensity * factor;
                image[index] = static_cast<unsigned short>(std::min(65535.0, newValue));
            }
        }
    }
}

double ImageDataSimulator::CalculateMean(const std::vector<unsigned short>& image) {
    if (image.empty()) return 0.0;
    
    double sum = std::accumulate(image.begin(), image.end(), 0.0);
    return sum / static_cast<double>(image.size());
}

double ImageDataSimulator::CalculateStdDev(const std::vector<unsigned short>& image) {
    if (image.size() < 2) return 0.0;
    
    double mean = CalculateMean(image);
    double sumSquares = 0.0;
    
    for (const auto& pixel : image) {
        double diff = static_cast<double>(pixel) - mean;
        sumSquares += diff * diff;
    }
    
    return std::sqrt(sumSquares / static_cast<double>(image.size() - 1));
}

unsigned short ImageDataSimulator::CalculateMedian(const std::vector<unsigned short>& image) {
    if (image.empty()) return 0;
    
    std::vector<unsigned short> sorted = image;
    std::sort(sorted.begin(), sorted.end());
    
    size_t size = sorted.size();
    if (size % 2 == 0) {
        return static_cast<unsigned short>((sorted[size/2 - 1] + sorted[size/2]) / 2);
    } else {
        return sorted[size/2];
    }
}

void ImageDataSimulator::SetRandomSeed(unsigned int seed) {
    randomGenerator.seed(seed);
}

bool ImageDataSimulator::ValidateImageSize(int width, int height) {
    return width > 0 && height > 0 && width <= 65536 && height <= 65536;
}

void ImageDataSimulator::Reset() {
    InitializeRandom();
    SetupDefaultParameters();
}

void ImageDataSimulator::SetupDefaultParameters() {
    // Initialize with default random seed
    SetRandomSeed(12345);
}

double ImageDataSimulator::GaussianProfile(double x, double y, double centerX, double centerY, double sigma) {
    double dx = x - centerX;
    double dy = y - centerY;
    double r2 = dx * dx + dy * dy;
    return std::exp(-r2 / (2.0 * sigma * sigma));
}

double ImageDataSimulator::MoffatProfile(double x, double y, double centerX, double centerY, double alpha, double beta) {
    double dx = x - centerX;
    double dy = y - centerY;
    double r2 = dx * dx + dy * dy;
    return std::pow(1.0 + r2 / (alpha * alpha), -beta);
}

void ImageDataSimulator::InitializeRandom() {
    randomGenerator.seed(std::random_device{}());
    uniformDist = std::uniform_real_distribution<double>(0.0, 1.0);
    normalDist = std::normal_distribution<double>(0.0, 1.0);
}

// MockImageDataManager implementation
void MockImageDataManager::SetupMocks() {
    // Create all mock instances
    mockGenerator = new MockImageDataGenerator();
    mockFITS = new MockFITSOperations();
    
    // Set static instances
    MockImageDataGenerator::SetInstance(mockGenerator);
    MockFITSOperations::SetInstance(mockFITS);
    
    // Create simulator
    simulator = std::make_unique<ImageDataSimulator>();
    simulator->SetupDefaultParameters();
}

void MockImageDataManager::TeardownMocks() {
    // Clean up all mock instances
    delete mockGenerator;
    delete mockFITS;
    
    // Reset pointers
    mockGenerator = nullptr;
    mockFITS = nullptr;
    
    // Reset static instances
    MockImageDataGenerator::SetInstance(nullptr);
    MockFITSOperations::SetInstance(nullptr);
    
    // Clean up simulator
    simulator.reset();
}

void MockImageDataManager::ResetMocks() {
    if (mockGenerator) {
        testing::Mock::VerifyAndClearExpectations(mockGenerator);
    }
    if (mockFITS) {
        testing::Mock::VerifyAndClearExpectations(mockFITS);
    }
    
    if (simulator) {
        simulator->Reset();
    }
}

// Getter methods
MockImageDataGenerator* MockImageDataManager::GetMockGenerator() { return mockGenerator; }
MockFITSOperations* MockImageDataManager::GetMockFITS() { return mockFITS; }
ImageDataSimulator* MockImageDataManager::GetSimulator() { return simulator.get(); }

std::vector<unsigned short> MockImageDataManager::CreateTestImage(int width, int height) {
    if (simulator) {
        ImageDataSimulator::ImageInfo info;
        info.width = width;
        info.height = height;
        
        ImageDataSimulator::NoiseInfo noise;
        
        return simulator->CreateBiasFrame(info, noise);
    }
    return std::vector<unsigned short>(width * height, 100);
}

std::vector<unsigned short> MockImageDataManager::CreateImageWithStar(int width, int height, double x, double y) {
    if (simulator) {
        ImageDataSimulator::ImageInfo info;
        info.width = width;
        info.height = height;
        
        std::vector<ImageDataSimulator::StarInfo> stars;
        stars.emplace_back(x, y, 5000, 2.5, 100, false);
        
        ImageDataSimulator::NoiseInfo noise;
        
        return simulator->CreateSyntheticImage(info, stars, noise);
    }
    return std::vector<unsigned short>(width * height, 100);
}

void MockImageDataManager::SetupImageGeneration() {
    if (mockGenerator) {
        EXPECT_CALL(*mockGenerator, ValidateImageSize(::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockGenerator, SetRandomSeed(::testing::_))
            .WillRepeatedly(::testing::Return());
    }
}

void MockImageDataManager::SetupFITSOperations() {
    if (mockFITS) {
        EXPECT_CALL(*mockFITS, GetLastError())
            .WillRepeatedly(::testing::Return(wxString("")));
    }
}
