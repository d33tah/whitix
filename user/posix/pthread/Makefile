DEPTH = ../..

include $(DEPTH)/make.rules
include $(DEPTH)/make.config
include ../make.inc

OBJS = pthread.o sem.o pthread_attr.o pthread_cond.o pthread_mutex.o file.o pthread_specific.o

build: $(OBJS)

clean:
	$(RM) *.o
