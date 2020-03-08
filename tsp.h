#pragma once
#include <iostream>
#include <random>
#include <map>
#include <limits>
#include <chrono>
#include <sstream>
#include <list>
#include <stack>
#include <iterator>
#include <algorithm>
#include <queue>
#include <mutex>
#include <thread>

using namespace std;
using namespace chrono;


typedef unsigned long int ulint;
typedef unsigned long long int ullint; // for permutation
ulint NUM_NODES; // total number of nodes

typedef ullint cost_t;
typedef ulint id_t;
typedef vector<id_t> permn_t;
typedef ullint permns_ct_t;

class tsp_exception : public exception
{
public:
	tsp_exception(const char* data) :exception(data) {}
};
class Edge // pair of nodes and cost
{
public:
	Edge(id_t _n1, id_t _n2, cost_t _cost) : id1(_n1), id2(_n2), cost(_cost) {}
	id_t id1;
	id_t id2;
	cost_t cost;
};

class Node // Single node and it's neighbors together with cost
{

public:	
	Node(id_t _id = 0) :node_id(_id) {  }
	id_t node_id;
	multimap<cost_t, id_t> Neighbors;
};

class Path // single path, list of nodes and total cost
{
private:
	mutex pathMutex;
	vector<id_t> path_nodes;
	cost_t path_cost;

public:
	static string bestCostPathString;
	static cost_t bestPathCost;
	//Path(cost_t cost, permn_t nodes) {			path_cost = cost;  path_nodes = nodes;	}
	Path() { }

	friend void CheckNewPermn(permn_t& newPermn, cost_t& newCost);
	void MakeBestCostPathString(cost_t& cost, permn_t& permn)
	{
		bestPathCost = cost;
		ostringstream oss;
		oss << "Min Path Cost: " << cost << endl;
		oss << "Min Path : " << endl;
		size_t permn_sz = permn.size();
		for (int i = 0; i < permn_sz; i++)
		{
			oss << permn[i];
			if (i < permn_sz - 1) oss << ", ";
		}
		oss << endl;
		bestCostPathString = oss.str();
	}
	void BackNode(id_t& id)
	{
		pathMutex.lock();
		if (path_nodes.empty())
			throw tsp_exception("Attempt to access node from empty pathnodes");
		id = path_nodes.back();
		pathMutex.unlock();
	}
	void FindNode(id_t id, bool& bNodeFound)
	{
		//pathMutex.lock();
		auto it = find(path_nodes.begin(), path_nodes.end(), id);
		bNodeFound = it != path_nodes.end();
		//pathMutex.unlock();
	}
	void GetCost(cost_t& cost)
	{
		pathMutex.lock();
		cost = path_cost;
		pathMutex.unlock();
	}
	void SetCost(cost_t cost)
	{
		pathMutex.lock();
		path_cost = cost;
		pathMutex.unlock();
	}
	void AddCost(cost_t cost)
	{
		pathMutex.lock();
		path_cost += cost;
		pathMutex.unlock();
	}
	void AddNode(id_t id)
	{
		pathMutex.lock();
		path_nodes.push_back(id);
		pathMutex.unlock();
	}
	void GetPathSize(ulint& path_size)
	{
		//pathMutex.lock();
		path_size = path_nodes.size();
		//pathMutex.unlock();
	}
	void ResetMinPath(cost_t cost, permn_t permn)
	{
		pathMutex.lock();
		if (permn.size() != NUM_NODES)
			throw logic_error("New Min Path length != NUM_NODES");
		path_cost = cost;
		path_nodes.clear();
		path_nodes.resize(NUM_NODES);
		for (int i = 0; i < NUM_NODES; ++i)
			path_nodes[i] = permn[i];
		pathMutex.unlock();
	}
	void GetCostAndNodes(cost_t& cost, permn_t& permn)
	{
		pathMutex.lock();
		cost = path_cost;
		permn.clear();
		permn.resize(path_nodes.size());
		copy(path_nodes.begin(), path_nodes.end(), permn.begin());
		pathMutex.unlock();
	}
	void CopyNodes(permn_t& permn)
	{
		pathMutex.lock();
		permn.clear();
		permn.resize(NUM_NODES);
		copy(path_nodes.begin(), path_nodes.begin(), permn.begin());
		pathMutex.unlock();
	}
	void OutputNodes(string msg)
	{
		pathMutex.lock();
		cout << msg << endl;
		cout << path_cost << endl;
		copy(path_nodes.cbegin(), path_nodes.cend(),
			ostream_iterator<id_t>(cout, ", "));
		cout << endl;
		pathMutex.unlock();
	}
};

