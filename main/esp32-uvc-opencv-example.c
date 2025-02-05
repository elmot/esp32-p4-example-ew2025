#include <stdio.h>
#include <pthread.h>
#include "SDL3/SDL.h"
#include "SDL3/SDL_esp-idf.h"
//#include "SDL3_image/SDL_image.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "bsp/esp-bsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "graphics.h"
#include "filesystem.h"
#include "text.h"
#include "camera.h"
//#include "esp_heap_trace.h"

void print_psram_info();

Uint32 SDLCALL TimerCallback(void *param, SDL_TimerID timerID, Uint32 interval) {
    (void) param;
    (void) timerID;
    // printf("Timer callback executed!\n");
    return interval; // Return the interval to keep the timer running
}

void *sdl_thread(void *args) {
    (void) args;
    printf("SDL3 on ESP32\n");
    print_psram_info();
    print_psram_info();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == false) {
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
        return NULL;
    }
    printf("SDL initialized successfully\n");

    SDL_Window *window = SDL_CreateWindow("SDL on ESP32", BSP_LCD_H_RES, BSP_LCD_V_RES, 0);
    if (!window) {
        printf("Failed to create window: %s\n", SDL_GetError());
        return NULL;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return NULL;
    }

    clear_screen(renderer);

    SDL_InitFS();
    TestFileOpen("/assets/espressif.bmp");

    TTF_Font *font = initialize_font("/assets/FreeSans.ttf", 12);
    if (!font) return NULL;

    SDL_TimerID timer_id = SDL_AddTimer(1000, TimerCallback, NULL);
    if (timer_id == 0) {
        printf("Failed to create timer: %s\n", SDL_GetError());
    } else {
        printf("Timer created successfully\n");
    }

    SDL_Texture *textTexture = render_text(renderer, font, "Hello ESP32 - SDL3", (SDL_Color) {255, 255, 255, 255});
    SDL_Texture *imageTexture = LoadBackgroundImage(renderer, "/assets/espressif.bmp");
    SDL_Texture *cameraTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
                                                   FRAME_H_RES, FRAME_V_RES);
    SDL_Texture *outputTextureA = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
                                                   FRAME_H_RES, FRAME_V_RES);
    SDL_Texture *outputTextureB = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
                                                   FRAME_H_RES, FRAME_V_RES);
    SDL_Texture *outputTextureC = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
                                                   FRAME_H_RES, FRAME_V_RES);

    float bmp_x = 2.0f, bmp_y = 2.0f;
    float bmp_speed_x = 2.0f, bmp_speed_y = 2.0f;

    float text_x = 30.0f, text_y = 40.0f;
    float text_speed_x = 1.5f, text_speed_y = 1.2f;
    float text_scale = 1.0f, text_scale_speed = 0.01f;
    float text_direction_x = 1, text_direction_y = 1;
    float scale_direction = 1;

    print_psram_info();
    init_image_processing();
    printf("Entering main loop...\n");
//    heap_trace_dump();
    SDL_Event event;
    while (1) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                break;
            } else if (event.type == SDL_EVENT_FINGER_UP) {
                bmp_x = (float) event.tfinger.x;
                bmp_y = (float) event.tfinger.y;
                printf("Finger up [%f, %f]\n", bmp_x, bmp_y);
            }
        }

        bmp_x += bmp_speed_x;
        bmp_y += bmp_speed_y;

        if (bmp_x <= 0 || bmp_x + 32 >= BSP_LCD_H_RES) bmp_speed_x *= -1;
        if (bmp_y <= 0 || bmp_y + 32 >= BSP_LCD_V_RES) bmp_speed_y *= -1;

        text_x += text_speed_x * text_direction_x;
        text_y += text_speed_y * text_direction_y;
        if (text_x <= 0 || text_x >= 200) text_direction_x *= -1;
        if (text_y <= 0 || text_y >= 200) text_direction_y *= -1;

        text_scale += text_scale_speed * scale_direction;
        if (text_scale <= 0.5f || text_scale >= 2.0f) scale_direction *= -1;

        clear_screen(renderer);
        draw_image(renderer, imageTexture, bmp_x, bmp_y, 32.0f, 32.0f);

        const SDL_Rect standardRect = {.x = 0, .y = 0, .w = FRAME_H_RES, .h = FRAME_H_RES};
        int pitch;
        void* cameraPixels;

        SDL_LockTexture(cameraTexture, &standardRect, &cameraPixels, &pitch);
        assert(pitch == FRAME_H_RES * 2);
        memcpy(cameraPixels, cam_buffer, FRAME_V_RES * FRAME_H_RES * 2);
        SDL_UnlockTexture(cameraTexture);

        draw_image(renderer, cameraTexture, 50, 50, FRAME_H_RES, FRAME_V_RES);
        if(is_image_processing_done()) {
            void *pixelsA;
            void *pixelsB;
            void *pixelsC;
            SDL_LockTexture(outputTextureA, &standardRect, &pixelsA, &pitch);
            assert(pitch == FRAME_H_RES * 2);
            SDL_LockTexture(outputTextureB, &standardRect, &pixelsB, &pitch);
            assert(pitch == FRAME_H_RES * 2);
            SDL_LockTexture(outputTextureC, &standardRect, &pixelsC, &pitch);
            assert(pitch == FRAME_H_RES * 2);
            image_processing_update_pixels(pixelsA, pixelsB, pixelsC);
            SDL_UnlockTexture(outputTextureA);
            SDL_UnlockTexture(outputTextureB);
            SDL_UnlockTexture(outputTextureC);
            image_processing_start(cam_buffer);
        }
        draw_image(renderer, outputTextureA, 100 + FRAME_H_RES, 50, FRAME_H_RES, FRAME_V_RES);
        draw_image(renderer, outputTextureB, 50, 100 + FRAME_V_RES, FRAME_H_RES, FRAME_V_RES);
        draw_image(renderer, outputTextureC, 100 + FRAME_H_RES, 100 + FRAME_V_RES, FRAME_H_RES, FRAME_V_RES);
        draw_image(renderer, imageTexture, bmp_x, bmp_y, 32.0f, 32.0f);
        draw_text(renderer, textTexture, text_x, text_y, 120, 20 * text_scale);
        SDL_RenderPresent(renderer);
        vTaskDelay(pdMS_TO_TICKS(6));
    }
}

#define HEAP_TRACE_MAX_STACK (450)

//static heap_trace_record_t record_buffer[HEAP_TRACE_MAX_STACK];
void app_main(void) {
    (void) app_main;
    pthread_t sdl_pthread;

    pthread_attr_t attr;
    print_psram_info();
//    heap_trace_init_standalone(record_buffer, HEAP_TRACE_MAX_STACK);
//    heap_trace_start(HEAP_TRACE_ALL);
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 28000);  // Set the stack size for the thread

    int ret = pthread_create(&sdl_pthread, &attr, sdl_thread, NULL);
    if (ret != 0) {
        printf("Failed to create SDL thread: %d\n", ret);
        return;
    }

    pthread_detach(sdl_pthread);
    init_camera();

}

void print_psram_info() {
    printf("PSRAM HEAP: total %d, free: %d; largest free block: %d\n\r",
           heap_caps_get_total_size(MALLOC_CAP_SPIRAM),
           heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
           heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)
           );
}
