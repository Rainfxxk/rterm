CC		:= gcc
CFLAGS 	:= -Wall -Wextra -g
LFLAGS	:= -lSDL2 -lSDL2_ttf

SRC_DIR := ./src
TARGET  := ./build/rterm
TARGET_DIR  := ./build/rterm
SRCS 	:= $(wildcard $(SRC_DIR)/*.c)
OBJS 	:= $(patsubst %.c,%.o,$(SRCS))
OBJ_D   := $(patsubst %.c,%.d,$(SRCS))

all: $(TARGET)
	./$^

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJS):%.o:%.c
	$(CC) $(CFLAGS) -MM -MT $@ -MF $(patsubst %.o, %.d, $@) $<
	$(CC) $(CFLAGS) -c -o $@ $<

-include $(OBJS_D)

$(TARGET): $(OBJS)
	$(CC) $^ -o $@ $(LFLAGS)

clean:
	rm $(OBJS) $(OBJ_D)
	rm -rf $(TARGET_DIR)

.PHONY: clean
