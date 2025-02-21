#ifndef ESP32_P4_EXAMPLE_EW2025_CAMERA_H
#define ESP32_P4_EXAMPLE_EW2025_CAMERA_H

#include <stdint.h>
extern uint16_t *cam_buffer; // Framebuffer for decoded data (to LCD)
#define FRAME_H_RES  320
#define FRAME_V_RES  240

#ifdef __cplusplus
extern "C" {
#endif

void init_camera(void);

void init_image_processing(void);

void image_processing_update_pixels(uint16_t * pixelsA, uint16_t * pixelsB, uint16_t * pixelsC);

bool is_image_processing_done(void);
void image_processing_start(void * cam_buffer);

extern volatile bool finger_detected;

#ifdef __cplusplus
}
#endif

#endif //ESP32_P4_EXAMPLE_EW2025_CAMERA_H
