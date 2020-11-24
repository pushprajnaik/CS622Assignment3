#include<iostream>
#include<fstream>
#include<cassert>
#include<bits/stdc++.h>
typedef long long int ll;
using namespace std;

//status M =0, E=1, S=2, I is captured through present
struct block
{
	bool present;
	ll address;
	bool modified;
	//int status;
};

struct qentry
{
	int tid;
	ll machineadd;
	ll globalcount;
	
};

/*msg code 
	getX - 0
	get  - 1
	putX - 2

*/
enum msg
{
	getX,get,putX,put,inv,invack
	
};
struct outentry
{
	int tid;
	ll machineadd;
	ll globalcount;
	enum msg dir_msg;
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
//vector<list<ll>> l2lru(1024);
//vector<list<ll>> l3lru(2048);
//ll l2hit,l2miss, l3hit,l3miss;

//variables 
ll sim_cycle, l1_access[8], l1_miss[8], l2_miss[8], global_consumed=-1;

//input queue for l1 cache
vector<list<qentry>> l1_in_queue(8);
/*list<qentry> q1;
list<qentry> q2;
list<qentry> q3;
list<qentry> q4;
list<qentry> q5;
list<qentry> q6;
list<qentry> q7;
*/
// lru for l1 
std::vector<vector<list<ll>> > lru_l1(8, vector<list<ll>>(64));
std::vector<vector<list<ll>> > lru_l2(8, vector<list<ll>>(2048));
 
 // output queue for l1 caches
std::vector<list<outentry>> l1_out_queue(8);

//output queue for l2 caches
std::vector<list<outentry>> l2_out_queue(8);

//input queue for l2 caches
std::vector<list<outentry>> l2_in_queue(8);

//l1 cache miss buffer
std::vector<list<outentry>> miss_buff_l1(8);

bool compareInterval(qentry i1, qentry i2){
	return(i1.machineadd < i2.machineadd);
}

//------------------***********--------------------------------
// core 0
bool hit_miss_l10(ll address) 
{
	l1_access[0]++;	
	address = address >>6;
    int set  =  address % 64;
    for (int i=0;i<8;i++) 
    {
        if((l10[set][i].address == address) && (l10[set][i].present))
            return true;
    }
    return false;
}
//core 1




//----------------------****************------------------

ll find_bank(ll address){
	address = address>>6;
	ll bank_id = address & 0x7;
	return bank_id;
}


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
			qentry entry;
			entry.tid = tid;
			entry.machineadd = machine_add;
			entry.globalcount = gc;
			if(tid == 0){
				//q0.tid = tid;
				l1_in_queue[0].push_back(entry);
				//q0.globalcount = gc;
			}else if(tid == 1){
				l1_in_queue[1].push_back(entry);
			}else if(tid ==2)
				l1_in_queue[2].push_back(entry);
			else if (tid == 3) l1_in_queue[3].push_back(entry);
			else if(tid ==4 ) l1_in_queue[4].push_back(entry);
			else if(tid ==5 ) l1_in_queue[5].push_back(entry);
			else if(tid ==6 ) l1_in_queue[6].push_back(entry);
			else if(tid ==7 ) l1_in_queue[7].push_back(entry);
	
      	}
      	fclose(fp);
      	//cout<<q0.size()<<endl<<q1.size()<<endl<<q2.size()<<endl<<q3.size()<<endl<<q4.size()<<endl<<q5.size()<<endl<<q6.size()<<endl<<q7.size()<<endl;

      	while(!l1_in_queue[0].empty() || !l1_in_queue[1].empty()|| !l1_in_queue[2].empty() || !l1_in_queue[3].empty()|| !l1_in_queue[4].empty() || !l1_in_queue[5].empty()||!l1_in_queue[6].empty() || !l1_in_queue[7].empty()){
      		sim_cycle++;
      		qentry temp_madd[8] = {0};
      		for(int i=0;i<8;i++){
      			qentry tempentry;
      			tempentry = l1_in_queue[i].front();
      			l1_in_queue[i].pop_front(); // update ***********
      			temp_madd[i] = tempentry;
      		}
      		sort(temp_madd,temp_madd+8, compareInterval);
      		
      		qentry this_cycle[8];
      		//ll mini_count;
      		int i=0;
      		while(i<8 && (temp_madd[i].globalcount == global_consumed+1)){
      			this_cycle[i] = temp_madd[i];
      			global_consumed += 1;
      			i++;
      		}//----------------------------


      		int j=0;
      		while(i>0){
      			int tid;
      			tid = this_cycle[j].tid;
      			if(tid == 0){
      				if(! hit_miss_l10(this_cycle[j].machineadd)){
      					l1_miss[0]++;
      					ll bid = find_bank(this_cycle[j].machineadd);
      					//put this request in respective l2 cache bank queue according to read write pass message.
      				}
      			} 
      			else if (tid == 1) //hit_miss_l11(this_cycle[j].machineadd);
      			else if (tid == 2) //hit_miss_l12(this_cycle[j].machineadd);
      			else if (tid == 3) //hit_miss_l13(this_cycle[j].machineadd);
      			else if (tid == 4) //hit_miss_l14(this_cycle[j].machineadd);
      			else if (tid == 5) //hit_miss_l15(this_cycle[j].machineadd);
      			else if (tid == 6) //hit_miss_l16(this_cycle[j].machineadd);
      			else if (tid == 7) //hit_miss_l17(this_cycle[j].machineadd);
      		}//-----------------------------------

      		//process output queue of each l2 cache.

      		
      		break;
      	}


	return 0;
}
