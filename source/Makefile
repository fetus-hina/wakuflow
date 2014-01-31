SRCS=$(shell ls *.cpp)
OBJS=$(SRCS:.cpp=.o)
TARGET=wakuflow

CXX=/usr/bin/g++
CXXFLAGS=-O2 -march=native -std=c++0x -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith
LDFLAGS=-lgd -lboost_program_options

.PHONY: all clean depend

all: $(DEPEND) $(TARGET)

depend:

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<
	
$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) ../image/libimage.a $(LDFLAGS)

clean:
	$(RM) $(OBJS) $(DEPEND) $(TARGET)
