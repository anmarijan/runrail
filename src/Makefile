CC = g++
OBJS = runrail.o SVGConv.o RunControl.o RailLine.o TrainBase.o train.o Lookup.o motor.o common.o
PROGRAM = runrail.exe
GIT_HASH = $(shell git log -1 --format="%h")
CFLAGS  = -std=c++1y -Wall -DGITVERSION=\"$(GIT_HASH)\"
LFLAGS  = -static
.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(PROGRAM) : $(OBJS)
	$(CC)  $(OBJS)  $(LFLAGS) -o $(PROGRAM)

$(OBJS) :

clean:
	rm -f *.o $(PROGRAM)
