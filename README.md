# Test of port of SDL3 for ESP32

Experimental port of SDL3 to ESP32 ESP-IDF v5.3.

Working parts:
- minimalistic SDL3 build
- SDL3 - littlefs integration
- SDL3_timer
- SDL_image - BMP
- SDL_ttf


## Build

```
git clone git@github.com:georgik/esp32-sdl3-test.git
cd esp32-sdl3-test

cd components/SDL
git clone --branch feature/esp-idf git@github.com:georgik/SDL.git
cd ../..

cd components/SDL_ttf
git clone git@github.com:libsdl-org/SDL_ttf.git --depth 10
cd ../..

cd components/SDL_image
git clone git@github.com:libsdl-org/SDL_image.git --depth 10
cd ../..

idf.py set-target esp32-s3
idf.py build
```

## Notes

- screen resolution bigger than 320x100 requires enabled PSRAM
- SDL surface for ESP-IDF framebuffer is defined for RGB565
- drawing surface requires `SDL_FRect` as destination, not `SDL_Rect`, in `SDL_Rect` case nothing draws

## Credits

- FreeSans.ttf - https://github.com/opensourcedesign/fonts/blob/master/gnu-freefont_freesans/FreeSans.ttf
