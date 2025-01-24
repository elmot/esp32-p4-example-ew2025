#include <stdio.h>
#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
//#include "esp_lcd_panel_ops.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "jpeg_decoder.h"
#include "usb/usb_host.h"
#include "usb/uvc_host.h"

#include "camera.h"
//#define FRAME_FORMAT UVC_VS_FORMAT_MJPEG
#define FRAME_FORMAT UVC_VS_FORMAT_YUY2
#define FRAME_FPS    30
#define DECODE_EVERY_XTH_FRAME 2        // Every 2nd frame will be decoded and send to display. This save CPU time
#define DECODE_WORKING_BUFFER_SIZE 24000 // We must increase JPEG decoder working buffer size

#if CONFIG_SPIRAM
#define NUMBER_OF_FRAME_BUFFERS 5 // Number of frames from the camera
#else
#define NUMBER_OF_FRAME_BUFFERS 2 // Number of frames from the camera
#endif

volatile uint16_t *cam_buffer = NULL; // Framebuffer for decoded data (to LCD)
static const char *TAG = "UVC_CAM";
static QueueHandle_t frame_q = NULL; // Queue of received frames that are passed to processing task
static SemaphoreHandle_t device_disconnected_sem;
static uvc_host_stream_hdl_t stream;

void yuy2_to_rgb565(const uint8_t *yuy2, uint16_t *rgb565, unsigned int width,unsigned  int height);

void stream_callback(const uvc_host_stream_event_data_t *event, void *user_ctx)
{
    (void) user_ctx;
    switch (event->type) {
        case UVC_HOST_TRANSFER_ERROR:
            ESP_LOGE(TAG, "USB error");
            break;
        case UVC_HOST_DEVICE_DISCONNECTED:
            ESP_LOGW(TAG, "Device disconnected");
            ESP_ERROR_CHECK(uvc_host_stream_close(event->device_disconnected.stream_hdl));
            xSemaphoreGive(device_disconnected_sem);
            break;
        case UVC_HOST_FRAME_BUFFER_OVERFLOW:
            ESP_LOGW(TAG, "Frame buffer overflow");
            break;
        case UVC_HOST_FRAME_BUFFER_UNDERFLOW:
            ESP_LOGW(TAG, "Frame buffer underflow");
            break;
        default:
            abort();
    }
}

bool frame_callback(const uvc_host_frame_t *frame, void *user_ctx)
{
    (void) user_ctx;
    bool frame_processed = false; // If we return false from this callback, we must return the frame with uvc_host_frame_return(stream, frame);

    switch (frame->vs_format.format) {
        case UVC_VS_FORMAT_YUY2: {
            ESP_LOGD(TAG, "YUY2 frame %dx%d", frame->vs_format.h_res, frame->vs_format.v_res);
            if (cam_buffer) {
                yuy2_to_rgb565(frame->data, (uint16_t*)cam_buffer, frame->vs_format.h_res, frame->vs_format.v_res);
//                esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, (int)frame->vs_format.h_res, (int)frame->vs_format.v_res, (const void *)fb);
            }
            frame_processed = true;
            break;
        }
        case UVC_VS_FORMAT_H264:
        case UVC_VS_FORMAT_H265:
        case UVC_VS_FORMAT_MJPEG: {
            // Attempt to put the new frame into the queue without checking if it is full.
            // If successful, we save processing time since we avoid the overhead of a full check for every frame.
            BaseType_t frame_put_to_queue = xQueueSendToBack(frame_q, &frame, 0);

            // Adding the frame to our queue should never fail, because our frame_q has the same length as the UVC driver queue (NUMBER_OF_FRAME_BUFFERS)
            assert(frame_put_to_queue == pdPASS);
            break;
        }
        default:
            ESP_LOGI(TAG, "Unsupported format!");
            break;
    }
    return frame_processed;
}

