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
const auto MAX_EDGE_COST = ULONG_MAX;

typedef ulint node_id_t;
typedef ulint node_ct_t;  // for node count
typedef ulint edge_cost_t;

typedef ullint permutation_cost_t;
typedef vector<node_id_t> permutation_t;
typedef ullint permutation_ct_t;

struct Permutation
{
public:
	permutation_t permutation;
	permutation_cost_t permutation_cost = 0;
	Permutation() :permutation_cost{} {}
	Permutation(permutation_cost_t permn_cost, permutation_t permn) : permutation(permn), permutation_cost(permn_cost) {}
	Permutation(const Permutation& rhs) :  permutation{ rhs.permutation }, permutation_cost{ rhs.permutation_cost }{}
	void operator=(const Permutation& rhs)
	{
		permutation.clear();
		permutation.resize(rhs.permutation.size());
		copy(rhs.permutation.cbegin(), rhs.permutation.cend(), permutation.begin());
		permutation_cost = rhs.permutation_cost;
	}
	friend ostream& operator <<(ostream& os,  Permutation permn);
};
ostream& operator << (ostream& os, Permutation permn)
{
	os << "Cost: " << permn.permutation_cost;
	//for (int i = 0; i < permn.permutation.size(); ++i)		cout << permn.permutation[i] << ", ";	cout << endl;
	copy(permn.permutation.begin(), permn.permutation.end(), ostream_iterator<node_id_t>(os, ", "));
	cout << endl;
	return os;
}
ostream& operator << (ostream& os, permutation_t permn)
{
	//for (int i = 0; i < permn.size(); ++i)		cout << permn[i] << ", ";	cout << endl;
	copy(permn.begin(), permn.end(), ostream_iterator<node_id_t>(os, ", "));
	cout << endl;
	return os;
}
class Logger
{
private:
	mutex cout_mutex;
public:
	enum class loglevel { basic=1, info, debug };	
public:
	loglevel LogLevel;
	Logger(loglevel level = loglevel::basic) { LogLevel = level; }
	/*
	template<typename T>
	void LogInfo(T mesg)
	{
		cout_mutex.lock();
		if (LogLevel < loglevel::info) { cout_mutex.unlock(); return; }
		cout << mesg << endl;
		cout_mutex.unlock();
	}
	*/
	template<typename T>
	void LogInfo(const T mesg, Permutation* pPermutation = 0)
	{
		cout_mutex.lock();
		if (LogLevel < loglevel::info) { cout_mutex.unlock(); return; }
		cout << mesg;
		cout << (*pPermutation);
		cout_mutex.unlock();
	}
	template<typename T, typename U>
	void LogInfo(const T mesg, U val)
	{
		cout_mutex.lock();
		if (LogLevel < loglevel::info) { cout_mutex.unlock(); return; }
		cout << mesg << val << endl;
		
		cout_mutex.unlock();
	}
	
	template<typename T>
	void LogDebugInfo(T mesg) 
	{ 
		cout_mutex.lock();
		if (LogLevel < loglevel::debug) { cout_mutex.unlock(); return; }
		cout << mesg;		
		cout_mutex.unlock();
	}
	
	//void LogDebugInfo(const char* mesg, Permutation* pPermutation = 0) { LogDebugInfo((string&)mesg,  pPermutation); }
	template<typename T>
	void LogDebugInfo(const T mesg, Permutation* pPermutation)
	{
		cout_mutex.lock();
		if (LogLevel < loglevel::debug) { cout_mutex.unlock(); return; }
		cout << mesg;
		cout << *pPermutation;
		cout_mutex.unlock();
	}
	template<typename T>
	void LogDebugInfo(T mesg, permutation_t permn, permutation_cost_t cost = {})
	{
		cout_mutex.lock();
		if (LogLevel < loglevel::debug) { cout_mutex.unlock(); return; }
		cout << mesg;
		cout << permn;
		cout_mutex.unlock();
	}
	 
};
 
Logger logger(Logger::loglevel::basic);
class tsp_exception : public exception
{
public:
	tsp_exception(const char* data) :exception(data) {}
};


class AvoidedPartialPermutations
{
	mutex avoided_partial_permns_mutex;
	permutation_ct_t cur_max_len = 0;
public:	
	map<permutation_ct_t, set<permutation_t>> avoided_partial_permns_;
	void AddPartialPermutation(const permutation_t& permn)
	{	
		avoided_partial_permns_mutex.lock();
		size_t permn_sz = permn.size();
		if (cur_max_len < permn_sz) cur_max_len = permn_sz;
		avoided_partial_permns_[permn_sz].insert(permn);
		logger.LogDebugInfo("Prefix added: ", permn);
		avoided_partial_permns_mutex.unlock();
	}
	
