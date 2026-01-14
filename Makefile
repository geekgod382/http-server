CC = gcc
CFLAGS = -Wall -O2

ifeq ($(OS), Windows_NT)
	TARGET = server.exe
	LIBS = -lws2_32
	RM = del
else
	TARGET = server
	LIBS = 
	RM = rm -f
endif

SRC = server.c
OBJS = $(SRC:.c=.o)

all: $(TARGET)
$(TARGET) : $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET)