// tsp.cpp 
// Author : SUNILKUMAR AV
// SUNILKUMAR.PROG@gmail.com
// C++ 11
// Intel(R) Core(TM) i5-8265U CPU @1.60 GHz @1.80 GHz
// 11/18/2019, version : 1.0
// 
// 

#include "tsp.h"


Permutations *pPermutations = nullptr;
Permutation min_cost_permutation; // min cost permn
vector<ulint> nodes_list{ 10 , 20, 50, 100, 500, 1000, 10000, 50000, 100000, 1000000 };
edge_cost_t** pEdge_Cost;  // two dim array indexed by two nodes to get cost of edge between them
atomic<long long int> thread_count{};
void compute_initial_min_cost_permutation()
{
	min_cost_permutation.permutation_cost = 0;
	node_id_t id = 0;
	min_cost_permutation.permutation.push_back(0);	// start with node 0
	node_ct_t node_ct = 1;
	node_id_t cur_id = 0;		
	do
	{		
		edge_cost_t min_cost = MAX_EDGE_COST;
		node_id_t min_nbr = 0;	
		for (node_id_t id = 0; id < total_nodes; ++id)
		{
			if (id == cur_id) continue;
			if (min_cost > pEdge_Cost[cur_id][id])
			{
				if (find(min_cost_permutation.permutation.begin(), min_cost_permutation.permutation.end(), id) != min_cost_permutation.permutation.end())
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

int main()
{

	set_terminate(term_func);	
	for (int cur_num_nodes = 0; cur_num_nodes < nodes_list.size(); ++cur_num_nodes)
	{		
		

		try
		{
			total_nodes = nodes_list[cur_num_nodes];
			cout << "Total nodes: " << total_nodes << endl;
			if (total_nodes <= 50)
				logger.LogLevel = Logger::loglevel::debug;
			else
				logger.LogLevel = Logger::loglevel::info;			

			pEdge_Cost = new edge_cost_t*[total_nodes];
			for (node_id_t id = 0; id < total_nodes; ++id)
				pEdge_Cost[id] = new edge_cost_t[total_nodes];
			

			Initialize_Edges();				
			compute_initial_min_cost_permutation();
			cout << "Initial Min Path: Cost: " << min_cost_permutation;
			pPermutations = new Permutations(min_cost_permutation);
			pAvoided_partial_permns_ = new AvoidedPartialPermutations();
			
			// Start processing permutations
			auto start_time = steady_clock::now();

			Permutation* pPermn1 = new Permutation(pEdge_Cost[0][1], { 0,1 });
			Permutation* pPermn2 = new Permutation(pEdge_Cost[0][1], { 1,0 });
			pPermutations->Push(pPermn1); pPermutations->Push(pPermn2);

			long long thread_ct = 1;
			do
			{
				bool bPermnsEmpty = true;
				pPermutations->Empty(bPermnsEmpty);
				while (!bPermnsEmpty)
				{
					GenerateThreads();
					pPermutations->Empty(bPermnsEmpty);
				}

			// Wait for all threads to finish			
				thread_ct = thread_count.fetch_add(0);
				this_thread::sleep_for(microseconds(10));
			} while (thread_ct);

			cout << "Best Min Path: Cost: ";
			pPermutations->OutputMinCostPermutation();

			
			permutation_ct_t avoided_permn_prefix_ct = 0;
			permutation_ct_t avoided_permns_ct = pAvoided_partial_permns_->Count(avoided_permn_prefix_ct);
			cout << "Processed Permutations: " << pPermutations->total_processed_permns_ct_ << endl;
			cout << "Avoided Permutation prefixes : " << avoided_permn_prefix_ct << endl;
			cout << "Avoided Permutations: " << avoided_permns_ct << endl;
			cout << "Total permutations: " << pPermutations->total_processed_permns_ct_ + avoided_permns_ct << endl;
			permutation_ct_t prod = 1;
			for (ulint i = 2; i <= total_nodes; ++i)
				prod *= i;

			cout << "Total Expected Permutations: " << prod << endl;
			cout << "Difference: " << prod - (pPermutations->total_processed_permns_ct_ + avoided_permns_ct) << endl;

			auto end_time = steady_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
			std::cout << "Time to test permutations: ";
			Convert_Time_Duration(duration);
			cout << endl;
			std::cout << "-----------------------------------" << endl;
		
			for (int i = 0; i < total_nodes - 1; i++)
				delete[] pEdge_Cost[i];
			delete[] pEdge_Cost;
			delete pPermutations;
			delete pAvoided_partial_permns_;
		
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
	Permutation* pPermutation = new Permutation();
	 
	try
	{
		pPermutations->Pop(pPermutation);		
	}
	catch (tsp_exception & te)
	{
		logger.LogInfo(te.what()); return;
	}
	
	bool bThreadCreated = false;	
	while (!bThreadCreated)
	{
		
		logger.LogDebugInfo(". ");
		try
		{
			thread* pt = new thread(generate_and_test_permutations, pPermutation);
			++thread_count;
			bThreadCreated = true;
		}
		catch (system_error & se)
		{
			logger.LogInfo(se.what());
			bThreadCreated = false;
		}
		catch (...)
		{
			logger.LogInfo("Unknown error occurred while creating new thread\n");
			bThreadCreated = false;
		}
	}	
}
void generate_and_test_permutations(Permutation* pPermutation)
{	
	logger.LogDebugInfo("generate_and_test_permutations: ", pPermutation);
	logger.LogDebugInfo(". ");

	permutation_t oldPermn(pPermutation->permutation);
	permutation_cost_t cur_cost(pPermutation->permutation_cost);
	if (pPermutation->permutation.size() >= total_nodes)
		throw tsp_exception("Size of Permutation to process > NUM_NODES");

	node_id_t next_id = oldPermn.size();
	permutation_t newPermn{ oldPermn };
	newPermn.insert(newPermn.begin(), next_id);
	logger.LogDebugInfo("Next Permn: ", newPermn);

	permutation_cost_t newCost = pEdge_Cost[next_id][oldPermn.front()];
	newCost += cur_cost;
	CheckNewPermn(newPermn, newCost);
	 
	
	ullint pos = 1;
	for (auto oit = oldPermn.begin() + 1; oit < oldPermn.end() - 1; ++oit)
	{
		permutation_t newPermn{ oldPermn };
		auto it = newPermn.begin() + pos;
		node_id_t cur_node = *it;
		node_id_t next_node = *(it + 1);
		newPermn.insert(it, next_id);
		logger.LogDebugInfo("Next Permn: ", newPermn);

		permutation_cost_t cur_edge_cost = pEdge_Cost[cur_node][next_node];
		permutation_cost_t new_edge_cost_1 = pEdge_Cost[cur_node][next_id];
		permutation_cost_t new_edge_cost_2 = pEdge_Cost[next_id][next_node];
		permutation_cost_t newCost = cur_cost - cur_edge_cost + new_edge_cost_1 + new_edge_cost_2;
		CheckNewPermn(newPermn, newCost);

		++pos;
	}
	newPermn = oldPermn;
	node_id_t last_node = oldPermn.back();
	newPermn.push_back(next_id);	
	logger.LogDebugInfo("Next Permn: ", newPermn);
	
	newCost = pEdge_Cost[next_id][last_node];
	newCost += cur_cost;
	CheckNewPermn(newPermn, newCost);
	
	--thread_count;
}
mutex checkNewPermn_mutex;
void CheckNewPermn(permutation_t& newPermn, permutation_cost_t& newCost)
{		
	bool bExists = false;
	pAvoided_partial_permns_->Exists(newPermn, bExists);
	if(bExists) return;
		 

	logger.LogDebugInfo("CheckNewPermn: ", newPermn, newCost);
	size_t newPermn_sz = newPermn.size();
	Permutation* pPermutation = new Permutation(newCost, newPermn);
	if (newPermn_sz == total_nodes)
		pPermutations->CheckSetMinCostPermutation(pPermutation);
	else 
	pPermutations->CheckAddPartialPermnGreaterThanMinCost(pPermutation);
	return;	 
}

void Initialize_Edges()
{
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist6(1, MAX_EDGE_COST); // distribution in range [1, ULLONG_MAX]

	ulint edge_ct = 0;
	for (ulint id = 0; id < total_nodes; id++)
	{
		for (ulint next_id = id + 1; next_id < total_nodes; next_id++)
		{
			ulint cost = dist6(rng);
			pEdge_Cost[id][next_id] = pEdge_Cost[id][next_id] = cost;
			++edge_ct;
		}
	}	
	cout << "Edges initialized: " << edge_ct << endl;
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
