#include "camera.h"

#include <opencv2/opencv.hpp>

// Function to convert RGB565 to BGR888 (used by OpenCV)
cv::Mat convertRGB565ToBGR(const uint16_t* rgb565Data, int width, int height) {
    cv::Mat bgrImage(height, width, CV_8UC3); // Create a BGR image

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint16_t rgb565 = rgb565Data[y * width + x];

            // Extract RGB components from RGB565
            uint8_t r = ((rgb565 >> 11) & 0x1F) * 255 / 31;
            uint8_t g = ((rgb565 >> 5) & 0x3F) * 255 / 63;
            uint8_t b = (rgb565 & 0x1F) * 255 / 31;

            // Set the BGR pixel
            bgrImage.at<cv::Vec3b>(y, x) = cv::Vec3b(b, g, r);
        }
    }

    return bgrImage;
}

void edge_detection(const uint16_t * const  srcRgb565, uint16_t  * dstRgb565) {
    auto bgrImage = convertRGB565ToBGR(srcRgb565,FRAME_H_RES,FRAME_V_RES);
    cv::Mat grayImage;
    cv::cvtColor(bgrImage, grayImage, cv::COLOR_BGR2GRAY);
    cv::Mat edges;
    cv::Canny(grayImage, edges, 100, 200); // Thresholds can be adjusted
}


int example() {
    // Width and height of the image
    const int width = 320; // Example width
    const int height = 240; // Example height

    // Example RGB565 pixel data (dummy data, you need to replace with actual image data)
    uint16_t* rgb565Data = new uint16_t[width * height];
    for (int i = 0; i < width * height; ++i) {
        rgb565Data[i] = static_cast<uint16_t>(rand() % 65536); // Random data
    }

    // Convert RGB565 data to BGR image
    cv::Mat bgrImage = convertRGB565ToBGR(rgb565Data, width, height);

    // Convert BGR to grayscale
    cv::Mat grayImage;
    cv::cvtColor(bgrImage, grayImage, cv::COLOR_BGR2GRAY);

    // Perform Canny edge detection
    cv::Mat edges;
    cv::Canny(grayImage, edges, 100, 200); // Thresholds can be adjusted

    // Display result
//    cv::imshow("Original Image", bgrImage);
//    cv::imshow("Edge Detection", edges);

    // Wait for a key press and clean up
    cv::waitKey(0);

    delete[] rgb565Data; // Free the allocated memory

    return 0;
}