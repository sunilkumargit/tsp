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

ulint total_nodes = 0; // total number of nodes

Permutation_Collection *pPermutations = nullptr;
SinglePermutation min_cost_permutation; // min cost permn
vector<ulint> nodes_list{5};
//, 10, 15, 20, 25, 30, 35, 40, 45, 50};
//, 100, 500
//, 1000, 10000, 50000, 100000, 1000000};
edge_cost_t **pEdge_Cost; // two dim array indexed by two nodes to get cost of
                          // edge between them
atomic<long int> thread_count{};

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
void Initialize_Edges() {
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist6(
      1, MAX_EDGE_COST); // distribution in range [1, ULONG_MAX]

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

map<permutation_t, string> permns_map;
int main() {

  set_terminate(tsp_terminate);
  for (int cur_num_nodes = 0; cur_num_nodes < nodes_list.size();
       ++cur_num_nodes) {
    try {
      total_nodes = nodes_list[cur_num_nodes];
      cout << "Total nodes: " << total_nodes << endl;
      if (total_nodes > 20)
        Logger::logLevel = Logger::Loglevel::basic;
      else
        Logger::logLevel = Logger::Loglevel::debug;

      pEdge_Cost = new edge_cost_t *[total_nodes];
      for (node_id_t id = 0; id < total_nodes; ++id)
        pEdge_Cost[id] = new edge_cost_t[total_nodes];

      Initialize_Edges();
      compute_initial_min_cost_permutation();
      cout << "Initial Min Path: Cost: " << min_cost_permutation;
      pPermutations = new Permutation_Collection(min_cost_permutation);

      // Start processing permutations
      auto start_time = steady_clock::now();

      SinglePermutation *pPermn1 =
          new SinglePermutation(pEdge_Cost[0][1], {0, 1});
      SinglePermutation *pPermn2 =
          new SinglePermutation(pEdge_Cost[0][1], {1, 0});
      pPermutations->Push(pPermn1);
      pPermutations->Push(pPermn2);
      permns_map[pPermn1->permutation] = "Generated:";
      permns_map[pPermn2->permutation] = "Generated:";
#ifdef SINGLE_THREAD
      ProcessPermutations();
#else

      long thread_ct = 1;
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

      this_thread::sleep_for(seconds(3));
      bool bPermnsEmpty = true;
      pPermutations->Empty(bPermnsEmpty);
      if (!bPermnsEmpty) {
        int i = 1;
      }
#endif
      cout << "Best Min Path: Cost: ";
      pPermutations->OutputMinCostPermutation();
      cout << " ---------------------------------------------------------------"
              "---"
           << endl;
      cout << "-------------------Permutation Prefix status "
              "-----------------------------------"
           << endl;
      for (auto val : permns_map)
        cout << (permutation_t)val.first << " : " << val.second << endl;
      cout << " -------------------------------------------------------"
           << endl;

      permutations_ct_t avoided_permns_ct =
          pPermutations->GetAvoidedPermutationsCount();
      cout << "Processed Permutations: "
           << pPermutations->total_processed_permns_ct_ << endl;
      cout << "Avoided Permutations: " << avoided_permns_ct << endl;
      cout << "Total permutations: "
           << pPermutations->total_processed_permns_ct_ + avoided_permns_ct
           << endl;
      permutations_ct_t prod = tsp_Utils::factorial(total_nodes);
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
      tsp_Utils::Convert_Time_Duration(duration);
      cout << endl;
      std::cout << "-----------------------------------" << endl;

      for (int i = 0; i < total_nodes; i++) {
        delete[] pEdge_Cost[i];
      }
      delete[] pEdge_Cost;
      delete pPermutations;

    } catch (system_error &err) {
      tsp_Utils::process_system_error(err);
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
void ProcessPermutations() {
  bool bPermnsEmpty = false;
  while (!bPermnsEmpty) {

    SinglePermutation *pPermutation = new SinglePermutation();
    try {
      pPermutations->Pop(pPermutation);
      generate_and_test_permutations(pPermutation);
    } catch (tsp_exception &te) {
      Logger::LogCriticalError(te.what());
      continue;
    }
    pPermutations->Empty(bPermnsEmpty);
  }
}
void GenerateThreads() {
  bool bPermnsEmpty = false;
  while (!bPermnsEmpty) {

    SinglePermutation *pPermutation = new SinglePermutation();
    try {
      pPermutations->Pop(pPermutation);
    } catch (tsp_exception &te) {
      Logger::LogCriticalError(te.what());
      continue;
    }

    // Allow only max 10 current threads
    long long thread_ct = thread_count.fetch_add(0);
    while (abs(thread_ct) > 10) {
      this_thread::sleep_for(microseconds(10));
      thread_ct = thread_count.fetch_add(0); // #TODO
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
        tsp_Utils::process_system_error(err);
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

  permutation_t oldPermn(pPermutation->permutation);
  permutation_cost_t cur_cost(pPermutation->permutation_cost);
  if (pPermutation->permutation.size() >= total_nodes)
    throw tsp_exception(__FILE__, __LINE__, __FUNCTION__,
                        "Size of Permutation to process > NUM_NODES");
  vector<node_id_t> other_nodes, nodes;
  for (int i = 0; i < total_nodes; i++)
    if (find(oldPermn.begin(), oldPermn.end(), i) == oldPermn.end())
    other_nodes.push_back(i);
  
  for (auto next_id : other_nodes) {

    //node_id_t next_id = oldPermn.size();
    permutation_t newPermn{oldPermn};
    newPermn.insert(newPermn.begin(), next_id);
    Logger::LogDebugInfo("Next Permn: ", newPermn);
    permns_map[newPermn] = "Generated:";
    //++partial_permutations_generated;
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
      //++partial_permutations_generated;
      Logger::LogDebugInfo("Next Permn: ", newPermn);
      permns_map[newPermn] = "Generated:";

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
    //++partial_permutations_generated;
    Logger::LogDebugInfo("Next Permn: ", newPermn);
    permns_map[newPermn] = "Generated:";

    newCost = pEdge_Cost[next_id][last_node];
    newCost += cur_cost;
    CheckNewPermutation(newPermn, newCost);
  }
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
