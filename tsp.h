#ifndef TCP_H_
#define TCP_H_
#pragma once

#define SINGLE_THREAD 1

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

/*
// boost logging
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

//void InitBoostLog();
//void ChangeBoostLogLevel(logging::trivial::severity_type loglevel);
*/
using namespace std;
using namespace chrono;

/*****************************************************************************************************************************/

///            CODE IS NOT YET DOCUMENTED/COMMENTED, as DESIGN NOT YET FINALIZED

/*****************************************************************************************************************************/

typedef unsigned long int ulint;
typedef unsigned long long int ullint; // for permutations
const auto MAX_EDGE_COST = ULONG_MAX;

typedef ulint node_id_t;
typedef ulint node_ct_t; // for node count
typedef ulint edge_cost_t;

typedef ullint permutation_cost_t;
typedef vector<node_id_t> permutation_t;
typedef ullint permutations_ct_t;

// global variables
// atomic<permutations_ct_t> expected_permutations_to_generate;
// atomic<permutations_ct_t> partial_permutations_generated;

class tsp_Utils {
public:
  static permutations_ct_t factorial(node_id_t N);
  static void process_system_error(system_error &err);
  static void Convert_Time_Duration(long long Duration);
};
/** global function declarations */
struct SinglePermutation;

void Initialize_Edges();
void generate_and_test_permutations(SinglePermutation *pPermutation);
void GenerateThreads();
void CheckNewPermutation(permutation_t &newPermn, permutation_cost_t &newCost);
void compute_initial_min_cost_permutation();
void ProcessPermutations();
ostream &operator<<(ostream &os, permutation_t permn);

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
    critical_error = 0,
    basic,
    error,
    info,
    debug
  };
  inline static Loglevel logLevel;

public:
  static void SetLogLevel(Loglevel level = Loglevel::basic);
  static void LogCriticalError(const string& mesg);  
  void static LogInfo(const string &mesg);
  void static LogInfo(const string &mesg, SinglePermutation *pPermutation);
  template <typename U> void static LogInfo(const string& mesg, U val);
  void static LogDebugInfo(const string &mesg);
  void static LogDebugInfo(const string &mesg, SinglePermutation *pPermutation);
  void static LogDebugInfo(const string &mesg, permutation_t permn,
                           permutation_cost_t cost = {});
};

// tsp_exception
class tsp_exception : public exception {
  string message = "Unknown exception";

public:
  /*
    tsp_exception(const char *file_name, const unsigned long line_number,
                  const char *function_name, const char *data)
        : exception(data) {}
  */
  tsp_exception(const string &file_name, const unsigned long &line_number,
                const string &function_name, const string &mesg)
      : exception(mesg.c_str()) {
    ostringstream oss;
    oss << file_name << " : " << line_number << " : " << function_name << " : "
        << mesg;
    message = oss.str();
  }
  char const *what() const { return message.c_str(); }
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
  inline static vector<permutation_t> avoided_permns =
      *new vector<permutation_t>();
  inline static vector<permutation_t> processed_permns =
      *new vector<permutation_t>();

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