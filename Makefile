TARGET = main.out
INCLUDE = -I/opt/vc/include/
LIBS = -lv4l2
CC = cc
CFLAGS = -g -Wall

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
    $(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
    $(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
    -rm -f *.o
    -rm -f $(TARGET)
