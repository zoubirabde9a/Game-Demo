/* Random: pulls in random.h, a fixed table of pseudo-random numbers and
   random_series, which walks that table from a seed. The calls are Seed,
   RandomChoice, RandomUnilateral (0 to 1), RandomBilateral (-1 to 1) and
   RandomBetween. The same seed always gives the same sequence. */
#include "random.h"