	void Exists(const permutation_t& permn, bool& bExists)
	{		 
		avoided_partial_permns_mutex.lock();
		size_t permn_sz = permn.size();
		if (permn_sz > cur_max_len) { bExists = false; avoided_partial_permns_mutex.unlock(); return; 	}	
		bExists = avoided_partial_permns_[permn_sz].find(permn) != avoided_partial_permns_[permn_sz].end();		
		avoided_partial_permns_mutex.unlock();
	}
	
	permutation_ct_t Count(permutation_ct_t& avoided_partial_permns_ct)
	{
		avoided_partial_permns_ct = 0;
		permutation_ct_t avoided_permn_ct = 0;
		for (auto item : avoided_partial_permns_)
		{
			size_t ct = item.second.size();
			avoided_partial_permns_ct += ct;
			permutation_ct_t prod = 1;
			for (ulint i = total_nodes - ct; i > 2; --i)
				prod *= i;
			avoided_permn_ct += prod;
		}
		return avoided_permn_ct;
	}
};


AvoidedPartialPermutations* pAvoided_partial_permns_;
class Permutations
{
private:
	mutex permutations_mutex_;
	queue<Permutation*> permutations_;	
	mutex min_permutation_mutex_;
	Permutation min_cost_permutation, initial_min_cost_permn_;
	
public:	
	permutation_ct_t total_processed_permns_ct_ = 0;
	Permutations(Permutation& initial_min_cost_permn) :min_cost_permutation(initial_min_cost_permn), initial_min_cost_permn_(initial_min_cost_permn){}
	void CheckSetMinCostPermutation(Permutation* pPermutation) // done in a single call
	{
		min_permutation_mutex_.lock();
		++total_processed_permns_ct_;
		if (pPermutation->permutation_cost < min_cost_permutation.permutation_cost)		
			min_cost_permutation = *pPermutation;
		delete pPermutation;
		min_permutation_mutex_.unlock();
	}
	void CheckAddPartialPermnGreaterThanMinCost(Permutation* pPermutation)
	{
		min_permutation_mutex_.lock();
		if (pPermutation->permutation_cost >= min_cost_permutation.permutation_cost)
		{
			pAvoided_partial_permns_->AddPartialPermutation(pPermutation->permutation);
			delete pPermutation;
		}
		else Push(pPermutation);
		min_permutation_mutex_.unlock();
	}
	void OutputMinCostPermutation()
	{
	   logger.LogInfo("Final Min Cost Permn: ", &min_cost_permutation);	
	   logger.LogInfo("Initial Min Cost Permn: ", &initial_min_cost_permn_);
	   logger.LogInfo("Difference: ", initial_min_cost_permn_.permutation_cost - min_cost_permutation.permutation_cost);
	}
	void Pop(Permutation* pPermutation)
	{
		permutations_mutex_.lock();
		if (permutations_.size() == 0)
			throw tsp_exception("Attempt to pop from empty set of permutations");
		
		pPermutation->permutation_cost = permutations_.front()->permutation_cost;
		pPermutation->permutation.resize(permutations_.front()->permutation.size());
		copy(permutations_.front()->permutation.begin(), permutations_.front()->permutation.end(), pPermutation->permutation.begin());		
		logger.LogDebugInfo("Popped: ", pPermutation);
		permutations_.pop();
		permutations_mutex_.unlock();
	}
	void Push(Permutation* pPermutation)
	{
		//permutations_mutex_.lock();
		permutations_.push(pPermutation);		
		logger.LogDebugInfo("Pushed: ", pPermutation);
		//permutations_mutex_.unlock();
	}
	void Empty(bool& bPermnsEmpty)
	{
		permutations_mutex_.lock();
		bPermnsEmpty = permutations_.empty();
		//if (bPermnsEmpty) logger.LogDebugInfo("Permutation set empty");
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

void Initialize_Edges();
void generate_and_test_permutations(Permutation* pPermutation);
void GenerateThreads();
void CheckNewPermn(permutation_t& newPermn, permutation_cost_t& newCost);
void compute_initial_min_cost_permutation();
void Convert_Time_Duration(long long Duration);
void term_func() {
	cout << "term_func was called by terminate." << endl;
	exit(-1);
}

#endif TCP_H_