class AvoidablePermnPrefix
{
private:
	mutex avoidablePermnPrefix_mutex;
	list<vector<id_t>> costlyPermnPrefixes;
	ullint avoided_permns_ct = 0;
	ullint avoided_permn_prefix_ct = 0;
public:
	AvoidablePermnPrefix() {};
	void AddPermnPrefix(vector<id_t> permn)
	{
		avoidablePermnPrefix_mutex.lock();

		if (permn.size() >= NUM_NODES)
			throw tsp_exception("Avoiding Permutation Prefix length >= NUM_NODES");

		auto lsit = find(costlyPermnPrefixes.begin(), costlyPermnPrefixes.end(), permn);
		if (lsit != costlyPermnPrefixes.end())
			return;

		costlyPermnPrefixes.push_back(permn);
		avoided_permn_prefix_ct++;
		ullint prod = 1;
		for (ullint ct = NUM_NODES - permn.size(); ct > 1; --ct)
			prod *= ct;
		avoided_permns_ct += prod;
		avoidablePermnPrefix_mutex.unlock();
	}
	void Exists(vector<id_t> permn, bool& bExists)
	{
		avoidablePermnPrefix_mutex.lock();
		auto lsit = find(costlyPermnPrefixes.begin(), costlyPermnPrefixes.end(), permn);
		bExists = lsit != costlyPermnPrefixes.end();
		avoidablePermnPrefix_mutex.unlock();
	}
	void GetCounts(ullint& avoided_permn_prefixes, ullint& avoided_permns) {
		avoidablePermnPrefix_mutex.lock();
		avoided_permn_prefixes = avoided_permn_prefix_ct;
		avoided_permns = avoided_permns_ct;
		avoidablePermnPrefix_mutex.unlock();
	}
};
AvoidablePermnPrefix avoidablePermnPrefixes;
class Permutations
{
private:
	mutex permns_mutex;
	queue<Path*> Perms;
	permns_ct_t total_processed_permns = 0, total_UnProcessed_permns = 0;
public:
	void GetTotalPermnCounts(permns_ct_t& processed_permns_ct, permns_ct_t& un_processed_permns_ct)
	{
		permns_mutex.lock();
		processed_permns_ct = total_processed_permns;
		un_processed_permns_ct = total_UnProcessed_permns;
		permns_mutex.unlock();
	}
	void IncrementProcessedPermnsCount()
	{
		//permns_mutex.lock();
		total_processed_permns++;
		//permns_mutex.unlock();
	}
	void IncrementUnProcessedPermnsCount()
	{
		permns_mutex.lock();
		total_UnProcessed_permns++;
		permns_mutex.unlock();
	}
	void Front(Path* pPath)
	{
		permns_mutex.lock();
		pPath = Perms.front();
		permns_mutex.unlock();
	}
	void Pop(cost_t& cost, permn_t& permn)
	{
		permns_mutex.lock();
		Path* pPath = Perms.front();
		pPath->GetCostAndNodes(cost, permn);
		Perms.pop();
		permns_mutex.unlock();
	}
	void Push(Path* pPath)
	{
		permns_mutex.lock();
		Perms.push(pPath);
		permns_mutex.unlock();
	}
	void Empty(bool& bPermnsEmpty)
	{
		permns_mutex.lock();
		bPermnsEmpty = Perms.empty();
		permns_mutex.unlock();
	}
};


void Initialize();
void generate_and_test_permutations(cost_t cur_cost, permn_t oldPermn);
void CheckNewPermn(permn_t& newPermn, cost_t& newCost);


void Convert_Time_Duration(long long Duration);
void term_func() {
	cout << "term_func was called by terminate." << endl;
	exit(-1);
}
mutex cout_mutex;
void  OutputBusy(string mesg)
{

	cout_mutex.lock();
	cout << mesg;
	cout.flush();
	cout_mutex.unlock();

}
class Logger
{
public:
	enum class loglevel { basic, info, debug };
private:
	static loglevel LogLevel;
public:
	Logger(loglevel level = loglevel::debug) { LogLevel = level; }
	static void LogMesg(const string& mesg)
	{
		cout_mutex.lock();
		cout << mesg << endl;
		cout_mutex.unlock();
	}
	static void DebugLogMesg(const string& mesg)
	{
		cout_mutex.lock();
		if (LogLevel < loglevel::debug) { cout_mutex.unlock();; return; }
		cout << mesg << endl;
		cout_mutex.unlock();

	}
	static void DebugLogMesg(const char* mesg)
	{
		cout_mutex.lock();
		if (LogLevel < loglevel::debug) { cout_mutex.unlock();; return; }
		cout << mesg << endl;
		cout_mutex.unlock();

	}
	static void DebugLogMesg(const char* mesg, cost_t cost, const permn_t& permn)
	{
		cout_mutex.lock();
		if (LogLevel < loglevel::debug) { cout_mutex.unlock();; return; }
		cout << mesg << cost << endl;
		copy(permn.begin(), permn.end(), ostream_iterator<id_t>(cout, ", "));
		cout << endl;
		cout_mutex.unlock();

	}
	static void DebugLogMesg(const char* mesg, const permn_t& permn)
	{
		cout_mutex.lock();
		if (LogLevel < loglevel::debug) { cout_mutex.unlock();; return; }
		cout << mesg;
		copy(permn.begin(), permn.end(), ostream_iterator<id_t>(cout, ", "));
		cout << endl;
		cout_mutex.unlock();

	}
};
Logger::loglevel Logger::LogLevel = loglevel::basic;
class ThreadList
{
private:
	mutex thread_list_mutex;
	list<thread::id>* pThreadList = new list<thread::id>();
public:
	void AddThread(thread::id id)
	{
		pThreadList->push_back(id);
	}
	void RemoveThread()
	{
		thread_list_mutex.lock();
		auto it = find(pThreadList->begin(), pThreadList->end(), this_thread::get_id());
		if (it == pThreadList->end())
			throw tsp_exception("Attempt to remove non-existing id");
		pThreadList->erase(it);
		thread_list_mutex.unlock();
	}
	void Empty(bool& bEmpty)
	{
		thread_list_mutex.lock();
		bEmpty = pThreadList->empty();
		thread_list_mutex.unlock();
	}
};
