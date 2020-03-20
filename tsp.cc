// tsp.cpp
// Author : SUNILKUMAR AV
// SUNILKUMAR.PROG@gmail.com
// C++ 11
// Intel(R) Core(TM) i5-8265U CPU @1.60 GHz @1.80 GHz
// 11/18/2019, version : 1.0
//
//

#include "tsp.h"

/*****************************************************************************************************************************/

///                          CODE IS NOT YET DOCUMENTED/COMMENTED, as DESIGN NOT
///                          YET FINALIZED

/*****************************************************************************************************************************/

Permutations *pPermutations = nullptr;
Permutation min_cost_permutation; // min cost permn
vector<ulint> nodes_list{10, 15, 20, 25, 30, 35, 40, 45, 50};
//, 100, 500
//, 1000, 10000, 50000, 100000, 1000000};
edge_cost_t **pEdge_Cost; // two dim array indexed by two nodes to get cost of
                          // edge between them
atomic<long long int> thread_count{};

struct cost_excess_cost {
  node_id_t _nbr_id;
  edge_cost_t _cost;
  edge_cost_t _excess_cost;
  cost_excess_cost(node_id_t nbr_id = 0, edge_cost_t cost = 0,
                   edge_cost_t excess_cost = 0)
      : _nbr_id(nbr_id), _cost(cost), _excess_cost(excess_cost) {}
  cost_excess_cost(const cost_excess_cost &rhs)
      : _nbr_id(rhs._nbr_id), _cost(rhs._cost), _excess_cost(rhs._excess_cost) {
  }
};
auto cost_excess_cost_cmp = [](const cost_excess_cost &ces1,
                               const cost_excess_cost &ces2) {
  return ces1._excess_cost < ces2._excess_cost;
};

vector<vector<struct cost_excess_cost>> node_cost_excess_cost;
vector<node_id_t> min_cost_nbr;
void Initialize_Edges() {
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist6(
      1, MAX_EDGE_COST); // distribution in range [1, ULLONG_MAX]

  ulint edge_ct = 0;
  for (node_id_t id = 0; id < total_nodes; id++) {
    for (node_id_t next_id = id + 1; next_id < total_nodes; next_id++) {
      edge_cost_t cost = dist6(rng);
      pEdge_Cost[id][next_id] = pEdge_Cost[next_id][id] = cost;
      ++edge_ct;
    }
  }
  cout << "Edges initialized: " << edge_ct << endl;
}

