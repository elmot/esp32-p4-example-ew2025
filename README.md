# Test of port of SDL3 for ESP32

Experimental!

## Build

```
git clone git@github.com:georgik/esp32-sdl3-test.git
mkdir components
cd components
git clone --branch feature/esp-idf git@github.com:georgik/SDL.git
cd ..
idf.py set-target esp32-s3
idf.py build
```
