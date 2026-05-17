CC      := cc
CFLAGS  := -std=c11 -Wall -Wextra -O2
LDFLAGS :=
TEST_FILE := tester.c
LIB_NAME := dstr

SRC := dstr.c
OBJ := $(SRC:.c=.o)

.PHONY: all clean test

all: libstatic libshared

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

libstatic: $(OBJ)
	ar rcs lib$(LIB_NAME).a $(OBJ)
	@echo "Built static library: lib$(LIB_NAME).a"

libshared: $(OBJ)
	$(CC) -shared -o lib$(LIB_NAME).so $(OBJ)
	@echo "Built shared library: lib$(LIB_NAME).so"

test: libstatic
	$(CC) $(CFLAGS) $(TEST_FILE) -L. -l$(LIB_NAME) -o tester
	@echo "Built tester executable"

clean:
	rm -f *.o *.a *.so *.out tester


