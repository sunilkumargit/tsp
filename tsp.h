#ifndef TCP_H_
#define TCP_H_
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
#include <set>
#include <atomic>

using namespace std;
using namespace chrono;


typedef unsigned long int ulint; 
typedef unsigned long long int ullint; // for permutations
ulint total_nodes; // total number of nodes


typedef ulint node_id_t;
typedef ulint node_ct_t;  // for node count
typedef ulint edge_cost_t;

typedef ullint permutation_cost_t;
typedef vector<node_id_t> permutation_t;
typedef ullint permutation_ct_t;


class Logger
{
	static mutex cout_mutex;
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
	static void DebugLogMesg(const char* mesg) { DebugLogMesg((string)mesg); }

	static void DebugLogMesg(const char* mesg, permutation_cost_t cost, const permutation_t& permn)
	{
		cout_mutex.lock();
		if (LogLevel < loglevel::debug) { cout_mutex.unlock();; return; }
		cout << mesg << cost << endl;
		copy(permn.begin(), permn.end(), ostream_iterator<node_id_t>(cout, ", "));
		cout << endl;
		cout_mutex.unlock();
	}
	static void DebugLogMesg(const char* mesg, const permutation_t& permn)
	{
		cout_mutex.lock();
		if (LogLevel < loglevel::debug) { cout_mutex.unlock();; return; }
		cout << mesg;
		copy(permn.begin(), permn.end(), ostream_iterator<node_id_t>(cout, ", "));
		cout << endl;
		cout_mutex.unlock();
	}
};
mutex Logger::cout_mutex;
Logger::loglevel Logger::LogLevel = loglevel::debug;
class tsp_exception : public exception
{
public:
	tsp_exception(const char* data) :exception(data) {}
};

class Edge // pair of nodes and cost
{
public:
	Edge(node_id_t _n1, node_id_t _n2, edge_cost_t _cost) : id1(_n1), id2(_n2), cost(_cost) {}
	node_id_t id1;
	node_id_t id2;
	edge_cost_t cost;
};

class AvoidedPermnPrefixes
{

public:
	map<permutation_ct_t, set<permutation_t>> avoided_permn_prefixes;
	void AddPermnPrefix(const permutation_t& permn)
	{		 
		avoided_permn_prefixes[permn.size()].insert(permn);		 
	}
	bool Exists(const permutation_t& permn)
	{		 
		bool bExists = avoided_permn_prefixes[permn.size()].find(permn) == avoided_permn_prefixes[permn.size()].end();		
		return bExists;		 
	}
	permutation_ct_t Count(permutation_ct_t& avoided_permn_prefix_ct) 
	{
		avoided_permn_prefix_ct = 0;
		permutation_ct_t avoided_permn_ct = 0;
		for (auto item : avoided_permn_prefixes)
		{
			size_t ct = item.second.size();
			avoided_permn_prefix_ct += ct;
			permutation_ct_t prod = 1;
			for (ulint i = total_nodes - ct; i > 2; --i)
				prod *= i;
			avoided_permn_ct += prod;
		}
		return avoided_permn_ct;
	}
};

struct Permutation
{
	permutation_t permutation;
	permutation_cost_t permutation_cost=0;
	Permutation(permutation_t permn = {}, permutation_cost_t permn_cost=0) :permutation(permn), permutation_cost(permn_cost) {}
	Permutation(const Permutation& rhs) :permutation{ rhs.permutation }, permutation_cost{ rhs.permutation_cost }{}
	//friend ostream& operator << (ostream & os, Permutation & perm);
};
ostream& operator << (ostream& os, Permutation& permn)
{
	os << "Cost: " << permn.permutation_cost;
	copy(permn.permutation.begin(), permn.permutation.end(), ostream_iterator<node_id_t>(os, ", "));
	return os;
}
class Permutations
{
private:
	mutex permutations_mutex_;
	queue<Permutation*> permutations_;	
public:	
	void Pop(Permutation& permutation)
	{
		permutations_mutex_.lock();
		if (permutations_.size() == 0)
			throw tsp_exception("Attempt to pop from empty set of permutations");
		permutation.permutation_cost = permutations_.front()->permutation_cost;
		permutation.permutation = permutations_.front()->permutation;		
		Logger::DebugLogMesg("Popped: ", permutation.permutation_cost, permutation.permutation);
		permutations_.pop();
		permutations_mutex_.unlock();
	}
	void Push(Permutation* pPermutation)
	{
		permutations_mutex_.lock();
		permutations_.push(pPermutation);		
		permutations_mutex_.unlock();
	}
	void Empty(bool& bPermnsEmpty)
	{
		permutations_mutex_.lock();
		bPermnsEmpty = permutations_.empty();
		if (bPermnsEmpty)Logger::DebugLogMesg("Permutation set empty");
		permutations_mutex_.unlock();
	}
};


/*
class ThreadList
{
private:
	mutex thread_list_mutex;
	ulint thread_ct = 0;
public:
	void AddThread(thread::id id)
	{
		thread_list_mutex.lock();
		++thread_ct;
		thread_list_mutex.unlock();
	}
	void RemoveThread(thread::id id)
	{
		thread_list_mutex.lock();
		--thread_ct;
		thread_list_mutex.unlock();
	}
	void AllThreadsFinished(bool& bFinished)
	{
		thread_list_mutex.lock();
		bFinished = thread_ct == 0;
		thread_list_mutex.unlock();
	}
};
*/

void Initialize();
void generate_and_test_permutations(permutation_cost_t cur_cost, permutation_t oldPermn);
void GenerateThreads();
void CheckNewPermn(permutation_t& newPermn, permutation_cost_t& newCost);
void find_initial_min_cost_permutation();
void Convert_Time_Duration(long long Duration);
void term_func() {
	cout << "term_func was called by terminate." << endl;
	exit(-1);
}

#endif TCP_H_