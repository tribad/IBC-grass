PROJ=ibcgrass

SRC=Cell.cpp\
    Environment.cpp\
    Genet.cpp\
    Grid.cpp\
    GridEnvir.cpp\
    IBC-grass.cpp\
    Output.cpp\
    Parameters.cpp\
    Plant.cpp\
    RandomGenerator.cpp\
    Seed.cpp\
    Traits.cpp

OBJ=$(SRC:.cpp=.o)

CXXFLAGS=-O0 -g -Wall -std=c++14

all : ibcgrass

ibcgrass : depend $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(PROJ)

clean:
	$(RM) -f $(PROJ)
	$(RM) -f $(OBJ)

depend : $(SRC)
	$(CXX) $(CXXFLAGS) -M $(SRC) > depend

-include "depend"
