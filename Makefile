# Zuma Game - Makefile (Raylib GPU Backend)
#
# Prerequisites:
#   1. MinGW-w64 with g++
#   2. Raylib 5.5+ installed at ${RAYLIB_PATH}
#   3. Set RAYLIB_PATH environment variable (default: C:/raylib)

RAYLIB_PATH ?= C:/raylib
CXX        ?= g++
CXXFLAGS   := -g -Wall -Wextra -std=c++17
INCLUDES   := -Iinclude -I$(RAYLIB_PATH)/include
LDFLAGS    := -L$(RAYLIB_PATH)/lib -lraylib -lopengl32 -lgdi32 -lwinmm
SRCDIR     := src
OBJDIR     := build
TARGET     := $(OBJDIR)/ZumaGame.exe
SOURCES    := $(wildcard $(SRCDIR)/*.cpp)

.PHONY: all build run clean

all: build

build: $(OBJDIR) $(TARGET)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

run: build
	@echo "Starting Zuma Game..."
	$(TARGET)

clean:
	@rm -f $(OBJDIR)/*.exe $(OBJDIR)/*.o
	@echo "Clean complete."