class MinimalPermutationComputerizer {
  inline static Permutation min_permutation;
  inline static vector<vector<struct cost_excess_cost>> node_cost_excess_cost;
  inline static vector<node_id_t> min_cost_nbr;
  inline static node_ct_t consumed_nodes = 0;

public:
  MinimalPermutationComputerizer(edge_cost_t **pEdgeCost) {
    min_cost_nbr = *new vector<node_id_t>(total_nodes);
    node_cost_excess_cost = *new vector<vector<cost_excess_cost>>(total_nodes);
    for (node_id_t id = 0; id < total_nodes; ++id)
      node_cost_excess_cost[id] = *new vector<cost_excess_cost>(total_nodes);

    for (node_id_t id = 0; id < total_nodes; id++) {
      for (node_id_t next_id = 0; next_id < total_nodes; next_id++) {
        if (id == next_id) {
          node_cost_excess_cost[id][next_id] = id;
          node_cost_excess_cost[id][next_id]._cost = ULONG_MAX;
          node_cost_excess_cost[id][next_id]._excess_cost = ULONG_MAX;
          continue;
        }
        node_cost_excess_cost[id][next_id]._nbr_id = next_id;
        node_cost_excess_cost[id][next_id]._cost = pEdgeCost[id][next_id];
      }
    }

    min_permutation.permutation.clear();
    min_permutation.permutation.push_back(0);
    consumed_nodes = 1;
    min_permutation.permutation_cost = 0;

    // Find min cost nbr
    for (node_id_t id = 0; id < total_nodes; id) {
      edge_cost_t min_cost = ULONG_MAX;
      for (node_id_t next_id = 0; next_id < total_nodes; ++next_id) {
        if (id == next_id)
          continue;

        if (min_cost > pEdge_Cost[id][next_id]) {
          min_cost = pEdge_Cost[id][next_id];
          min_cost_nbr[id] = next_id;
        }
      }
    }
    // Compute excess cost vector
    for (node_id_t id = 0; id < total_nodes; id) {
      for (node_id_t next_id = 0; next_id < total_nodes; ++next_id) {
        if (id == next_id)
          continue;
        node_cost_excess_cost[id][next_id]._excess_cost =
            node_cost_excess_cost[id][next_id]._cost -
            pEdge_Cost[id][min_cost_nbr[id]];
      }

      sort(node_cost_excess_cost[id].begin(), node_cost_excess_cost[id].end(),
           [](const cost_excess_cost &lhs, const cost_excess_cost &rhs) {
             return lhs._cost < rhs._cost;
           });
    }
  }
  static void compute_min_cost_permutation() {
    node_id_t last_id = min_permutation.permutation.back();
    node_id_t nearest_nbr = min_cost_nbr[last_id];
    list<node_id_t> other_similar_nbrs;
    for (node_id_t id = 0; id < total_nodes; ++id) {
      if (id == nearest_nbr)
        continue;
      if (check_node_consumed(id))
        continue;
      if (min_cost_nbr[id] == nearest_nbr)
        other_similar_nbrs.push_back(id);
    }
    if (other_similar_nbrs.empty()) {
      min_permutation.permutation.push_back(nearest_nbr);
      min_permutation.permutation_cost += pEdge_Cost[last_id][nearest_nbr];
    }
    int cur_pos = 1;
    edge_cost_t last_id_excess_cost =
        (node_cost_excess_cost[last_id].begin() + cur_pos)->_excess_cost;
    vector<pair<node_id_t, edge_cost_t>> similar_nbr_excess_cost(
        other_similar_nbrs.size());
    similar_nbr_excess_cost.push_back(make_pair(last_id, last_id_excess_cost));
    int pos = 0;
    for (auto it = other_similar_nbrs.begin(); it != other_similar_nbrs.begin();
         ++pos, ++it) {
      node_id_t cur_id = *it;
      similar_nbr_excess_cost[pos].first = cur_id;
      similar_nbr_excess_cost[pos].second =
          (node_cost_excess_cost[cur_id].begin() + cur_pos)->_excess_cost;
    }
  }
  static bool check_node_consumed(node_id_t id) {
    for (int i = 0; i < consumed_nodes; i++)
      if (min_permutation.permutation[i] == id)
        return true;
    return false;
  }
};

void compute_min_nbrs(node_id_t **pEdgeCost) {
  if (min_cost_nbr.size() != total_nodes)
    min_cost_nbr = *(new vector<node_id_t>(total_nodes));

  // Find min cost nbr
  for (node_id_t id = 0; id < total_nodes; id) {
    edge_cost_t min_cost = ULONG_MAX;
    for (node_id_t next_id = 0; next_id < total_nodes; ++next_id) {
      if (id == next_id)
        continue;

      if (min_cost > pEdge_Cost[id][next_id]) {
        min_cost = pEdge_Cost[id][next_id];
        min_cost_nbr[id] = next_id;
      }
    }
  }
}
class optimize_min_permutation {
private:
  Permutation min_permn;
  permutation_cost_t initial_excess_cost = 0;

public:
  optimize_min_permutation(const Permutation &initial_min_cost_permutation)
      : min_permn(initial_min_cost_permutation) {
    compute_min_nbrs(pEdge_Cost);
  }
  void optimize_permutation_cost() {
    vector<edge_cost_t> excess_cost = *(new vector<edge_cost_t>(total_nodes));
    for (node_id_t id = 0; id < total_nodes - 1; ++id) {
      excess_cost[id] =
          pEdge_Cost[id][id + 1] - pEdge_Cost[id][min_cost_nbr[id]];
      initial_excess_cost += excess_cost[id];
    }

    for_each(
        excess_cost.begin(), excess_cost.end(),
        [this](const edge_cost_t &cost) { this->initial_excess_cost += cost; });
    if (initial_excess_cost == 0) {
      cout << "Optimized cost: " << endl;
      return;
    }
  }
};
void compute_initial_min_cost_permutation() {
  min_cost_permutation.permutation_cost = 0;
  min_cost_permutation.permutation.clear();
  node_id_t id = 0;
  min_cost_permutation.permutation.push_back(0); // start with node 0
  node_ct_t node_ct = 1;
  node_id_t cur_id = 0;
  do {
    edge_cost_t min_cost = MAX_EDGE_COST;
    node_id_t min_nbr = 0;
    for (node_id_t id = 0; id < total_nodes; ++id) {
      if (id == cur_id)
        continue;
      if (min_cost > pEdge_Cost[cur_id][id]) {
        if (find(min_cost_permutation.permutation.begin(),
                 min_cost_permutation.permutation.end(),
                 id) != min_cost_permutation.permutation.end())
          continue;
        min_nbr = id;
        min_cost = pEdge_Cost[cur_id][id];
      }
    }
    min_cost_permutation.permutation_cost += min_cost;
    ++node_ct;
    cur_id = min_nbr;
    min_cost_permutation.permutation.push_back(min_nbr);
  } while (node_ct < total_nodes);
}

