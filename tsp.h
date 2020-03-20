#ifndef TCP_H_
#define TCP_H_
#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <stack>
#include <thread>
#include <vector>

using namespace std;
using namespace chrono;

/*****************************************************************************************************************************/

///            CODE IS NOT YET DOCUMENTED/COMMENTED, as DESIGN NOT YET FINALIZED

/*****************************************************************************************************************************/

typedef unsigned long int ulint;
typedef unsigned long long int ullint; // for permutations
ulint total_nodes;                     // total number of nodes
const auto MAX_EDGE_COST = ULONG_MAX;

typedef ulint node_id_t;
typedef ulint node_ct_t; // for node count
typedef ulint edge_cost_t;

typedef ullint permutation_cost_t;
typedef vector<node_id_t> permutation_t;
typedef ullint permutations_ct_t;

// global variables
atomic<permutations_ct_t> expected_permutations_to_generate = 0;
atomic<permutations_ct_t> partial_permutations_generated = 0;

/** global function declarations */
inline permutations_ct_t factorial(node_id_t N);
void process_system_error(system_error &err);
void Initialize_Edges();
void generate_and_test_permutations(SinglePermutation *pPermutation);
void GenerateThreads();
void CheckNewPermutation(permutation_t &newPermn, permutation_cost_t &newCost);
void compute_initial_min_cost_permutation();
void Convert_Time_Duration(long long Duration);

void tsp_terminate();
void tsp_exit(int status);

// SinglePermutation
struct SinglePermutation {
public:
  permutation_t permutation;
  permutation_cost_t permutation_cost = 0;
  SinglePermutation();
  SinglePermutation(permutation_cost_t permn_cost, permutation_t permn);
  SinglePermutation(const SinglePermutation &rhs);
      
  void operator=(const SinglePermutation &rhs);
  friend ostream &operator<<(ostream &os, SinglePermutation permn);
};

// Logger
static class Logger {
public:
  inline static mutex cout_mutex;
  inline static enum class Loglevel {
    basic = 1,
    critical_error,
    error,
    info,
    debug
  };
  inline static Loglevel logLevel;

public:
  static void LockCoutMutex() { cout_mutex.lock(); }
  static void UnLockCoutMutex() { cout_mutex.unlock(); }

  static void SetLogLevel(Loglevel level = Loglevel::basic);

  template <typename T> 
  static void LogCriticalError(const T mesg);

  template <typename T>
  void static LogInfo(const T mesg, SinglePermutation *pPermutation = 0);

  template <typename T, typename U>
  void static LogInfo(const T mesg, U val);

  template <typename T> void static LogDebugInfo(T mesg);

  template <typename T>
  void static LogDebugInfo(const T mesg, SinglePermutation *pPermutation);

  template <typename T>
  void static LogDebugInfo(T mesg, permutation_t permn,
                           permutation_cost_t cost = {});
};

// tsp_exception
class tsp_exception : public exception {
public:
  tsp_exception(const char *data) : exception(data) {}
};

// Permutations
static class Permutation_Collection {
private:
  inline static mutex _permutations_mutex;
  inline static mutex _min_permutation_mutex;
  inline static queue<SinglePermutation *> _permutations =
      *new queue<SinglePermutation *>();

  inline static SinglePermutation min_cost_permutation;
  inline static SinglePermutation initial_min_cost_permn_;
  inline static vector<node_id_t> avoided_permutations =
      *new vector<node_id_t>();

public:

  permutations_ct_t total_processed_permns_ct_ = 0;
  Permutation_Collection(const SinglePermutation &initial_min_cost_permn);
  void CheckSetMinCostPermutation(
      SinglePermutation *pPermutation); // done in a single call
  void
  CheckAddPartialPermutationGreaterThanMinCost(SinglePermutation *pPermutation);
  void OutputMinCostPermutation();
  void Pop(SinglePermutation *pPermutation);
  void Push(SinglePermutation *pPermutation);
  void Empty(bool &bPermnsEmpty);
  permutations_ct_t GetAvoidedPermutationsCount();
};

#endif TCP_H_