_Noreturn static void processing_task(void *pvParameters)
{
    (void) pvParameters;
    uvc_host_frame_t *frame;
    uint8_t *jpeg_working_buffer = malloc(DECODE_WORKING_BUFFER_SIZE);
    assert(jpeg_working_buffer);

    while (1) {
        xQueueReceive(frame_q, &frame, portMAX_DELAY);
        ESP_LOGD(TAG, "MJPEG frame %dx%d %d bytes", frame->vs_format.h_res, frame->vs_format.v_res, frame->data_len);

        static int frame_i = 0;
        if (cam_buffer && ((frame_i % DECODE_EVERY_XTH_FRAME) == 0)) {
            frame_i = 0;
            esp_jpeg_image_cfg_t jpeg_cfg = {
                    .indata = (uint8_t *)frame->data,
                    .indata_size = frame->data_len,
                    .outbuf = (uint8_t *)cam_buffer,
                    .outbuf_size = FRAME_H_RES * FRAME_V_RES * 2,
                    .out_format = JPEG_IMAGE_FORMAT_RGB565,
                    .out_scale = JPEG_IMAGE_SCALE_0,
                    .flags = {
                            .swap_color_bytes = 0,
                    },
                    .advanced = {
                            .working_buffer = jpeg_working_buffer,
                            .working_buffer_size = DECODE_WORKING_BUFFER_SIZE,
                    },
            };
            esp_jpeg_image_output_t outimg;

            if (ESP_OK == esp_jpeg_decode(&jpeg_cfg, &outimg)) {
//                esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, outimg.width, outimg.height, (const void *)fb);
            } else {
                ESP_LOGW(TAG, "Decoding failed");
            }
        } else {
            ESP_LOGI(TAG, "Skipping decoding of received MJPEG frame");
        }
        frame_i++;

        uvc_host_frame_return(stream, frame);
    }

    // This code should never be reached. Leaving it here for completeness
    //free(jpeg_working_buffer);
    //vTaskDelete(NULL);
}

_Noreturn static void usb_lib_task(void *arg)
{
    (void) arg;
    while (1) {
        // Start handling system events
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            usb_host_device_free_all();
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            ESP_LOGI(TAG, "USB: All devices freed");
            // Continue handling USB events to allow device reconnection
        }
    }
}

void init_camera(void)
{

    //@todo does not work in PSRAM... :'(
    cam_buffer = heap_caps_aligned_alloc(64, FRAME_H_RES * FRAME_V_RES * 2, MALLOC_CAP_INTERNAL);
    if (cam_buffer == NULL) {
        ESP_LOGW(TAG, "Insufficient memory for LCD frame buffer. LCD output disabled.");
    }

    frame_q = xQueueCreate(NUMBER_OF_FRAME_BUFFERS, sizeof (uvc_host_frame_t *));
    device_disconnected_sem = xSemaphoreCreateBinary();
    assert(device_disconnected_sem && frame_q);

    // Install USB Host driver. Should only be called once in entire application
    ESP_LOGI(TAG, "Installing USB Host");
    const usb_host_config_t host_config = {
            .skip_phy_setup = false,
            .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    ESP_ERROR_CHECK(usb_host_install(&host_config));

    // Create a task that will handle USB library events
    if (xTaskCreate(usb_lib_task, "usb_lib", 4096, NULL, 10, NULL) != pdTRUE) {
        ESP_LOGE(TAG, "Creating USB host lib task failed");
        abort();
    }

    // UVC driver install
    const uvc_host_driver_config_t uvc_driver_config = {
            .driver_task_stack_size = 6 * 1024,
            .driver_task_priority = 6,
            .xCoreID = tskNO_AFFINITY,
            .create_background_task = true,
    };
    ESP_ERROR_CHECK(uvc_host_install(&uvc_driver_config));

    // UVC Stream init
    const uvc_host_stream_config_t uvc_stream_config = {
            .event_cb = stream_callback,
            .frame_cb = frame_callback,
            .user_ctx = NULL,
            .usb = {
                    .vid = 0,
                    .pid = 0,
                    .uvc_stream_index = 0,
            },
            .vs_format = {
                    .h_res = FRAME_H_RES,
                    .v_res = FRAME_V_RES,
                    .fps = FRAME_FPS,
                    .format = FRAME_FORMAT,
            },
            .advanced = {
                    .number_of_frame_buffers = NUMBER_OF_FRAME_BUFFERS,
                    .frame_size = 2 * FRAME_H_RES * FRAME_V_RES,
#if CONFIG_SPIRAM
                    .frame_heap_caps = MALLOC_CAP_SPIRAM,
#else
                    .frame_heap_caps = 0,
#endif
                    .number_of_urbs = 3,
                    .urb_size = 4 * 1024,
            },
    };

    BaseType_t ret = xTaskCreate(processing_task, "frame_process", 4 * 1024, NULL, 2, NULL);
    assert(ret == pdPASS);

    while (true) {
        ESP_LOGI(TAG, "Opening the stream...");
        esp_err_t err = uvc_host_stream_open(&uvc_stream_config, pdMS_TO_TICKS(5000), &stream);
        if (ESP_OK != err) {
            ESP_LOGI(TAG, "Failed to open device");
            continue;
        }

        ESP_ERROR_CHECK(uvc_host_stream_start(stream));
        xSemaphoreTake(device_disconnected_sem, portMAX_DELAY);
    }
}