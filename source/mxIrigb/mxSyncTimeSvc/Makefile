EXEC=ServiceSyncTime
CXX=g++
LDFLAGS = -L../mxirig -lmxirig-$(shell uname -m) -lrt -lm

all: $(EXEC).o
	$(CXX) $(EXEC).o -o $(EXEC) $(LDFLAGS)

clean:
	rm -rf $(EXEC).o $(EXEC)
