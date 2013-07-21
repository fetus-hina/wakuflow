SRCS=wakuflow.cpp waku.cpp waku_v2.cpp gd.cpp
OBJS=$(SRCS:.cpp=.o)
TARGET=wakuflow

.PHONY: all clean

all: $(TARGET)

%.o: %.cpp
	g++ -c -O2 -march=native \
		-Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith \
		-o $@ $<
	
$(TARGET): $(OBJS)
	g++ -o $(TARGET) $(OBJS) ../image/libimage.a -lgd -lboost_program_options

clean:
	$(RM) $(OBJS) $(TARGET)
