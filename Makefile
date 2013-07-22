OBJ=rbtree.o
TEST_OBJ=rbtree_basic_test.o

TARGET=librbtree.so
TEST_TARGETS=rbtree_basic_test

OFLAGS=-O0 -g

CFLAGS=$(OFLAGS) -I. -Wall -std=c99

all: $(TARGET) $(TEST_TARGETS)

.c.o:
	$(CC) $(CFLAGS) -c $<

$(TARGET): $(OBJ)

rbtree_basic_test: $(TEST_OBJ)
	$(CC) -o rbtree_basic_test $(OBJ) rbtree_basic_test.o $(CFLAGS)

clean:
	$(RM) $(OBJ) $(TARGET) $(TEST_OBJ) $(TEST_TARGETS)

.PHONY: clean all
