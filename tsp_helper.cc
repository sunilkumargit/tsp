/** Definitions */
#include "tsp.h"

/*****************************************************************************************************************************/

///            CODE IS NOT YET DOCUMENTED/COMMENTED, as DESIGN NOT YET FINALIZED

/*****************************************************************************************************************************/


void tsp_terminate() {
  cout << "Total Nodes: " << total_nodes << endl;
  cout << "term_func was called by terminate." << endl;
  cout.flush();
  terminate();
}
void tsp_exit(int status) {
  cout << "Total Nodes: " << total_nodes << endl;
  if (status == 0)
    cout << "Status Success" << endl;
  else
    cout << "Status Success" << endl;
  cout.flush();
  exit(status);
}

inline permutations_ct_t factorial(node_id_t N) { 
    permutations_ct_t prod = 1;
  for (node_id_t i = 2; i <= N; ++i)
    prod *= i;
  return prod;
}
void process_system_error(system_error &err) {
  cout << "caught system_error " << err.what() << endl; // error message
  auto ec = err.code();
  cout << "category: " << ec.category().name() << " : "
       << ec.category().message(ec.value()) << endl;
  cout << "value: " << ec.value() << endl;
  cout << "message: " << ec.message() << endl;
}
ostream &operator<<(ostream &os, permutation_t permn) {
  copy(permn.begin(), permn.end(), ostream_iterator<node_id_t>(os, ", "));
  cout << endl;
  return os;
}

ostream &operator<<(ostream &os, SinglePermutation permn) {
  os << "Cost: " << permn.permutation_cost;
  copy(permn.permutation.begin(), permn.permutation.end(),
       ostream_iterator<node_id_t>(os, ", "));
  cout << endl;
  return os;
}

// Logger
mutex Logger::cout_mutex;
void Logger::SetLogLevel(Loglevel level) { logLevel = level; }

template <typename T> void Logger::LogCriticalError(const T mesg) {
  cout_mutex.lock();
  cout << mesg << endl;
  cout_mutex.unlock();
  return;
}

template <typename T>
void Logger::LogInfo(const T mesg, SinglePermutation *pPermutation) {
  cout_mutex.lock();
  if (LogLevel < loglevel::info) {
    cout_mutex.unlock();
    return;
  }
  cout << mesg;
  cout << (*pPermutation);
  cout_mutex.unlock();
}

template <typename T, typename U> void Logger::LogInfo(const T mesg, U val) {
  cout_mutex.lock();
  if (LogLevel < loglevel::info) {
    cout_mutex.unlock();
    return;
  }
  cout << mesg << val << endl;

  cout_mutex.unlock();
}

template <typename T> void Logger::LogDebugInfo(T mesg) {
  cout_mutex.lock();
  if (LogLevel < loglevel::debug) {
    cout_mutex.unlock();
    return;
  }
  cout << mesg;
  cout_mutex.unlock();
}

template <typename T>
void Logger::LogDebugInfo(const T mesg, SinglePermutation *pPermutation) {
  cout_mutex.lock();
  if (LogLevel < loglevel::debug) {
    cout_mutex.unlock();
    return;
  }
  cout << mesg;
  cout << *pPermutation;
  cout_mutex.unlock();
}

template <typename T>
void Logger::LogDebugInfo(T mesg, permutation_t permn,
                          permutation_cost_t cost = {}) {
  cout_mutex.lock();
  if (LogLevel < loglevel::debug) {
    cout_mutex.unlock();
    return;
  }
  cout << mesg;
  cout << permn;
  cout_mutex.unlock();
}

// SinglePermutation
SinglePermutation::SinglePermutation() : permutation_cost{} {}
SinglePermutation::SinglePermutation(permutation_cost_t permn_cost,
                                     permutation_t permn)
    : permutation(permn), permutation_cost(permn_cost) {}
SinglePermutation::SinglePermutation(const SinglePermutation &rhs)
    : permutation{rhs.permutation}, permutation_cost{rhs.permutation_cost} {}
