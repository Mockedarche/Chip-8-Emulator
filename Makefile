CC = gcc
CFLAGS = -O2 -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lraylib -framework IOKit -framework Cocoa -framework OpenGL
TARGET = chip_8_emulator

all: $(TARGET)

$(TARGET): chip_8_emulator.c
	$(CC) chip_8_emulator.c chip_8_core.c timing.c -o $(TARGET) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(TARGET)
