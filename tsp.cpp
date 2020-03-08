// tsp.cpp 
// Author : SUNILKUMAR AV
// SUNILKUMAR.PROG@gmail.com
// C++ 11
// Intel(R) Core(TM) i5-8265U CPU @1.60 GHz @1.80 GHz
// 11/18/2019, version : 1.0
// 
// 
#include "tsp.h"

vector<Node*> graph_nodes;  // all nodes
multimap<cost_t, Edge> tsp_edges; // all edges
string Path::bestCostPathString = "";
cost_t Path::bestPathCost = 0;
Permutations permns;
Path best_path; // min path so far
vector<ulint> total_nodes{ 10, 20, 50 , 100, 500, 1000, 10000, 50000, 100000, 1000000 };
ThreadList threadList;
void GenerateThreads();
cost_t** pSorted_nbr_cost;  // two dim array indexed by two nodes to get cost of edge between them

int main()
{

	set_terminate(term_func);

	 
	cout << "-------------------------------------" << endl;
	for (int cur_num_nodes = 0; cur_num_nodes < total_nodes.size(); ++cur_num_nodes)
	{		
		graph_nodes.clear();
		tsp_edges.clear();
		try
		{
			NUM_NODES = total_nodes[cur_num_nodes];
			cout << "Total nodes: " << NUM_NODES << endl;
			
			Initialize();
			ulint tsp_edge_ct = 0;
			for (int i = 0; i < NUM_NODES; i++)
			{
				graph_nodes.push_back(new Node(i));
			}
			pSorted_nbr_cost = (cost_t**)malloc(sizeof(cost_t*) * NUM_NODES);
			for(int i=0;i<NUM_NODES;i++)
				pSorted_nbr_cost[i] = (cost_t*) malloc(sizeof(cost_t) * NUM_NODES);
			
			for (auto elit = tsp_edges.begin(); elit != tsp_edges.end(); ++elit)
			{
				id_t id1 = elit->second.id1;
				id_t id2 = elit->second.id2;
				cost_t cost = elit->first;

				graph_nodes[id1]->Neighbors.insert(make_pair(cost, id2)); // make each other as neighbor
				graph_nodes[id2]->Neighbors.insert(make_pair(cost, id1));

				pSorted_nbr_cost[id1][id2] = cost;
				pSorted_nbr_cost[id2][id1] = cost;				

				tsp_edge_ct++;
			}
			std::cout << "Edges processed: " << tsp_edge_ct << endl;
			
			best_path.AddNode(0);	// start with node 0
			ulint path_size = 0;
			do
			{
				id_t next_id;
				best_path.BackNode(next_id);
				//std::cout << "next_node: " << next_id;
				bool bNbrFound = false;
				for (auto nit = graph_nodes[next_id]->Neighbors.begin(); !bNbrFound && nit != graph_nodes[next_id]->Neighbors.end(); ++nit)
				{
					bool bNodeFound = false;
					best_path.FindNode(nit->second, bNodeFound);
					if (!bNodeFound)
					{
						best_path.AddNode(nit->second);
						best_path.AddCost(nit->first);
						//std::cout << " nbr: " << nit->second << endl;
						bNbrFound = true;
						break;
					}
				}
				
				best_path.GetPathSize(path_size);
			} while (path_size < NUM_NODES);
			best_path.GetCost(Path::bestPathCost);
			best_path.OutputNodes((string)"Initial Min path cost  : ");
			 
			// Start processing permutations
			auto start_time = steady_clock::now();
			vector<ulint> v1 = { 0,1 };	vector<ulint> v2 = { 1,0 };
			cost_t cost = 0;//			graph_nodes[0]->GetSortedNeighborCost(1, cost);
			cost = pSorted_nbr_cost[0][1];
			Path* pPath1 = new Path();
			Path* pPath2 = new Path();
			pPath1->SetCost(cost); pPath2->SetCost(cost);
			pPath1->AddNode(0); pPath1->AddNode(1);
			pPath2->AddNode(1); pPath2->AddNode(0);
			permns.Push(pPath1); permns.Push(pPath2);			

			bool bPermnsEmpty = true;
			permns.Empty(bPermnsEmpty);
			while (!bPermnsEmpty)
			{
				GenerateThreads();
				permns.Empty(bPermnsEmpty);
			}

			cout << endl;
			cout << Path::bestCostPathString;

			permns_ct_t total_processed_permns_ct=0, total_un_processed_permns_ct = 0;
			permns.GetTotalPermnCounts(total_processed_permns_ct, total_un_processed_permns_ct);
			permns_ct_t avoided_permn_prefixes = 0, avoided_permns_ct=0;
			avoidablePermnPrefixes.GetCounts(avoided_permn_prefixes, avoided_permns_ct);
			cout << "Processed Permutations: " << total_processed_permns_ct << endl;
			cout << "UnProcessed Permutations: " << total_un_processed_permns_ct << endl;
			cout << "Avoided Permutation prefixes: " << avoided_permn_prefixes << endl;
			cout << "Avoided Permutations: " << avoided_permns_ct << endl;
			cout << "Total permutations: " << total_processed_permns_ct + avoided_permns_ct << endl;
			permns_ct_t prod = 1;
			for (ulint i = 2; i <= NUM_NODES; ++i)
				prod *= i;

			cout << "Total Expected Permutations: " << prod << endl;
			cout << "Difference: " << prod - (total_processed_permns_ct + total_un_processed_permns_ct + avoided_permns_ct) << endl;
			auto end_time = steady_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
			std::cout << "Time to test permutations: ";
			Convert_Time_Duration(duration);
			cout << endl;
			std::cout << "-----------------------------------" << endl;

			for (int i = 0; i < NUM_NODES; i++)
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
	list<thread*>* pThreadList = new list<thread*>();
	ulint thread_ct = 0, join_ct = 0;
	permn_t oldPermn;
	cost_t cur_cost = 0;
	permns.Pop(cur_cost, oldPermn);
	bool bThreadCreated = false;
	while (!bThreadCreated)
	{
		Logger::DebugLogMesg(". ");
		try
		{
			thread* pt = new thread(generate_and_test_permutations, cur_cost, oldPermn);
			threadList.AddThread(pt->get_id());
			thread_ct++;
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
	 
	bool bThreadListEmpty = false;
	threadList.Empty(bThreadListEmpty);
	while (!bThreadListEmpty)
	{
		threadList.Empty(bThreadListEmpty);
	}
}
void generate_and_test_permutations(cost_t cur_cost, permn_t oldPermn)
{	
	Logger::DebugLogMesg("generate_and_test_permutations: ", cur_cost, oldPermn);
	Logger::DebugLogMesg(". "); 

	if (oldPermn.size() >= NUM_NODES)
		throw tsp_exception("Size of Permutation to process > NUM_NODES");

	id_t next_id = oldPermn.size();
	permn_t newPermn(oldPermn);
	newPermn.insert(newPermn.begin(), next_id);
	Logger::DebugLogMesg("Next Permn: ", newPermn);
	bool bExists = false;
	avoidablePermnPrefixes.Exists(newPermn, bExists);
	if (!bExists)
	{
		cost_t newCost = 0;
		//graph_nodes[next_id]->GetSortedNeighborCost(oldPermn.front(), newCost);
		newCost = pSorted_nbr_cost[next_id][oldPermn.front()];
		newCost += cur_cost;
		CheckNewPermn(newPermn, newCost);
	}
	else
	{
		Logger::DebugLogMesg("Avoidable Permn Prefix Exists");
		permns.IncrementUnProcessedPermnsCount();
	}
	int pos = 1;
	for (auto oit = oldPermn.begin() + 1; oit < oldPermn.end() - 1; ++oit)
	{
		permn_t newPermn(oldPermn);
		auto it = newPermn.begin() + pos;
		id_t cur_node = *it;
		id_t next_node = *(it + 1);
		newPermn.insert(it, next_id);
		Logger::DebugLogMesg("Next Permn: ", newPermn);
		bool bExists;
		avoidablePermnPrefixes.Exists(newPermn, bExists);
		if (bExists){ Logger::DebugLogMesg("Avoidable Permn Prefix Exists"); permns.IncrementUnProcessedPermnsCount(); continue;}

		cost_t cur_edge_cost = 0; //graph_nodes[cur_node]->GetSortedNeighborCost(next_node, cur_edge_cost);
		cur_edge_cost = pSorted_nbr_cost[cur_node][next_node];
		cost_t new_edge_cost_1 = 0; //graph_nodes[cur_node]->GetSortedNeighborCost(next_id, new_edge_cost_1);
		new_edge_cost_1 = pSorted_nbr_cost[cur_node][next_id];
		cost_t new_edge_cost_2 = 0; //graph_nodes[next_id]->GetSortedNeighborCost(next_node, new_edge_cost_2);
		new_edge_cost_2 = pSorted_nbr_cost[next_id][next_node];
		cost_t newCost = cur_cost - cur_edge_cost + new_edge_cost_1 + new_edge_cost_2;
		CheckNewPermn(newPermn, newCost);

		++pos;
	}
	newPermn = oldPermn;
	id_t last_node = oldPermn.back();
	newPermn.push_back(next_id);	
	Logger::DebugLogMesg("Next Permn: ", newPermn);
	bExists = false;
	avoidablePermnPrefixes.Exists(newPermn, bExists);
	if (!bExists)
	{
		cost_t newCost = 0; //graph_nodes[next_id]->GetSortedNeighborCost(last_node, newCost);
		newCost = pSorted_nbr_cost[next_id][last_node];
		newCost += cur_cost;
		CheckNewPermn(newPermn, newCost);
	}
	else
	{
		Logger::DebugLogMesg("Avoidable Permn Prefix Exists");
		permns.IncrementUnProcessedPermnsCount();
	}
	threadList.RemoveThread();
}
mutex checkNewPermn_mutex;
void CheckNewPermn(permn_t& newPermn, cost_t& newCost)
{
	checkNewPermn_mutex.lock();	
	Logger::DebugLogMesg("CheckNewPermn: ", newCost, newPermn);
	permns.IncrementProcessedPermnsCount();
	size_t newPermn_sz = newPermn.size();
	if (Path::bestPathCost > newCost && newPermn_sz == NUM_NODES)
	{
		best_path.MakeBestCostPathString(newCost, newPermn);
		Logger::DebugLogMesg("CheckNewPermn: Min Path Changed");
	}
	else if (best_path.path_cost <= newCost && newPermn_sz < NUM_NODES)
	{		
		Logger::DebugLogMesg("CheckNewPermn: Avoiding permn");
		avoidablePermnPrefixes.AddPermnPrefix(newPermn);
	}
	else if (newPermn_sz < NUM_NODES)
	{
		Path* pPath = new Path();
		pPath->path_cost = newCost;
		pPath->path_nodes = newPermn;
		permns.Push(pPath);
		Logger::DebugLogMesg("CheckNewPermn: Pushed New Permn");
	}
	checkNewPermn_mutex.unlock();
}
void Initialize()
{
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist6(1, ULONG_MAX); // distribution in range [1, ULLONG_MAX]

	for (ulint id = 0; id < NUM_NODES; id++)
	{
		for (ulint next_id = id + 1; next_id < NUM_NODES; next_id++)
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
