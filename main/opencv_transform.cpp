#include "camera.h"

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>

using namespace cv;

static CascadeClassifier faceCascade;

void init_image_processing() {
    assert(faceCascade.load("/assets/lbpcascade_frontalface.xml")) ;
}

void image_processing(const uint16_t * const  srcRgb565, uint16_t  * outFrameA, uint16_t  * outFrameB, uint16_t  * outFrameC) {
    Mat srcImage(FRAME_V_RES, FRAME_H_RES, CV_8UC2, (void *) srcRgb565);

    Mat result565A(FRAME_V_RES, FRAME_H_RES, CV_8UC2, (void *) outFrameA);
    Mat result565B(FRAME_V_RES, FRAME_H_RES, CV_8UC2, (void *) outFrameB);
    Mat result565C(FRAME_V_RES, FRAME_H_RES, CV_8UC2, (void *) outFrameC);

    Mat grayFrame;
    cvtColor(srcImage, grayFrame, COLOR_BGR5652GRAY);
    cvtColor(grayFrame, result565A, COLOR_GRAY2BGR565);


//    Mat processedB;
//    Canny(grayFrame, processedB, 100, 200); // Thresholds can be adjusted

    std::vector<Rect> faces;
    Mat scaledFrame;
    resize(grayFrame, scaledFrame, Size{FRAME_H_RES / 2, FRAME_V_RES / 2});
    equalizeHist(scaledFrame, scaledFrame);
    faceCascade.detectMultiScale(scaledFrame, faces, 1.15, 3, 0, Size(30, 30));

    // Draw rectangles around detected faces
    for (const auto &face : faces) {
        Rect dFace(face.x * 2, face.y * 2, face.width * 2, face.height * 2);
        rectangle(grayFrame, dFace, Scalar(255), 2);
    }

    cvtColor(grayFrame, result565B, COLOR_GRAY2BGR565);

    Mat processedC;
    bitwise_not(srcImage, result565C);

    cvtColor(grayFrame,result565B, COLOR_GRAY2BGR565);

}
