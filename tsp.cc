// tsp.cpp 
// Author : SUNILKUMAR AV
// SUNILKUMAR.PROG@gmail.com
// C++ 11
// Intel(R) Core(TM) i5-8265U CPU @1.60 GHz @1.80 GHz
// 11/18/2019, version : 1.0
// 
// 
#include "tsp.h"


multimap<edge_cost_t, Edge> tsp_edges; // all edges
AvoidedPermnPrefixes avoided_permn_prefixes;
permutation_ct_t total_processed_permns_ct_ = 0;
Permutations permutations;
Permutation initial_min_cost_permutation, best_min_cost_permutation; // min cost paths
vector<ulint> nodes_list{ 10   , 20, 50, 100, 500, 1000, 10000, 50000, 100000, 1000000 };
edge_cost_t** pSorted_nbr_cost;  // two dim array indexed by two nodes to get cost of edge between them
atomic<long long int> thread_count{};
void find_initial_min_cost_permutation()
{
	initial_min_cost_permutation.permutation.push_back(0);	// start with node 0
	node_ct_t node_ct = 0;
	node_id_t cur_id = 0;	
	do
	{
		std::cout << "cur_node: " << cur_id;
		auto elit = find_if(tsp_edges.begin(), tsp_edges.end(),
			[=](const pair<edge_cost_t, Edge>& ce) {if (ce.second.id2 == cur_id || ce.second.id1 == cur_id) return true; });
		if (elit == tsp_edges.end())
		{
			ostringstream oss;
			oss << "Neighbor for " << cur_id << " not found in edges";
			throw tsp_exception(oss.str().c_str());
		}
		initial_min_cost_permutation.permutation_cost += elit->first;
		if (elit->second.id1 == cur_id) cur_id = elit->second.id2;
		else cur_id = elit->second.id1;
		initial_min_cost_permutation.permutation.push_back(cur_id);
		node_ct++;
	} while (node_ct < total_nodes);	
}
int main()
{

	set_terminate(term_func);	
	for (int cur_num_nodes = 0; cur_num_nodes < nodes_list.size(); ++cur_num_nodes)
	{		
		tsp_edges.clear();
		try
		{
			total_nodes = nodes_list[cur_num_nodes];
			cout << "Total nodes: " << total_nodes << endl;

			Initialize();
			ulint tsp_edge_ct = 0;
			pSorted_nbr_cost = (edge_cost_t**)malloc(sizeof(edge_cost_t*) * total_nodes);
			for (int i = 0; i < total_nodes; i++)
				pSorted_nbr_cost[i] = (edge_cost_t*)malloc(sizeof(edge_cost_t) * total_nodes);

			for (auto elit = tsp_edges.begin(); elit != tsp_edges.end(); ++elit)
			{
				node_id_t id1 = elit->second.id1;
				node_id_t id2 = elit->second.id2;
				edge_cost_t cost = elit->first;

				pSorted_nbr_cost[elit->second.id1][elit->second.id2] = elit->first;
				pSorted_nbr_cost[elit->second.id2][elit->second.id1] = elit->first;

				tsp_edge_ct++;
			}
			std::cout << "Count of Edges: " << tsp_edge_ct << endl;

			find_initial_min_cost_permutation();
			cout << "Initial Min Path: " << initial_min_cost_permutation;

			// Start processing permutations
			auto start_time = steady_clock::now();

			Permutation* pPermn1 = new Permutation({ 0,1 },pSorted_nbr_cost[0][1]);
			Permutation* pPermn2 = new Permutation({ 1,0 }, pSorted_nbr_cost[0][1]);
			permutations.Push(pPermn1); permutations.Push(pPermn2);

			long long thread_ct = 1;
			do
			{
				bool bPermnsEmpty = true;
				permutations.Empty(bPermnsEmpty);
				while (!bPermnsEmpty)
				{
					GenerateThreads();
					permutations.Empty(bPermnsEmpty);
				}

			// Wait for all threads to finish			
				thread_ct = thread_count.fetch_add(0);
				this_thread::sleep_for(microseconds(10));
			} while (thread_ct);

			cout << "Best Min Cost Path: " << best_min_cost_permutation;

			
			permutation_ct_t avoided_permn_prefix_ct = 0;
			permutation_ct_t avoided_permns_ct = avoided_permn_prefixes.Count(avoided_permn_prefix_ct);
			cout << "Processed Permutations: " << total_processed_permns_ct_ << endl;
			cout << "Avoided Permutation prefixes : " << avoided_permn_prefix_ct << endl;
			cout << "Avoided Permutations: " << avoided_permns_ct << endl;
			cout << "Total permutations: " << total_processed_permns_ct_ + avoided_permns_ct << endl;
			permutation_ct_t prod = 1;
			for (ulint i = 2; i <= total_nodes; ++i)
				prod *= i;

			cout << "Total Expected Permutations: " << prod << endl;
			cout << "Difference: " << prod - (total_processed_permns_ct_ + avoided_permns_ct) << endl;

			auto end_time = steady_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
			std::cout << "Time to test permutations: ";
			Convert_Time_Duration(duration);
			cout << endl;
			std::cout << "-----------------------------------" << endl;

			for (int i = 0; i < total_nodes; i++)
				free(pSorted_nbr_cost[i]);
			free(pSorted_nbr_cost);
		}
		catch (bad_alloc& ba)
		{
			cout << ba.what() << endl;
		}
		catch (tsp_exception& tspe)
		{
			cout << tspe.what() << endl;
		}
		catch (exception& e)
		{
			cout << e.what() << endl;
		}
		catch (...)
		{
			cout << "Unknown error occurred..." << endl;
		}		
	}
	exit(EXIT_SUCCESS);
}
void GenerateThreads()
{
	Permutation permutation;
	try
	{
		permutations.Pop(permutation);
	}
	catch (tsp_exception & te)
	{
		Logger::DebugLogMesg(te.what()); return;
	}
	bool bThreadCreated = false;
	permutation_cost_t permn_cost = permutation.permutation_cost;
	permutation_t oldPermn{ permutation.permutation };
	while (!bThreadCreated)
	{
		
		Logger::DebugLogMesg(". ");
		try
		{
			thread* pt = new thread(generate_and_test_permutations, permn_cost, oldPermn);
			++thread_count;
			bThreadCreated = true;
		}
		catch (system_error & se)
		{
			Logger::DebugLogMesg(se.what());
			bThreadCreated = false;
		}
		catch (...)
		{
			Logger::DebugLogMesg("Unknown error occurred while creating new thread\n");
			bThreadCreated = false;
		}
	}	
}
void generate_and_test_permutations(permutation_cost_t cur_cost, permutation_t oldPermn)
{	
	Logger::DebugLogMesg("generate_and_test_permutations: ", cur_cost, oldPermn);
	Logger::DebugLogMesg(". "); 

	if (oldPermn.size() >= total_nodes)
		throw tsp_exception("Size of Permutation to process > NUM_NODES");

	node_id_t next_id = oldPermn.size();
	permutation_t newPermn{ oldPermn };
	newPermn.insert(newPermn.begin(), next_id);
	Logger::DebugLogMesg("Next Permn: ", newPermn);

	permutation_cost_t newCost = pSorted_nbr_cost[next_id][oldPermn.front()];
	newCost += cur_cost;
	CheckNewPermn(newPermn, newCost);
	 
	
	int pos = 1;
	for (auto oit = oldPermn.begin() + 1; oit < oldPermn.end() - 1; ++oit)
	{
		permutation_t newPermn{ oldPermn };
		auto it = newPermn.begin() + pos;
		node_id_t cur_node = *it;
		node_id_t next_node = *(it + 1);
		newPermn.insert(it, next_id);
		Logger::DebugLogMesg("Next Permn: ", newPermn);	

		permutation_cost_t cur_edge_cost = pSorted_nbr_cost[cur_node][next_node];
		permutation_cost_t new_edge_cost_1 = pSorted_nbr_cost[cur_node][next_id];
		permutation_cost_t new_edge_cost_2 = pSorted_nbr_cost[next_id][next_node];
		permutation_cost_t newCost = cur_cost - cur_edge_cost + new_edge_cost_1 + new_edge_cost_2;
		CheckNewPermn(newPermn, newCost);

		++pos;
	}
	newPermn = oldPermn;
	node_id_t last_node = oldPermn.back();
	newPermn.push_back(next_id);	
	Logger::DebugLogMesg("Next Permn: ", newPermn);
	
	newCost = pSorted_nbr_cost[next_id][last_node];
	newCost += cur_cost;
	CheckNewPermn(newPermn, newCost);
	
	--thread_count;
}
mutex checkNewPermn_mutex;
void CheckNewPermn(permutation_t& newPermn, permutation_cost_t& newCost)
{
	checkNewPermn_mutex.lock();	
	if (avoided_permn_prefixes.Exists(newPermn)) {
		checkNewPermn_mutex.unlock();
		return;
	}

	Logger::DebugLogMesg("CheckNewPermn: ", newCost, newPermn);	
	size_t newPermn_sz = newPermn.size();
	if (newPermn_sz == total_nodes)
		++total_processed_permns_ct_;
	if (best_min_cost_permutation.permutation_cost > newCost && newPermn_sz == total_nodes)
	{
		best_min_cost_permutation.permutation_cost = newCost;
		best_min_cost_permutation.permutation = newPermn;
		Logger::DebugLogMesg("CheckNewPermn: Min Cost Permutation Changed");
		checkNewPermn_mutex.unlock();
		return;
	}
	else if (best_min_cost_permutation.permutation_cost <= newCost && newPermn_sz < total_nodes)
	{		
		Logger::DebugLogMesg("CheckNewPermn: Avoiding permn");
		avoided_permn_prefixes.AddPermnPrefix(newPermn);
		checkNewPermn_mutex.unlock();
		return;
	}
	else if (newPermn_sz < total_nodes)
	{
		Permutation* pPermn = new Permutation(newPermn, newCost);
		permutations.Push(pPermn);		
		Logger::DebugLogMesg("CheckNewPermn: Pushed New Permn");
	}
	checkNewPermn_mutex.unlock();
}
void Initialize()
{
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist6(1, ULONG_MAX); // distribution in range [1, ULLONG_MAX]

	for (ulint id = 0; id < total_nodes; id++)
	{
		for (ulint next_id = id + 1; next_id < total_nodes; next_id++)
		{
			ulint cost = dist6(rng);
			tsp_edges.insert(make_pair(cost, *new Edge(id, next_id, cost)));
		}
	}	
	cout << "Initialized" << endl;
}
void Convert_Time_Duration(long long Duration)
{
	double milli_seconds = 0.0, seconds = 0.0, minutes = 0.0, hours = 0.0;
	if (Duration < 1000)
	{
		cout << Duration << " Microseconds";
		return;
	}
	else
		milli_seconds = Duration / 1000.0;
	if (milli_seconds < 1000.0)
	{
		cout << milli_seconds << " Milliseconds";
		return;
	}
	else
		seconds = milli_seconds / 1000.0;
	if (seconds < 60.0)
	{
		cout << seconds << " Seconds";
		return;
	}
	else
		minutes = seconds / 60.0;
	if (minutes < 60.0)
	{
		cout << minutes << " Minutes";
		return;
	}
	else
		hours = minutes / 60.0;
	cout << hours << " Hours";
}
