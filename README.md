# tsp
A partial solution for Travelling salesman problem
Space is used to save time.
Multimap (ordered) is used to sort edges and keep nearest neighbors.
Cost of edges are initialized by random numbers from 1 to ULONG_MAX.
Now 20 nodes are tried and assumed all are connected.
Program is single threaded to measure performance improvement later.
// C++ 11
// Intel(R) Core(TM) i5-8265U CPU @1.60 GHz @1.80 GHz
