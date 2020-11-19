#include<iostream>
#include<fstream>
#include<cassert>
#include<bits/stdc++.h>
typedef long long int ll;
using namespace std;

struct block
{
	bool present;
	ll address;
	bool modified;
};

struct qentry
{
	int tid;
	ll machineadd;
	ll globalcount;
	
};

//32 KB, 8-way, 64-byte block size, LRU

vector<vector<block>> l10(64, vector<block>(8,{false,-1,false}));
vector<vector<block>> l11(64, vector<block>(8,{false,-1,false}));
vector<vector<block>> l12(64, vector<block>(8,{false,-1,false}));
vector<vector<block>> l13(64, vector<block>(8,{false,-1,false}));
vector<vector<block>> l14(64, vector<block>(8,{false,-1,false}));
vector<vector<block>> l15(64, vector<block>(8,{false,-1,false}));
vector<vector<block>> l16(64, vector<block>(8,{false,-1,false}));
vector<vector<block>> l17(64, vector<block>(8,{false,-1,false}));

//4 MB, 16-way, 64-byte block size, LRU,

vector<vector<block>> l30(2048, vector<block>(16,{false,-1,false}));
vector<vector<block>> l31(2048, vector<block>(16,{false,-1,false}));
vector<vector<block>> l32(2048, vector<block>(16,{false,-1,false}));
vector<vector<block>> l33(2048, vector<block>(16,{false,-1,false}));
vector<vector<block>> l34(2048, vector<block>(16,{false,-1,false}));
vector<vector<block>> l35(2048, vector<block>(16,{false,-1,false}));
vector<vector<block>> l36(2048, vector<block>(16,{false,-1,false}));
vector<vector<block>> l37(2048, vector<block>(16,{false,-1,false}));
vector<list<ll>> l2lru(1024);
vector<list<ll>> l3lru(2048);
ll l2hit,l2miss, l3hit,l3miss;

//input queue for l1 cache
list<ll> q0;
queue<qentry> q1;
queue<qentry> q2;
queue<qentry> q3;
queue<qentry> q4;
queue<qentry> q5;
queue<qentry> q6;
queue<qentry> q7;


int main(int argc, char const *argv[])
{
	FILE *fp;
	int tid,rw;
	ll machine_add,gc;

	fp = fopen(argv[1], "rb");
	while (!feof(fp)) 
      	{
        	fread(&tid, sizeof(int), 1, fp);
        	fread(&machine_add, sizeof(ll), 1, fp);
        	fread(&rw, sizeof(int), 1, fp);
        	fread(&gc, sizeof(ll), 1, fp);

            // Process the entry
			//addr = addr>>6;
			if(tid == 0){
				//q0.tid = tid;
				q0.push_back(machine_add);
				//q0.globalcount = gc;
			}/*else if(tid == 1){
				q0.tid = tid;
				q0.machineadd = machine_add;
				q0.globalcount = gc;
			}*/
	
      	}
      	fclose(fp);
      	cout<<q0.size()<<endl;
	return 0;
}