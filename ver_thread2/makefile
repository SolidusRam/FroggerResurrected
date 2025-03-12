CC = gcc
CFLAGS += -g -MMD -MP -fcommon
TARGET = frogger
LDFLAGS = -lncurses -lpthread -lm

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS) -I$(INCLUDE_DIR)

-include $(DEPS)

clean:
	@rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean