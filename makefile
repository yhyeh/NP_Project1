# g++ (Ubuntu 9.3.0-10ubuntu2) 9.3.0
CC=g++
TARGET=npshell

SRCS=$(shell ls *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(SRCS))

OUT_DIR=bin
OUT_OBJS=$(addprefix $(OUT_DIR)/, $(OBJS))

$(TARGET): $(OUT_OBJS)
	$(CC) -o $@ npshell.cpp

$(OUT_DIR)/%.o: %.cpp
	$(CC) -o $@ -c $<

.PHONY: clean
clean:
	rm -f npshell $(OUT_OBJS)
