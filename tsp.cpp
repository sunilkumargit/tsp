// tsp.cpp 
// Author : SUNILKUMAR AV
// SUNILKUMAR.PROG@gmail.com
// 11/18/2019, version : 1.0
// First version using 20 node.
// Single thread to measure time impact later
//

#include <iostream>
#include <random>
#include <map>
#include <limits>
#include <chrono>
#include <sstream>
using namespace std;

const unsigned NUM_NODES = 20; // try with low number of nodes first
typedef unsigned long int ulint;
typedef unsigned long long int ullint; // for path length
typedef ulint node;
typedef ullint cost;
typedef ulint id;
void test_permutation(vector<node>& cur_permutation);

class tsp_exception : public exception
{
public:
	tsp_exception(const char* data):exception(data)  {}
};
class edge // pair of nodes and cost
{
public:
	edge(id _n1, id _n2, cost _cost) :id1(_n1), id2(_n2),cost(_cost){}
	id id1;
	id id2;
	cost cost;
};

class Neighbor // represent a neighbor and distance to it
{
public:
	Neighbor(id _to, cost c) :to(_to), cost(c) {}
	id to;
	cost cost;
};

class Node // Single node and it's neigbors together with cost
{
public:
	Node(id _id) :node_id(_id){}
	id node_id;	
	multimap<cost, id> Neighbors;
};

vector<Node> graph_nodes;  // all nodes
multimap<cost, edge> tsp_edges; // all edges

class path // single path, list of nodes and total cost
{
public:
	vector<id> path_nodes;
	cost path_cost;
};

path best_path; // min path so far
void Initialize();

int main()
{
	Initialize();	
	ulint tsp_edge_ct = 0;
	for (auto elit = tsp_edges.begin(); elit != tsp_edges.end(); ++elit)
	{
		id id1 = elit->second.id1;
		id id2 = elit->second.id2;
		cost cost = elit->first;

		graph_nodes[id1].Neighbors.insert(make_pair(cost,id2)); // make each other as neighbor
		graph_nodes[id2].Neighbors.insert(make_pair(cost,id1));
		tsp_edge_ct++;
	}
	std::cout << " tsp_edges processed: " << tsp_edge_ct << endl;
	best_path.path_nodes.push_back(0);	// start with node 0
	do
	{
		id next_id = best_path.path_nodes.back();
		std::cout << "next_node: " << next_id;
		bool bNbrFound = false;
		for (auto nit = graph_nodes[next_id].Neighbors.begin(); !bNbrFound && nit != graph_nodes[next_id].Neighbors.end(); ++nit)
		{
			if (find(best_path.path_nodes.begin(), best_path.path_nodes.end(), nit->second) == best_path.path_nodes.end())
			{
				best_path.path_nodes.push_back(nit->second);
				best_path.path_cost += nit->first;
				std::cout << " nbr: " << nit->second << endl;
				bNbrFound = true;
				break;
			}			
		}
	}while (best_path.path_nodes.size() < NUM_NODES);	

	vector<node> cur_permutation; // starting sequence
	for (int i=0;i<NUM_NODES;i++)
	{		
			cur_permutation.push_back(i);		
	}
	auto start_time = std::chrono::high_resolution_clock::now();  // measure time
	try
	{
		test_permutation(cur_permutation);
	}
	catch (tsp_exception tspe)
	{
		std::cout << " Neighbors missing, Error building data structures" << endl;
		std::cout << " Error: " << tspe.what() << endl;
		exit(EXIT_FAILURE);
	}
	auto end_time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
	std::cout << "Time to test permutations: " << duration << endl;

	exit(EXIT_SUCCESS);
}

void test_permutation(vector<node>& cur_permutation)
{
	long int perm_ct = 0;
	ulint best_path_change_ct = 0;
	bool bValidPermutation = false;
	do  // for each permutations
	{
		cost cur_permutation_cost = 0;					
		perm_ct++;
		for (int i = 0; i < NUM_NODES - 1 && best_path.path_cost > cur_permutation_cost; i++)
		{
			bool bFoundNbr = false;
			auto nit = find_if(graph_nodes[cur_permutation[i]].Neighbors.begin(), graph_nodes[cur_permutation[i]].Neighbors.end(),
				[&](auto const& cost_id_pair)->bool { return cost_id_pair.second == cur_permutation[i + 1]; });
			if (nit == graph_nodes[cur_permutation[i]].Neighbors.end())
			{
				ostringstream stringStream;
				stringStream << "Neighbor " << i+1 << " of " << i << "in permutation not found";				
				throw tsp_exception(stringStream.str().c_str());
			}
			cur_permutation_cost += nit->first;	

			if (best_path.path_cost < cur_permutation_cost) // add last to first node edge cost
			{				
				auto fit = find_if(graph_nodes[cur_permutation[NUM_NODES-1]].Neighbors.begin(), graph_nodes[cur_permutation[NUM_NODES - 1]].Neighbors.end(),
					[&](auto const& cost_id_pair)->bool { return cost_id_pair.second == cur_permutation[0]; });
				if (fit == graph_nodes[cur_permutation[NUM_NODES - 1]].Neighbors.end())
				{
					ostringstream stringStream;
					stringStream << "Neighbor " << i + 1 << " of " << i << "in permutation not found";
					throw tsp_exception(stringStream.str().c_str());
				}
				cur_permutation_cost += fit->first;
			}
		}
		
		if (best_path.path_cost > cur_permutation_cost)
		{		
			++best_path_change_ct;
			copy(cur_permutation.begin(), cur_permutation.end(), best_path.path_nodes.begin());
			best_path.path_cost = cur_permutation_cost;
			std::cout << "Best Path Changed: " << best_path_change_ct << endl;
			for (int i = 0; i < NUM_NODES; i++)
				std::cout << best_path.path_nodes[i] << " , ";
			std::cout << " : " << best_path.path_cost << endl;
		}	

		bValidPermutation = next_permutation(cur_permutation.begin(), cur_permutation.end(), std::less<int>());		
	} while (bValidPermutation); // end generating permutations

	for (int i = 0; i < NUM_NODES; i++)
		std::cout << best_path.path_nodes[i] << " , ";
	std::cout << " : " << best_path.path_cost << endl;
}
void Initialize()
{
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist6(1, ULONG_MAX); // distribution in range [1, ULLONG_MAX]

	for (ulint id = 0; id < NUM_NODES; id++)
	{
		for (ulint next_id = id+1; next_id < NUM_NODES; next_id++)
		{
			ulint cost = dist6(rng);
			tsp_edges.insert(make_pair(cost, *new edge(id, next_id,cost)));
		}
	}
	for (int i = 0; i < NUM_NODES+1; i++)
	{
		graph_nodes.push_back(*new Node(i));
	}
	cout << " Initialized" << endl;
}

