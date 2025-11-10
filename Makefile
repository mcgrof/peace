# Detect OS
UNAME_S := $(shell uname -s)
ifeq ($(OS),Windows_NT)
    PLATFORM := Windows
else
    PLATFORM := $(UNAME_S)
endif

CC = gcc
CFLAGS = -Wall -O2
TARGET = waves
SRC = waves.c
TRANSFORMER = transformer
TRANSFORMER_SRC = transformer.c

# Platform-specific settings
ifeq ($(PLATFORM),Windows)
    # Windows settings (MinGW/MSYS2)
    TARGET := $(TARGET).exe
    LIBS = -lopengl32 -lglfw3 -lgdi32 -lm
    LDFLAGS =
else ifeq ($(PLATFORM),Darwin)
    # macOS settings
    CFLAGS += -I/opt/homebrew/include -I/usr/local/include
    LDFLAGS = -L/opt/homebrew/lib -L/usr/local/lib
    LIBS = -framework OpenGL -lglfw -lm
else ifeq ($(findstring BSD,$(PLATFORM)),BSD)
    # BSD settings (FreeBSD, OpenBSD, NetBSD)
    CFLAGS += -I/usr/local/include -I/usr/X11R6/include
    LDFLAGS = -L/usr/local/lib -L/usr/X11R6/lib
    LIBS = -lGL -lglfw -lm
else
    # Linux and other Unix-like systems
    LIBS = -lGL -lglfw -lm
    LDFLAGS =
endif

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS) $(LIBS)

$(TRANSFORMER): $(TRANSFORMER_SRC)
	$(CC) $(CFLAGS) -o $(TRANSFORMER) $(TRANSFORMER_SRC) $(LDFLAGS) $(LIBS)

capture: capture_simple.c
	$(CC) $(CFLAGS) -o capture capture_simple.c $(LDFLAGS) $(LIBS)

capture-advanced: capture.c
	$(CC) $(CFLAGS) -o capture capture.c $(LDFLAGS) -lGL -lGLEW -lglfw -lm

demo-capture: capture
	./capture_demo.sh

all: $(TARGET) $(TRANSFORMER)

clean:
	rm -f $(TARGET) $(TRANSFORMER) peaceful peaceful_waves capture
	rm -rf frames
	rm -f peaceful_waves.gif peaceful_waves_small.gif peaceful_snapshot.png

run: $(TARGET)
	./$(TARGET)

run-transformer: $(TRANSFORMER)
	./$(TRANSFORMER)

style:
	clang-format -style="{BasedOnStyle: Google, IndentWidth: 4}" -i $(SRC) $(TRANSFORMER_SRC)

.PHONY: all clean run run-transformer style capture demo-capture