int main() {

  set_terminate(tsp_terminate);
  for (int cur_num_nodes = 0; cur_num_nodes < nodes_list.size();
       ++cur_num_nodes) {
    try {
      total_nodes = nodes_list[cur_num_nodes];
      cout << "Total nodes: " << total_nodes << endl;
      //  if (total_nodes <= 50)
      Logger::LogLevel = Logger::loglevel::basic;
      //  else
      // Logger::LogLevel = Logger::loglevel::info;

      pEdge_Cost = new edge_cost_t *[total_nodes];
      for (node_id_t id = 0; id < total_nodes; ++id) {
        pEdge_Cost[id] = new edge_cost_t[total_nodes];
      }

      Initialize_Edges();
      compute_initial_min_cost_permutation();
      cout << "Initial Min Path: Cost: " << min_cost_permutation;
      pPermutations = new Permutations(min_cost_permutation);      

      // Start processing permutations
      auto start_time = steady_clock::now();

      Permutation *pPermn1 = new Permutation(pEdge_Cost[0][1], {0, 1});
      Permutation *pPermn2 = new Permutation(pEdge_Cost[0][1], {1, 0});
      pPermutations->Push(pPermn1);
      pPermutations->Push(pPermn2);

      long long thread_ct = 1;
      do {
        bool bPermnsEmpty = true;
        pPermutations->Empty(bPermnsEmpty);
        while (!bPermnsEmpty) {
          GenerateThreads();
          pPermutations->Empty(bPermnsEmpty);
        }

        // Wait for all threads to finish
        thread_ct = thread_count.fetch_add(0);
        this_thread::sleep_for(milliseconds(1));
      } while (thread_ct);

      cout << "Best Min Path: Cost: ";
      pPermutations->OutputMinCostPermutation();
      
      permutation_ct_t avoided_permns_ct =
          pPermutations->GetAvoidedPermutationsCount();
      cout << "Processed Permutations: "
           << pPermutations->total_processed_permns_ct_ << endl;   
      cout << "Avoided Permutations: " << avoided_permns_ct << endl;
      cout << "Total permutations: "
           << pPermutations->total_processed_permns_ct_ + avoided_permns_ct
           << endl;
      permutation_ct_t prod = 1;
      for (ulint i = 2; i <= total_nodes; ++i)
        prod *= i;

      cout << "Total Expected Permutations: " << prod << endl;
      cout << "Difference: "
           << prod - (pPermutations->total_processed_permns_ct_ +
                      avoided_permns_ct)
           << endl;

      auto end_time = steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                          end_time - start_time)
                          .count();
      std::cout << "Time to test permutations: ";
      Convert_Time_Duration(duration);
      cout << endl;
      std::cout << "-----------------------------------" << endl;

      for (int i = 0; i < total_nodes; i++) {
        delete[] pEdge_Cost[i];
      }
      delete[] pEdge_Cost;
      delete pPermutations;

    } catch (system_error &err) {
      cout << "caught system_error " << err.what() << endl; // error message
      auto ec = err.code();
      cout << "category: " << ec.category().name() << " : "
           << ec.category().message(ec.value()) << endl;
      cout << "value: " << ec.value() << endl;
      cout << "message: " << ec.message() << endl;
    } catch (bad_alloc &ba) {
      cout << ba.what() << endl;
      ostringstream oss;
      oss << "Need file storage: " << total_nodes << endl;
      Logger::LogCriticalError(oss.str());
      tsp_exit(0);
    } catch (tsp_exception &tspe) {
      cout << tspe.what() << endl;
    } catch (exception &e) {
      cout << e.what() << endl;
    } catch (...) {
      cout << "Unknown error occurred..." << endl;
    }
  }

  tsp_exit(EXIT_SUCCESS);
}

