EXEC=mxIrigUtil
CXX=g++
CXXFLAGS+= -Wno-write-strings
LDFLAGS = -L../mxirig -lmxirig-$(shell uname -m) -lrt -lm

all: $(EXEC).o
	$(CXX) $(EXEC).o -o $(EXEC) $(LDFLAGS)

clean:
	rm -rf $(EXEC).o $(EXEC)
