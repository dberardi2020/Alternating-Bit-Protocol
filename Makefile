# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -Wall -g

OBJS = project2.o student2.o

# the build target executable:
TARGET = project2

all: $(TARGET)

project2: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o project2

project2.o: project2.c
	$(CC) $(CFLAGS) -c project2.c student2.c

clean:
	rm -f *~ *.o project2