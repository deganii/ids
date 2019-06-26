TARGET    = main.out
CFLAGS    += -D_REENTRANT -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM
SRC       = $(wildcard native/*.c) main.c
HEADERS   = $(wildcard native/*.h)
#OBJ      = $(SRC:.c=.o)
OBJECTS   = $(patsubst %.c, %.o, $(wildcard native/*.c)) $(patsubst %.c, %.o, $(wildcard *.c))
LIBS      = -lmtdev -ljpeg -lbrcmGLESv2 -lSDL2 -lGLESv2 -lv4l2 -lEGL -lGLESv2 -lbcm_host -lopenmaxil -lilclient -lvcos -lvchiq_arm -lpthread -lrt -lm
INC_PATH  = -I./native/ -I/usr/local/include/SDL2 -I/opt/vc/include/ -I/opt/vc/include/interface/vmcs_host/linux/ -I/opt/vc/include/interface/vcos/pthreads/ -I/opt/vc/src/hello_pi/libs/ilclient/
LIB_PATH  = -L/opt/vc/lib/ -L/opt/vc/src/hello_pi/libs/ilclient/

.PHONY: default all clean

default: $(TARGET)
all: default

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(INC_PATH) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LIB_PATH) $(LIBS) -o $@

rpi: prepare
	$(CC) -MM $(SRC) $(CFLAGS) $(INC_PATH) -o main.out $(LIB_PATH) $(LIBS)

prepare:
	rm -f main.out $(OBJS)


clean:
	-rm -f *.o
	-rm -f $(TARGET)