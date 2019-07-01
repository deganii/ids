cmake_minimum_required(VERSION 3.7)
project(ids C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE on )

include_directories(
        ./native/
        /usr/local/include/SDL2/
        /opt/vc/include/
        /opt/vc/include/interface/vmcs_host/linux/
        /opt/vc/include/interface/vcos/pthreads/
        /opt/vc/src/hello_pi/libs/ilclient/
)

link_directories(
        /opt/vc/lib/
        /opt/vc/src/hello_pi/libs/ilclient/
)

add_compile_options(
        -D_REENTRANT -Wall -g
        -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -DUSE_EXTERNAL_OMX
        -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST
        -DUSE_VCHIQ_ARM
)

add_library(idslib native
        native/display.c
        native/display.h
        native/fontinfo.h
        native/shapes.c
        native/shapes.h
        native/tis_v4l2.c
        native/tis_v4l2.h
        native/touch.c
        native/touch.h
        native/ui.c
        native/ui.h
        native/settings.h
        native/omx_encoder.c
        native/omx_encoder.h
        native/types.h)

add_executable(ids main.cpp)
add_executable(ids-c main.c)
add_executable(displaytest tests/displaytest.c)

target_link_libraries(idslib
        mtdev
        jpeg
        brcmGLESv2
        SDL2
        GLESv2
        v4l2
        EGL
        GLESv2
        bcm_host
        openmaxil
        ilclient
        vcos
        vchiq_arm
        pthread rt m
        /usr/local/lib/liblfds711.a
)

target_link_libraries(ids idslib)
target_link_libraries(ids-c idslib)
target_link_libraries(displaytest idslib)

add_dependencies(ids-c idslib)
add_dependencies(ids idslib)
add_dependencies(displaytest idslib)



