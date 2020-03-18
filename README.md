# tsp
A partial solution for Travelling salesman problem
Space is used to save time.
Multimap (ordered) is used to sort edges and keep nearest neighbors.
Cost of edges are initialized by random numbers from 1 to ULONG_MAX.
All nodes are assumed all are connected.
// C++ 11
// Intel(R) Core(TM) i5-8265U CPU @1.60 GHz @1.80 GHz re
1. Changes required : Path class & Permutation class are almost duplicates. Merge them.
2. All permutations must be either processed or avoided by prefixes.
3. Improve time required.
4. Document the code well.
