struct cost_excess_cost {
  node_id_t _id;
  node_id_t _cur_processing_id;
  node_id_t _cur_processing_nbr_id;
  node_id_t _cur_alternate_nbr_id;
  edge_cost_t _excess_cost;
  cost_excess_cost(node_id_t id, node_id_t cur_processing_id,
                   node_id_t cur_processing_nbr_id,
                   node_id_t cur_alternate_nbr_id, edge_cost_t excess_cost)
      : _id(id), _cur_processing_id(cur_processing_id),
        _cur_processing_nbr_id(cur_processing_nbr_id),
        _cur_alternate_nbr_id(cur_alternate_nbr_id) {}
  cost_excess_cost(const cost_excess_cost &rhs)
      : _id(rhs._id), _cur_processing_id(rhs._cur_processing_id),
        _cur_processing_nbr_id(rhs._cur_processing_nbr_id),
        _cur_alternate_nbr_id(rhs._cur_alternate_nbr_id) {}
};

struct nbr_cost {
  node_id_t _id;
  edge_cost_t _cost;
  nbr_cost(node_id_t id, edge_cost_t cost) : _id(id), _cost(cost) {}
  nbr_cost(const nbr_cost &rhs) : _id(rhs._id), _cost(rhs._cost) {}
};
auto nbr_cost_cmp = [](const nbr_cost &nc1, const nbr_cost &nc2) {
  return nc1._cost < nc2._cost;
};



class MinimalPermutationComputerizer {
  inline static SinglePermutation min_permutation;
  inline static node_ct_t consumed_nodes = 0;
  inline static edge_cost_t min_edge_cost = 0;
  inline static vector<node_id_t> min_cost_nbr;
  inline static vector<vector<nbr_cost>> sorted_nbr_costs;

public:
  MinimalPermutationComputerizer(edge_cost_t **pEdgeCost) {
    sorted_nbr_costs = *new vector<vector<nbr_cost>>(total_nodes);

    min_permutation.permutation.clear();
    min_permutation.permutation.push_back(0);
    consumed_nodes = 1;
    min_permutation.permutation_cost = 0;

    // Find min edge cost & deduct from all edge costs
    min_edge_cost = pEdge_Cost[0][1];
    for (node_id_t id = 0; id < total_nodes; id) {
      for (node_id_t next_id = 0; next_id < total_nodes; ++next_id) {
        if (id == next_id)
          continue;
        if (min_edge_cost > pEdge_Cost[id][next_id])
          min_edge_cost = pEdge_Cost[id][next_id];
      }
    }
    for (node_id_t id = 0; id < total_nodes; id) {
      for (node_id_t next_id = 0; next_id < total_nodes; ++next_id) {
        if (id == next_id)
          continue;
        pEdge_Cost[id][next_id] -= min_edge_cost;
      }
    }
    /*
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
     */
    for (node_id_t id = 0; id < total_nodes; id) {
      for (node_id_t next_id = 0; next_id < total_nodes; ++next_id) {
        if (id == next_id) {
          sorted_nbr_costs[id].push_back(nbr_cost(next_id, MAX_EDGE_COST));
          sorted_nbr_costs[next_id].push_back(nbr_cost(id, MAX_EDGE_COST));
        }
        sorted_nbr_costs[next_id].push_back(
            nbr_cost(id, pEdge_Cost[id][next_id]));
      }
      sort(sorted_nbr_costs[id].begin(), sorted_nbr_costs[id].end(),
           nbr_cost_cmp);
    }
  }
  static void set_min_permutation(node_id_t from, node_id_t to) {
    min_permutation.permutation.push_back(to);
    min_permutation.permutation_cost += pEdge_Cost[from][to];
  }
  static node_id_t
  Find_next_nearest_nbr_to_process(node_id_t id,
                                   vector<node_id_t> &processed_nbrs) {
    for (node_id_t nbr = 0; nbr < sorted_nbr_costs[id].size(); ++nbr) {
      if (is_node_consumed(nbr))
        continue;
      if (find(processed_nbrs.begin(), processed_nbrs.end(), nbr) !=
          processed_nbrs.end())
        continue;
      return nbr;
    }
  }

  static void compute_min_cost_permutation() {
    do {

      node_id_t last_id = min_permutation.permutation.back();
      bool bNbrFixed = false;
      vector<node_id_t> processed_nbrs;
      vector<vector<const cost_excess_cost *>> processed_costs;

      while (!bNbrFixed) {
        node_id_t next_nearest_nbr_to_process =
            Find_next_nearest_nbr_to_process(last_id, processed_nbrs);

        list<node_id_t> other_similar_nbrs;
        for (node_id_t id = 0; id < total_nodes; ++id) {
          if (id == next_nearest_nbr_to_process)
            continue;
          if (is_node_consumed(id))
            continue;
          if (next_non_consumed_nbr(id) == next_nearest_nbr_to_process)
            other_similar_nbrs.push_back(id);
        }
        node_id_t cur_nbr_being_processed = next_nearest_nbr_to_process;
        processed_nbrs.push_back(next_nearest_nbr_to_process);
        next_nearest_nbr_to_process =
            Find_next_nearest_nbr_to_process(last_id, processed_nbrs);
        processed_nbrs.pop_back();

        edge_cost_t last_id_next_excess_cost =
            sorted_nbr_costs[last_id][next_nearest_nbr_to_process]._excess_cost;
        vector<const cost_excess_cost *> similar_nbr_excess_cost(
            other_similar_nbrs.size());
        similar_nbr_excess_cost.push_back(
            &node_cost_excess_cost[last_id][cur_nbr_being_processed]);
        int pos = 0;
        for (auto id : other_similar_nbrs) {
          cost_excess_cost *pCost_excess_cost =
              new cost_excess_cost(id, pEdge_Cost)
        }
        similar_nbr_excess_cost.push_back(
            &node_cost_excess_cost[id][cur_nbr_being_processed]);

        sort(similar_nbr_excess_cost.begin(), similar_nbr_excess_cost.end(),
             [](const cost_excess_cost *p1, const cost_excess_cost *p2) {
               return p1->_excess_cost > p2->_excess_cost;
             });
        if (similar_nbr_excess_cost[0]->_nbr_id == last_id) {
          set_min_permutation(last_id, cur_nbr_being_processed);
          bNbrFixed = true;
        }
        processed_costs.push_back(similar_nbr_excess_cost);
        processed_nbrs.push_back(cur_nbr_being_processed);
      }
    } while (min_permutation.permutation.size() < total_nodes);
  }
  static bool is_node_consumed(node_id_t id) {
    for (int i = 0; i < min_permutation.permutation.size(); ++i)
      if (min_permutation.permutation[i] == id)
        return true;
    return false;
  }
  static node_id_t next_non_consumed_nbr(node_id_t id) {
    node_id_t next_nearest_nbr;
    for_each(node_cost_excess_cost[id].begin(), node_cost_excess_cost[id].end(),
             [=, &next_nearest_nbr](const cost_excess_cost &nc) {
               if (nc._nbr_id != id && !is_node_consumed(nc._nbr_id))
                 next_nearest_nbr = nc._nbr_id;
             });
    return next_nearest_nbr;
  }
};

/*
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
*/
class optimize_min_permutation {
private:
  SinglePermutation min_permn;
  permutation_cost_t initial_excess_cost = 0;

public:
  optimize_min_permutation(
      const SinglePermutation &initial_min_cost_permutation)
      : min_permn(initial_min_cost_permutation) {
    // compute_min_nbrs(pEdge_Cost);
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