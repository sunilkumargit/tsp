/** Definitions */
#include "tsp.h"

/*****************************************************************************************************************************/

///            CODE IS NOT YET DOCUMENTED/COMMENTED, as DESIGN NOT YET FINALIZED

/*****************************************************************************************************************************/
extern ulint total_nodes;  // total number of nodes
extern map<permutation_t, string> permns_map;
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
/*
void InitBoostLog() {
  logging::add_file_log(keywords::file_name = "tsp_%N.log",
                        keywords::rotation_size = 10 * 1024 * 1024,
                        keywords::time_based_rotation =
                            sinks::file::rotation_at_time_point(0, 0, 0),
                        keywords::format = "[%TimeStamp%]: %Message%");

  logging::core::get()->set_filter(logging::trivial::severity >=
                                   logging::trivial::info);
}
void ChangeBoostLogLevel(logging::trivial::severity_type loglevel) {
  logging::core::get()->set_filter(logging::trivial::severity >= loglevel);
}
*/

permutations_ct_t tsp_Utils::factorial(node_id_t N) {
  permutations_ct_t prod = 1;
  for (node_id_t i = 2; i <= N; ++i) prod *= i;
  return prod;
}

void tsp_Utils::process_system_error(system_error &err) {
  cout << "caught system_error " << err.what() << endl;  // error message
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
  os << "Cost: " << permn.permutation_cost << " : ";
  copy(permn.permutation.begin(), permn.permutation.end(),
       ostream_iterator<node_id_t>(os, ", "));
  cout << endl;
  return os;
}

// Logger
void Logger::SetLogLevel(Loglevel level) { logLevel = level; }

void Logger::LogCriticalError(const string &mesg) {
  lock_guard lg(cout_mutex);
  cout << mesg << endl;
}
void Logger::LogInfo(const string &mesg) {
  lock_guard lg(cout_mutex);
  if (logLevel < Logger::Loglevel::info) return; 
  cout << this_thread::get_id() << " : " << mesg << endl;
  cout_mutex.unlock();
}
void Logger::LogInfo(const string &mesg, SinglePermutation *pPermutation) {
  lock_guard lg(cout_mutex);
  if (logLevel < Logger::Loglevel::info) return; 
  cout << this_thread::get_id() << " : " << mesg << (*pPermutation) << endl;
  cout_mutex.unlock();
}

template <typename U>
void Logger::LogInfo(const string &mesg, U val) {
  lock_guard lg(cout_mutex);
  if (logLevel < Logger::Loglevel::info)  return;  
  cout << this_thread::get_id() << " : " << mesg << " : " << val << endl;
 
}

void Logger::LogDebugInfo(const string &mesg) {
  lock_guard lg(cout_mutex);
  if (logLevel < Logger::Loglevel::debug)  return;
  cout << this_thread::get_id() << " : " << mesg << endl;
}

void Logger::LogDebugInfo(const string &mesg, SinglePermutation *pPermutation) {
  lock_guard lg(cout_mutex);
  if (logLevel < Logger::Loglevel::debug)  return;  
  cout << this_thread::get_id() << " : " << mesg << " : " << *pPermutation       << endl;
}

void Logger::LogDebugInfo(const string &mesg, permutation_t permn,
                          permutation_cost_t cost) {
  SinglePermutation *pPermutation = new SinglePermutation(cost, permn);
  Logger::LogDebugInfo(mesg, pPermutation);
  delete pPermutation;
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

// Collection of Permutation
Permutation_Collection::Permutation_Collection(
    const SinglePermutation &initial_min_cost_permn) {
  min_cost_permutation = initial_min_cost_permn;
  initial_min_cost_permn_ = initial_min_cost_permn;
}
void Permutation_Collection::CheckSetMinCostPermutation(
    SinglePermutation *pPermutation)  // done in a single call
{
  lock_guard lg(_permutations_mutex);
  processed_permns.push_back(pPermutation->permutation);
  ++total_processed_permns_ct_;
  if (pPermutation->permutation_cost < min_cost_permutation.permutation_cost) {
    min_cost_permutation = *pPermutation;
    permns_map[pPermutation->permutation] += "SetMinCost";
  } else {
    permns_map[pPermutation->permutation] += "GreaterThanMinCost";
  }
  delete pPermutation;

}
void Permutation_Collection::CheckAddPartialPermutationGreaterThanMinCost(
    SinglePermutation *pPermutation) {
  lock_guard lg(_permutations_mutex);
  
  if (pPermutation->permutation_cost >= min_cost_permutation.permutation_cost) {
    avoided_permutations.push_back(pPermutation->permutation.size());
    avoided_permns.push_back(pPermutation->permutation);
    permns_map[pPermutation->permutation] += "Avoided";
    delete pPermutation;
  } else
    Push(pPermutation);
}

void Permutation_Collection::Pop(SinglePermutation *pPermutation) {
  lock_guard lg(_permutations_mutex);
  if (_permutations.size() == 0)
    throw tsp_exception(__FILE__, __LINE__, __FUNCTION__,
                        "Attempt to pop from empty set of permutations");

  pPermutation->permutation_cost = _permutations.front()->permutation_cost;
  pPermutation->permutation.resize(_permutations.front()->permutation.size());
  copy(_permutations.front()->permutation.begin(),
       _permutations.front()->permutation.end(),
       pPermutation->permutation.begin());
  Logger::LogDebugInfo("Popped: ", pPermutation);
  permns_map[pPermutation->permutation] += "Popped:";
  _permutations.pop();
}
void Permutation_Collection::Push(SinglePermutation *pPermutation) {
  // permutations_mutex_.lock(); - not needed as call only from CheckAddPartialPermutationGreaterThanMinCost
  _permutations.push(pPermutation);
  Logger::LogDebugInfo("Pushed: ", pPermutation);
  permns_map[pPermutation->permutation] += "Pushed:";
  // permutations_mutex_.unlock();
}
void Permutation_Collection::Empty(bool &bPermnsEmpty) {
  lock_guard lg(_permutations_mutex);
  bPermnsEmpty = _permutations.empty();
}
permutations_ct_t Permutation_Collection::GetAvoidedPermutationsCount() {
  permutations_ct_t total_avoided_permutations_ct = 0;
  for (auto ct : avoided_permutations) {
    permutations_ct_t prod = 1;
    node_id_t length = total_nodes - ct;
    for (node_id_t i = 2; i <= length; i++) prod *= i;
    total_avoided_permutations_ct += prod;
  }
  return total_avoided_permutations_ct;
}

void Permutation_Collection::OutputMinCostPermutation() {
  Logger::LogInfo("Final Min Cost Permn: ", &min_cost_permutation);
  Logger::LogInfo("Initial Min Cost Permn: ", &initial_min_cost_permn_);
  Logger::LogInfo("Difference: ", initial_min_cost_permn_.permutation_cost -
                                      min_cost_permutation.permutation_cost);
  Logger::LogInfo("----------------------------------------------------------");
  Logger::LogInfo(
      "------------- Processed Permns -------------------------------");
  cout << "Processed Permns count: " << processed_permns.size() << endl;
  for (auto &permn : processed_permns) cout << permn << endl;
  Logger::LogInfo("----------------------------------------------------------");
  Logger::LogInfo(
      "------------- Avoided Permns -------------------------------");
  cout << "Avoided Permns count: " << avoided_permns.size() << endl;
  for (auto &permn : avoided_permns) cout << permn << endl;
  Logger::LogInfo("----------------------------------------------------------");
}

// Convert time
void tsp_Utils::Convert_Time_Duration(long long Duration) {
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