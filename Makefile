# Platform
CROSS_COMPILE =

# Compiler
CC = $(CROSS_COMPILE)g++

# Compiler flags
CFLAGS = -Wall -std=c++17

# Linker flags
LDFLAGS = -lpthread

# Target binary program
TARGET = channel

# Source files
SOURCES = main.cc

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

install:
	cp $(TARGET) /usr/local/bin

uninstall:
	rm -f /usr/local/bin/$(TARGET)

clean:
	rm -f $(TARGET)