void SinglePermutation::operator=(const SinglePermutation &rhs) {
  permutation.clear();
  permutation.resize(rhs.permutation.size());
  copy(rhs.permutation.cbegin(), rhs.permutation.cend(), permutation.begin());
  permutation_cost = rhs.permutation_cost;
}

// Permutations
// mutex Permutations::_permutations_mutex;
// mutex Permutations::_min_permutation_mutex;
Permutation_Collection::Permutation_Collection(
    const SinglePermutation &initial_min_cost_permn) {
  min_cost_permutation = initial_min_cost_permn;
  initial_min_cost_permn_ = initial_min_cost_permn;
}
void Permutation_Collection::CheckSetMinCostPermutation(
    SinglePermutation *pPermutation) // done in a single call
{
  _min_permutation_mutex.lock();
  ++total_processed_permns_ct_;
  if (pPermutation->permutation_cost < min_cost_permutation.permutation_cost)
    min_cost_permutation = *pPermutation;
  delete pPermutation;
  _min_permutation_mutex.unlock();
}
void Permutation_Collection::CheckAddPartialPermutationGreaterThanMinCost(
    SinglePermutation *pPermutation) {
  _permutations_mutex.lock();
  if (pPermutation->permutation_cost >= min_cost_permutation.permutation_cost) {
    avoided_permutations.push_back(pPermutation->permutation.size());
    delete pPermutation;
  } else
    Push(pPermutation);
  _permutations_mutex.unlock();
}
void Permutation_Collection::OutputMinCostPermutation() {
  Logger::LogInfo("Final Min Cost Permn: ", &min_cost_permutation);
  Logger::LogInfo("Initial Min Cost Permn: ", &initial_min_cost_permn_);
  Logger::LogInfo("Difference: ", initial_min_cost_permn_.permutation_cost -
                                      min_cost_permutation.permutation_cost);
}
void Permutation_Collection::Pop(SinglePermutation *pPermutation) {
  _permutations_mutex.lock();
  if (_permutations.size() == 0)
    throw tsp_exception("Attempt to pop from empty set of permutations");

  pPermutation->permutation_cost = _permutations.front()->permutation_cost;
  pPermutation->permutation.resize(_permutations.front()->permutation.size());
  copy(_permutations.front()->permutation.begin(),
       _permutations.front()->permutation.end(),
       pPermutation->permutation.begin());
  Logger::LogDebugInfo("Popped: ", pPermutation);
  _permutations.pop();
  _permutations_mutex.unlock();
}
void Permutation_Collection::Push(SinglePermutation *pPermutation) {
  // permutations_mutex_.lock();
  _permutations.push(pPermutation);
  Logger::LogDebugInfo("Pushed: ", pPermutation);
  // permutations_mutex_.unlock();
}
void Permutation_Collection::Empty(bool &bPermnsEmpty) {
  _permutations_mutex.lock();
  bPermnsEmpty = _permutations.empty();
  _permutations_mutex.unlock();
}
permutations_ct_t Permutation_Collection::GetAvoidedPermutationsCount() {
  permutations_ct_t total_avoided_permutations_ct = 0;
  for (auto ct : avoided_permutations) {
    permutations_ct_t prod = 1;
    node_id_t length = total_nodes - ct;
    for (permutation_ct_t i = 2; i <= length; i++)
      prod *= i;
    total_avoided_permutations_ct += prod;
  }
  return total_avoided_permutations_ct;
}

// Convert time
void Convert_Time_Duration(long long Duration) {
  double milli_seconds = 0.0, seconds = 0.0, minutes = 0.0, hours = 0.0;
  if (Duration < 1000) {
    cout << Duration << " Microseconds";
    return;
  } else
    milli_seconds = Duration / 1000.0;
  if (milli_seconds < 1000.0) {
    cout << milli_seconds << " Milliseconds";
    return;
  } else
    seconds = milli_seconds / 1000.0;
  if (seconds < 60.0) {
    cout << seconds << " Seconds";
    return;
  } else
    minutes = seconds / 60.0;
  if (minutes < 60.0) {
    cout << minutes << " Minutes";
    return;
  } else
    hours = minutes / 60.0;
  cout << hours << " Hours";
}