CXX=g++

DIR = mxirig \
unitest \
mxIrigUtil \
mxSyncTimeSvc

all:
	for i in $(DIR); do \
		make -C $$i; \
	done

clean:
	for i in $(DIR); do \
		make -C $$i clean; \
	done
