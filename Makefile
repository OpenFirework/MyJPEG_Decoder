CXX := g++
CC := gcc

TARGET := pic_decode.bin
INCLUDE_DIRS := ./jpeg ./bmp
JPEG_SRCS := ./jpeg/jpeg_decoder.cpp
BMP_SRCS := ./bmp/bmp_code.cpp
COMMON_FLAGS += $(foreach includedir,$(INCLUDE_DIRS),-I$(includedir))
SRCS := $(JPEG_SRCS) $(BMP_SRCS) test_main.cpp

$(TARGET):$(SRCS)
	$(CXX) $(COMMON_FLAGS) $(SRCS) -o $(TARGET)
