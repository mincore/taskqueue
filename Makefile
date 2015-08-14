CLFAGS=-std=c99 -Wall -g
LDFLAGS=-lpthread

TARGET=test_taskqueue
OBJS=$(TARGET).o taskqueue.o

$(TARGET):$(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

%.o:%.c
	$(CC) $(CLFAGS) $< -c -o $@

clean:
	rm -f $(OBJS) $(TARGET)
