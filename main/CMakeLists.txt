idf_component_register(
    SRCS
        "esp32-uvc-opencv-example.c"
        "text.c"
        "filesystem.c"
        "graphics.c"
        "camera.c"
        "yuy2.c"
        "opencv_transform.cpp"
    INCLUDE_DIRS "."
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=overloaded-virtual)