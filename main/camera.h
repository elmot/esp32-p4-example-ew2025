
#ifndef ESP32_P4_EXAMPLE_EW2025_CAMERA_H
#define ESP32_P4_EXAMPLE_EW2025_CAMERA_H

extern volatile uint16_t *cam_buffer; // Framebuffer for decoded data (to LCD)
#define FRAME_H_RES  320
#define FRAME_V_RES  240

void init_camera(void);

#endif //ESP32_P4_EXAMPLE_EW2025_CAMERA_H
