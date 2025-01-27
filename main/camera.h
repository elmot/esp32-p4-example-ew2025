#ifndef ESP32_P4_EXAMPLE_EW2025_CAMERA_H
#define ESP32_P4_EXAMPLE_EW2025_CAMERA_H

#include <stdint.h>
extern volatile uint16_t *cam_buffer; // Framebuffer for decoded data (to LCD)
#define FRAME_H_RES  320
#define FRAME_V_RES  240

#ifdef __cplusplus
extern "C" {
#endif

void init_camera(void);

void init_image_processing();

void image_processing(const uint16_t * const  srcRgb565, uint16_t  * outFrameA, uint16_t  * outFrameB, uint16_t  * outFrameC);

#ifdef __cplusplus
}
#endif

#endif //ESP32_P4_EXAMPLE_EW2025_CAMERA_H
