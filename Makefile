CC=g++
CFLAGS=-ggdb -c -Wall -I/u1/uaf/bellamy/Projects/Libraries/hdf5/include 
LDFLAGS=-L/u1/uaf/bellamy/Projects/Libraries/hdf5/lib -lhdf5_cpp -lhdf5
RD_SOURCES=readdata.cpp
RD_OBJECTS=$(RD_SOURCES:.cpp=.o)
CR_SOURCES=create.cpp
CR_OBJECTS=$(CR_SOURCES:.cpp=.o)

EXECUTABLE=test



all: $(RD_SOURCES) $(CR_SOURCES) $(EXECUTABLE)

$(EXECUTABLE):readdata create
	
readdata: $(RD_OBJECTS) 
	$(CC) $(LDFLAGS) $(RD_OBJECTS) -o $@

create: $(CR_OBJECTS)  
	$(CC) $(LDFLAGS) $(CR_OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@


