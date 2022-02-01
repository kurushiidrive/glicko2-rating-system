CC=g++
CXXFLAGS=-std=c++11 -Wall -Wextra
SOURCES=glicko2-client.cpp Player.cpp Glicko2.cpp
OBJECTS=$(SOURCES:.cpp=.o)
DEPS=Player.h Glicko2.h
EXEC=glicko2-client

%.o: %.cpp $(DEPS)
	$(CC) -o $@ -c $< $(CXXFLAGS)

all : $(SOURCES) $(EXEC)

$(EXEC) : $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)
	rm -f $(OBJECTS)

clean :
	rm -f $(EXEC) $(OBJECTS)
