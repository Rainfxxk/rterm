CC		:= gcc
CFLAGS 	:= -Wall -Wextra
LFLAGS	:= -lSDL2 -lSDL2_ttf

SRC_DIR := ./src
OBJ_DIR := ./build
SRCS 	:= $(wildcard $(SRC_DIR)/*.c)
OBJS 	:= $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

all: $(OBJ_DIR)/rterm
	./$^

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) 
	$(CC) $(CFLAGS) $^ -c -o $@


$(OBJ_DIR)/rterm: $(OBJS)
	$(CC) $^ -o $@ $(LFLAGS)

clean:
	rm -rf $(OBJ_DIR)

.PHONY: clean
