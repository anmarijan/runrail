GIT_HASH = $(shell git log -1 --format="%h")
OBJS = runrail.o SVGConv.o RunControl.o RailLine.o TrainBase.o train.o Lookup.o motor.o common.o
PROGRAM = runrail.exe
CXX = g++
CXXFLAGS = -std=c++1y -Wall -DGITVERSION=\"$(GIT_HASH)\"
LDFLAGS = -static -lboost_program_options-mt
.cpp.o:
	$(CXX) $(CFLAGS) -c $< -o $@

$(PROGRAM) : $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $(PROGRAM)

clean:
	rm -f *.o $(PROGRAM)
