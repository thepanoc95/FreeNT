CC := x86_64-w64-mingw32-gcc
TARGET := liberty.exe
SRC := main.c
BUILD_DIR := ../build/x64

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(SRC)
	$(CC) -std=c11 -Wall -Wextra -Wl,-subsystem,console -Wl,-e,mainCRTStartup -o $@ $(SRC) -lntdll -L../build/x64 -lfreedll -lntdylib