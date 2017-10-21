#ifndef IBCGRASS_H
#define IBCGRASS_H

#include <string>

#define getGridArea() GridSize*GridSize

extern long GridSize;

extern std::string NameSimFile;
extern std::string outputPrefix;
extern RandomGenerator rng;
extern Output output;
#endif // IBCGRASS_H