void GenerateThreads() {
  bool bPermnsEmpty = false;
  while (!bPermnsEmpty) {

    SinglePermutation *pPermutation = new SinglePermutation();
    try {
      pPermutations->Pop(pPermutation);
    } catch (tsp_exception &te) {
      Logger::LogInfo(te.what());
      continue;
    }

    // Allow only max 10 current threads
    long long thread_ct = thread_count.fetch_add(0);
    while (abs(thread_ct) > 10) {
      this_thread::sleep_for(microseconds(10));
      thread_ct = thread_count.fetch_add(0);
    }  

    bool bThreadCreated = false;
    while (!bThreadCreated) {

      Logger::LogDebugInfo(". ");
      try {
        thread *pt = new thread(generate_and_test_permutations, pPermutation);
        ++thread_count;
        bThreadCreated = true;
      } catch (bad_alloc &ba) {
        cout << ba.what() << endl;
        ostringstream oss;
        oss << ba.what() << ": Need file storage: " << total_nodes << endl;
        Logger::LogCriticalError(oss.str());
        tsp_exit(0);
      } catch (system_error &err) {
        process_system_error(err);
      } catch (exception &e) {
        cout << e.what() << endl;       
      } catch (...) {
        Logger::LogInfo("Unknown error occurred while creating new thread\n");        
      }
    }
    pPermutations->Empty(bPermnsEmpty);
  }
 
}

void generate_and_test_permutations(SinglePermutation *pPermutation) {
  Logger::LogDebugInfo("generate_and_test_permutations: ", pPermutation);
  Logger::LogDebugInfo(". ");
  expected_permutations_to_generate +=
      factorial(total_nodes - pPermutation->permutation.size());

  permutation_t oldPermn(pPermutation->permutation);
  permutation_cost_t cur_cost(pPermutation->permutation_cost);
  if (pPermutation->permutation.size() >= total_nodes)
    throw tsp_exception("Size of Permutation to process > NUM_NODES");

  node_id_t next_id = oldPermn.size();
  permutation_t newPermn{oldPermn};
  newPermn.insert(newPermn.begin(), next_id);
  Logger::LogDebugInfo("Next Permn: ", newPermn);
  ++partial_permutations_generated;
  permutation_cost_t newCost = pEdge_Cost[next_id][oldPermn.front()];
  newCost += cur_cost;
  CheckNewPermutation(newPermn, newCost);

  ullint pos = 1;
  for (auto oit = oldPermn.begin() + 1; oit < oldPermn.end() - 1; ++oit) {
    permutation_t newPermn{oldPermn};
    auto it = newPermn.begin() + pos;
    node_id_t cur_node = *it;
    node_id_t next_node = *(it + 1);
    newPermn.insert(it, next_id);
    ++partial_permutations_generated;
    Logger::LogDebugInfo("Next Permn: ", newPermn);

    permutation_cost_t cur_edge_cost = pEdge_Cost[cur_node][next_node];
    permutation_cost_t new_edge_cost_1 = pEdge_Cost[cur_node][next_id];
    permutation_cost_t new_edge_cost_2 = pEdge_Cost[next_id][next_node];
    permutation_cost_t newCost =
        cur_cost - cur_edge_cost + new_edge_cost_1 + new_edge_cost_2;
    CheckNewPermutation(newPermn, newCost);

    ++pos;
  }
  newPermn = oldPermn;
  node_id_t last_node = oldPermn.back();
  newPermn.push_back(next_id);
  ++partial_permutations_generated;
  Logger::LogDebugInfo("Next Permn: ", newPermn);

  newCost = pEdge_Cost[next_id][last_node];
  newCost += cur_cost;
  CheckNewPermutation(newPermn, newCost);

  --thread_count;
}

void CheckNewPermutation(permutation_t &newPermn, permutation_cost_t &newCost) {
  
  Logger::LogDebugInfo("CheckNewPermn: ", newPermn, newCost);
  size_t newPermn_sz = newPermn.size();
  SinglePermutation *pPermutation = new SinglePermutation(newCost, newPermn);
  if (newPermn_sz == total_nodes)
    pPermutations->CheckSetMinCostPermutation(pPermutation);
  else
    pPermutations->CheckAddPartialPermutationGreaterThanMinCost(pPermutation);
  return;
}


