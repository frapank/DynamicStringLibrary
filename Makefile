CC      := cc
CFLAGS  := -g -std=c11 -Wall -Wextra -O2
SAN_CFLAGS := -std=c11 -Wall -Wextra -g -fsanitize=address,undefined

LIB_NAME := dstr
TARGET_STATIC := lib$(LIB_NAME).a
TARGET_SHARED := lib$(LIB_NAME).so

HEADER := $(LIB_NAME).h
SRC    := $(LIB_NAME).c
OBJ    := $(SRC:.c=.o)

TEST_FILE := tester.c
TEST_EXEC := tester
TEST_ASAN_EXEC := tester_asan

.PHONY: all clean test cppcheck sanitize run_asan

all: $(TARGET_STATIC) $(TARGET_SHARED)

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(TARGET_STATIC): $(OBJ)
	ar rcs $@ $(OBJ)
	@echo "Built static library: $@"

$(TARGET_SHARED): $(OBJ)
	$(CC) -shared -o $@ $(OBJ)
	@echo "Built shared library: $@"

test: $(TARGET_STATIC)
	$(CC) $(CFLAGS) $(TEST_FILE) $(TARGET_STATIC) -o $(TEST_EXEC)
	@echo "Built tester executable"

run_test: test
	./$(TEST_EXEC)

valgrind: test
	valgrind --leak-check=full \
	         --show-leak-kinds=all \
	         --track-origins=yes \
	         --error-exitcode=1 \
	         ./$(TEST_EXEC)

sanitize:
	$(CC) $(SAN_CFLAGS) $(SRC) $(TEST_FILE) -o $(TEST_ASAN_EXEC)
	@echo "Built tester_asan executable"

run_asan: sanitize
	ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 ./$(TEST_ASAN_EXEC)

cppcheck:
	cppcheck --enable=warning,style,performance,portability \
	         --error-exitcode=1 \
	         -D__GNUC__ -D__x86_64__ \
	         --suppress=unusedFunction \
	         --suppress=staticFunction \
	         $(SRC) $(HEADER)
format:
	clang-format -i $(HEADER) $(SRC)

format-check:
	@clang-format dstr.c | diff -u - dstr.c || \
		(echo "Wrong format in dstr.c, run 'make format'" >&2; exit 1)
	@clang-format dstr.h | diff -u - dstr.h || \
		(echo "Wrong format in dstr.h, run 'make format'" >&2; exit 1)

clean:
	rm -f *.o *.a *.so $(TEST_EXEC) $(TEST_ASAN_EXEC)
