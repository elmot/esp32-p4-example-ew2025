#include "camera.h"

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/photo.hpp>
#include <pthread.h>
#include <semaphore.h>


using namespace cv;

static CascadeClassifier faceCascade;

sem_t processing_ready;
[[noreturn]] static void* image_processing(void* args);

void init_image_processing()
{
    assert(faceCascade.load("/assets/lbpcascade_frontalface.xml"));
    sem_init(&processing_ready, 0, 0);
    pthread_t threadHandle;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 12000); // Set the stack for the thread
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); // Set the stack size for the thread
    assert(!pthread_create(&threadHandle, &attr, &image_processing, NULL));
    pthread_setname_np(threadHandle, "slow-image-processing");
}


static volatile bool processing_finished = true;

bool is_image_processing_done() {
    return processing_finished;
}

static Mat grayFrame;
static Mat invertFrame;


[[noreturn]] static void* image_processing(void* args)
{
    while (true)
    {
        sem_wait(&processing_ready);
        Mat scaledFrame;
        resize(grayFrame, scaledFrame, Size{FRAME_H_RES / 2, FRAME_V_RES / 2});
        equalizeHist(scaledFrame, scaledFrame);

        std::vector<Rect> faces;
        faceCascade.detectMultiScale(scaledFrame, faces, 1.15, 3, 0, Size(30, 30));

        for (const auto& face : faces)
        {
            Rect dFace(face.x * 2, face.y * 2, face.width * 2, face.height * 2);
            rectangle(grayFrame, dFace, Scalar(255), 2);
        }
        processing_finished = true;
    }
}

void image_processing_update_pixels(uint16_t* pixelsA, uint16_t* pixelsB, uint16_t* pixelsC)
{
    if (grayFrame.empty()) return;
    Mat result565A(FRAME_V_RES, FRAME_H_RES, CV_8UC2, (void*)pixelsA);

    Mat result565B(FRAME_V_RES, FRAME_H_RES, CV_8UC2, (void*)pixelsB);
    cvtColor(grayFrame, result565B, COLOR_GRAY2BGR565);

    bitwise_not(grayFrame, grayFrame);
    cvtColor(grayFrame, result565A, COLOR_GRAY2BGR565);
}

void image_processing_start(void* cam_buffer)
{
    processing_finished = false;
    Mat srcImage(FRAME_V_RES, FRAME_H_RES, CV_8UC2, (void*)cam_buffer);
    cvtColor(srcImage, grayFrame, COLOR_BGR5652GRAY);
    sem_post(&processing_ready);
}


/*
static void image_processing() {
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
*/
