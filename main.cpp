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
	int read_write;
};


enum msg
{
	getX,getmsg,putX,put,inv,invack,swb,ack_getx
	
};
struct outentry
{
	int tid;
	ll machineadd;
	ll globalcount;
	int rw;
	enum msg dir_msg;
	int how_many_ack;
};

struct dir_entry
{
	bool dirty;
	bool present;
	ll address;
	bitset<8> sharer;
};

//32 KB, 8-way, 64-byte block size, LRU

vector<vector<vector<block>>> l1(8,vector<vector<block>>(64, vector<block>(8,{false,-1,false})));

//4 MB, 16-way, 64-byte block size, LRU,

vector<vector<vector<dir_entry>>> l2(8,vector<vector<dir_entry>>(2048, vector<dir_entry>(16,{false,false,0,0})));

//----------------LRU FOR L1 AND L2 CACHES-----------------------

std::vector<vector<list<ll>> > lru_l1(8, vector<list<ll>>(64));
std::vector<vector<list<ll>> > lru_l2(8, vector<list<ll>>(2048));

//----------------------DIRECTORY FOR L2 CACHE BANKS---------------------



//variables 
ll sim_cycle, l1_access[8], l1_miss[8], l2_miss[8], global_consumed=-1;

//input queue for l1 cache
vector<list<qentry>> l1_in_queue(8);


 // other input queue for l1 caches
std::vector<list<outentry>> l1_out_queue(8);

//output queue for l2 caches
//std::vector<list<outentry>> l2_out_queue(8);

//input queue for l2 caches
std::vector<list<outentry>> l2_in_queue(8);

//l1 cache miss buffer
std::vector<list<outentry>> miss_buff_l1(8);

bool compareInterval(qentry i1, qentry i2){
	return(i1.tid < i2.tid);
}
//--------------------BITVECOTR TO INT------------------------------------
int btoi(bitset<8>bv){
	int po2 =1,k=0,res=0;
	while(k<8){
		if(bv.test(k)){
			res+= po2;
		}
		po2 = po2<<1;
		k++;
	}
	return res;
}
//-----------------SET DIRTY TO ZERO------------------------------------------
void set_dirty_zero(ll address, int tid) 
{
	//l1_access[tid]++;	
	address = address >>6;
    int set  =  address % 2048;
    for (int i=0;i<16;i++) 
    {
        if((l2[tid][set][i].address == address) && (l2[tid][set][i].present))
            l2[tid][set][i].dirty = 0;
    }
   
}

//------------------HIT MISS FUNCTION OF ALL THE CACHES--------------------------------

bool hit_miss_l1(ll address, int tid) 
{
	l1_access[tid]++;	
	address = address >>6;
    int set  =  address % 64;
    for (int i=0;i<8;i++) 
    {
        if((l1[tid][set][i].address == address) && (l1[tid][set][i].present))
            return true;
    }
    return false;
}
//------------------HIT MISS L2---------------------------------------------

int hit_miss_l2(ll address, int tid) 
{
	//l1_access[tid]++;	
	address = address >>6;
    int set  =  address % 2048;
    for (int i=0;i<16;i++) 
    {
        if((l2[tid][set][i].address == address) && (l2[tid][set][i].present))
            return l2[tid][set][i].dirty;
    }
    return -1;
}

//----------------------UPDATE L1 LRU ON L1 HIT OF ALL CACHES------------------

void update_on_hit(ll address, int tid) 
{
	address = address>>6;
    int set  =  address % 64;
    // Move the block to the top of LRU set
    lru_l1[tid][set].remove(address);
    lru_l1[tid][set].push_front(address);
    //return;
}


//-------------------FIND BANK ID---------------------------

ll find_bank(ll address){
	address = address>>6;
	ll bank_id = address & 0x7;
	return bank_id;
}
//---------------------SETTING SHARER---------------------------
void set_sharer(int k, ll address, int tid){
	address = address>>6;
	int set = address % 2048;
	int i;
	for(i=0;i<16;i++){
		if(((l2[k][set][i].address == address) && l2[k][set][i].present)){
			l2[k][set][i].sharer.set(tid);
			break;
		}
	}
	//cout<<l2[k][set][i].sharer<<endl;
}

//------------------GET SHARER---------------------------------
bitset<8> get_sharer(int k, ll address){
	address = address>>6;
	int set = address % 2048;
	int i;
	for(i=0;i<16;i++){
		if(((l2[k][set][i].address == address) && l2[k][set][i].present)){
			return l2[k][set][i].sharer;
		
		}
	}
	abort();
	
}
//------------------------SETTING OWNER---------------------------
void set_owner(int k, ll address, int tid){
	address = address>>6;
	int set = address % 2048;
	int i;
	for(i=0;i<16;i++){
		if(((l2[k][set][i].address == address) && l2[k][set][i].present)){
			l2[k][set][i].sharer = tid;
			break;
		}
	}
	//cout<<l2[k][set][i].sharer<<endl;
}


//---------------------CURRENT CYCLE FUNCTION FOR L1--------------

void current_cycle(ll machineadd, ll globalcount, int tid, int read_write){


	if(! hit_miss_l1(machineadd, tid)){ // if miss in l1 cache.
      					l1_miss[tid]++;
      					ll bid = find_bank(machineadd);

      					//cout<<bid<<"<--bank id \n";
      					//put this request in respective l2 cache bank queue according to read write pass message.
      					//cout<<this_cycle[j].read_write<<"read write\n";
      					if(read_write == 0){
      						// this means its a read miss --> send a get request to respective home l2 cache;
      						//cout<<"read\n";
      						outentry send;
      						send.tid = tid;
      						send.machineadd = machineadd;
      						send.globalcount = globalcount;
      						send.dir_msg  = getmsg;
      						send.rw = read_write;
      						l2_in_queue[bid].push_back(send);

      					}else{
      						//this is a write miss -> send a getx to ...
      						outentry send;
      						send.tid = tid;
      						send.machineadd = machineadd;
      						send.globalcount = globalcount;
      						send.dir_msg  = getX;
      						send.rw = read_write;
      						l2_in_queue[bid].push_back(send);

      					}

      				}else{
      					//if hit in l1 cache;
      					//1. update l1 lru
      					update_on_hit(machineadd, tid);

      				}


}
//----------------------ADD BLOCK TO L2 CACHE ON L2 MISS----------------------
ll add_block_l2(ll address, int k) 
{	
	address = address>>6;
    int set  =  address % 2048;
    // Check if a way is free in the set
    
    for (int i=0;i<16;i++) 
    {
        if (l2[k][set][i].present) 
        	continue;
        // Found an empty slot
        l2[k][set][i].present = true;
        l2[k][set][i].address = address;
        lru_l2[k][set].push_front(address);
       // cout<<"found empty slot in l2 and added\n";
        return 0;
    }
    // All 'ways' in the set are valid, evict one
    ll evict_block = lru_l2[k][set].back();
    lru_l2[k][set].pop_back();

    for (int i=0;i<16;i++) 
    {
        if (l2[k][set][i].address != evict_block)
        	continue;
        // Found the block to be evicted
        l2[k][set][i].address = address;
        lru_l2[k][set].push_front(address);
        return evict_block;
    }
    abort(); 

}
//----------------------ADD BLOCK L1---------------------------------------
ll add_block_l1(ll address, int tid) 
{
	address = address>>6;
    int set  =  address % 64;
    // Check if a way is free in the set
    for (int i=0;i<8;i++) 
    {
        if (l1[tid][set][i].present) 
        	continue;
        // Found an empty slot
        
        l1[tid][set][i].present = true;
        l1[tid][set][i].address = address;
        lru_l1[tid][set].push_front(address);
        //cout<<"found empty slot in l1 and added\n";
        return 0;
    }
    
    // All 'ways' in the set are valid, evict one
    ll evict_block = lru_l1[tid][set].back();
    lru_l1[tid][set].pop_back();

    for (int i=0;i<8;i++) 
    {
        if (l1[tid][set][i].address != evict_block) 
        	continue;
        
        // Found the block to be evicted
        l1[tid][set][i].address = address;
        lru_l1[tid][set].push_front(address);
        return evict_block;
    }
    abort(); 
}

//-----------------------INVALIDATE L1 CACHE BLK WHEN EVICTED BLK FROM L2 IN PRESENT IN L1;-------------------
void invalidate_block_l1(ll address, int tid) 
{
	address = address>>6;
    int set  =  address % 64;
    for (int i=0;i<8;i++) 
    {
        if (l1[tid][set][i].address != address) 
        	continue;
        // Found the block; Invalidate it
        l1[tid][set][i].present = false;
        lru_l1[tid][set].remove(address);
        return;
    }
    abort(); 
}



int main(int argc, char const *argv[])
{
	int termi =1000000;
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
			entry.read_write = rw;
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


      	cout<<"--------------------------------------------------\n";

		for(int i=0;i<8;i++){
			if(!l1_in_queue[i].empty())
      		  cout<<i<<"-->"<<l1_in_queue[i].size()<<endl;
      	}
      	

      	while(!l1_in_queue[0].empty() || !l1_in_queue[1].empty()|| !l1_in_queue[2].empty() || !l1_in_queue[3].empty()|| !l1_in_queue[4].empty() || !l1_in_queue[5].empty()||!l1_in_queue[6].empty() || !l1_in_queue[7].empty()){
      		sim_cycle++;
      		qentry temp_madd[8] = {0};
      		qentry tempentry;
      		for(int i=0;i<8;i++){
      			
      			tempentry = l1_in_queue[i].front();
      			l1_in_queue[i].pop_front(); 
      			temp_madd[i] = tempentry;
      			
      		}

      		// sort the temp_madd according to global count
      		sort(temp_madd,temp_madd+8, compareInterval);


      		//for debug
      		//for(int i=0;i<8;i++) cout<<temp_madd[i].machineadd<<endl<<temp_madd[i].globalcount<<endl;
      		
      		qentry this_cycle[8];
      		//ll mini_count;
      		int i=0;
      		while(i<8 && (temp_madd[i].globalcount == global_consumed+1)){
      			//cout<<"inside loop 1\n";
      			this_cycle[i] = temp_madd[i];
      			global_consumed += 1;
      			i++;
      		}
      		int i1 =i;

      		while(i != 8){

      			int tid = temp_madd[i].tid;
      			// return to respective input queue
      			l1_in_queue[tid].push_front(temp_madd[i]);
      			i++;

      		}

      		int j=0;
      		while(i1>0){		// not terminating loop, update value of i and j;
      			//cout<<"inside loop 2\n";	
      			int tid;
      			tid = this_cycle[j].tid;
      			if(tid == 0){
      				current_cycle(this_cycle[j].machineadd, this_cycle[j].globalcount, this_cycle[j].tid, this_cycle[j].read_write);
      			} 
      			else if (tid == 1){
      				current_cycle(this_cycle[j].machineadd, this_cycle[j].globalcount, this_cycle[j].tid, this_cycle[j].read_write);
      			} 
      			else if (tid == 2){
      				current_cycle(this_cycle[j].machineadd, this_cycle[j].globalcount, this_cycle[j].tid, this_cycle[j].read_write);
      			} 
      			else if (tid == 3){
      				current_cycle(this_cycle[j].machineadd, this_cycle[j].globalcount, this_cycle[j].tid, this_cycle[j].read_write);
      			} 
      			else if (tid == 4){
      				current_cycle(this_cycle[j].machineadd, this_cycle[j].globalcount, this_cycle[j].tid, this_cycle[j].read_write);
      			} 
      			else if (tid == 5) {
      				current_cycle(this_cycle[j].machineadd, this_cycle[j].globalcount, this_cycle[j].tid, this_cycle[j].read_write);
      			}
      			else if (tid == 6) {
      				current_cycle(this_cycle[j].machineadd, this_cycle[j].globalcount, this_cycle[j].tid, this_cycle[j].read_write);
      			}
      			else if (tid == 7){
      				current_cycle(this_cycle[j].machineadd, this_cycle[j].globalcount, this_cycle[j].tid, this_cycle[j].read_write);
      			}
      	
      				j++;i1--;
      		}

      		//for debug
      		//cout<<l2_in_queue[7].front().machineadd<<endl;
      		//cout<<l2_in_queue[7].front().dir_msg<<endl;
      		//cout<<l2_in_queue[7].front().tid<<endl;
      		//cout<<l2_in_queue[7].front().globalcount<<endl;

      		// process other queue of l1 (if not empty) && now process l2 input queue (if not empty) 
      		outentry ohterinput[8], l2input[8];
      		int k=0;

      		//cout<<"l1 out queue size "<<l1_out_queue[5].size()<<endl;
      		while(k<8){

      			if (! l1_out_queue[k].empty()){

      				//debug
      				cout<<"reaching inside owner to requester\n";
      				ohterinput[k] = l1_out_queue[k].front();

      				ll machineadd_ = ohterinput[k].machineadd;
      				int tid_ = ohterinput[k].tid;
      				ll globalcount_ = ohterinput[k].globalcount;
      				int rw_ = ohterinput[k].rw;
      				
      				outentry send;
      				send.tid = tid_;
      				send.machineadd = machineadd_;
      				send.globalcount = globalcount_;
      				send.rw = rw_; 

      				//1. send SWB to home bank and put message to original requester
      				if(ohterinput[k].dir_msg == getmsg){

	      				int home = find_bank(machineadd_);
	      				
	      				send.dir_msg = swb;

	      				l2_in_queue[home].push_back(send);

	      				send.dir_msg = put;

	      				l1_out_queue[tid_].push_back(send);
	      			}
      			

      				//2. send a ack to home bank and send putX to requester

      				if(ohterinput[k].dir_msg == getX){

      					int home = find_bank(machineadd_);
	      				
	      				send.dir_msg = ack_getx;

	      				l2_in_queue[home].push_back(send);

	      				send.dir_msg = putX;

	      				l1_out_queue[tid_].push_back(send);
      				}

      				//3. send invack to requester and 

      				if(ohterinput[k].dir_msg == inv){

      					send.dir_msg = invack;

	      				l1_out_queue[tid_].push_back(send);

      				}

      				//cout<<"l1_out_queue not empty";
      				l1_out_queue[k].pop_front();
      			}

      			if(! l2_in_queue[k].empty()){
      				//cout<<"home to requester and owner\n";
      				l2input[k] = l2_in_queue[k].front();
      				
      				//process this entry from l2 input queue and then remove it 
      				// check if dirty is set or not

      				ll machineadd_ = l2input[k].machineadd;
      				int tid_ = l2input[k].tid;
      				ll globalcount_ = l2input[k].globalcount;
      				int rw_ = l2input[k].rw;

      				int d = hit_miss_l2(l2input[k].machineadd, k);//cout<<"here 1-->\n"<<"dirty bit value "<<d<<endl;

      				if ( d == 0){
      					//dirty bit is notset
      					if(l2input[k].rw == 0){
      						//read
      						//send put and set bit corresponding to k(means sharer) in bitvector
      						set_sharer(k, l2input[k].machineadd, l2input[k].tid);
      						//debug
      						cout<<"here after sharer\n";

      						outentry send;
      						send.tid = l2input[k].tid;
      						send.machineadd = l2input[k].machineadd;
      						send.globalcount = l2input[k].globalcount;
      						send.rw = l2input[k].rw;
      						send.dir_msg = put;
      						l1_out_queue[l2input[k].tid].push_back(send); //send msg from home bank to requester l1 cache.



      					}else{
      						//write
      						//find sharer from bit vector and send them invalidation; make bitvector  = k;(owner) ;
      						//1. read bitvector and send inv to all sharer
      						
      						outentry send;
      						send.tid = tid_;
      						send.machineadd = machineadd_;
      						send.globalcount = globalcount_;
      						send.rw = rw_;
      						send.dir_msg = inv;

      						bitset<8> temp = get_sharer(k, machineadd_);
      						int inv_to_who = 0;
      						int count_inv =0;

      						while(inv_to_who < 8){
      							if (temp.test(inv_to_who)){
      								//send inv to inv_to_who
      								l1_out_queue[inv_to_who].push_back(send); // **TODO also send how many acks to expect...?????
      								count_inv++;
      							}
      							inv_to_who++;
      						}

      						//2. set bit vector to store k as owner
      						set_owner(k,machineadd_,tid_);

      						//3. send putX to k
      						send.dir_msg = putX;
      						send.how_many_ack = count_inv;

      						l1_out_queue[tid_].push_back(send);

      					}
      				}else if(d == 1){
      					//dirty bit is  set
      					if(l2input[k].rw == 0){
      						//read
      						// send get request to current owner and change bit vector--> make both k and owner sharers of blk;
      						//1.forward get request to current owner 
      						bitset<8> curr_owner_bit = get_sharer(k, machineadd_);
      						int curr_owner = btoi(curr_owner_bit);
      						outentry send;
      						send.tid = tid_;
      						send.machineadd = machineadd_;
      						send.globalcount =globalcount_;
      						send.rw = rw_;
      						send.dir_msg = getmsg;					//** TODO not implemented pending state.

      						l1_out_queue[curr_owner].push_back(send);

      						//2.set dirty bit to zero and set bitvector to store k and curr_owner as sharer;
      						set_dirty_zero(machineadd_, k);
      						set_sharer(k, machineadd_, tid_);
      						set_sharer(k, machineadd_, curr_owner);

      					}else{
      						//write
      						//;send getx to current owner and set bitvector such that k is new owner;
      						bitset<8> curr_owner_bit = get_sharer(k, machineadd_);
      						int curr_owner = btoi(curr_owner_bit);
      						outentry send;
      						send.tid = tid_;
      						send.machineadd = machineadd_;
      						send.globalcount =globalcount_;
      						send.rw = rw_;
      						send.dir_msg = getX;

      						l1_out_queue[curr_owner].push_back(send);

      						//2. set tid_ as new owner 
      						set_owner(k,machineadd_,tid_);

      					}

      				}else{
      					// blk not present in l2 cache i.e. l2 cache miss.
      					//cout<<"l2 miss\n";
      					l2_miss[k]++;
      					ll evicted = add_block_l2(machineadd_, k);
      					 if (evicted && hit_miss_l1(evicted, tid_)){
            				invalidate_block_l1(evicted, tid_);
            			}
             				add_block_l1(machineadd_, tid_);
             				//cout<<"reached here\n";

      				}

      				l2_in_queue[k].pop_front();
      			}
      			//cout<<"l1 out queue size "<<l1_out_queue[5].size()<<endl;
      			k++;
      		}
      		//cout<<"out of loop\n";

      		/*int u=0;
      		while(size_otherinput--){
      			// process one by one;

      		}

      		int n=0;
      		while(size_l2input--){
      			//process one by one
      			outentry l2process;
      			l2process.machineadd = l2input[k].machineadd;
      			l2process.dir_msg = l2input[k].dir_msg;
      			if(l2process.dir_msg == getmsg && )
      		}*/

      		

      	




      		if(termi == 0)
      		break;
      	termi--;
      	}

      	/*for(int i=0;i<8;i++){
      		cout<<"l1 access of core "<<i<<" "<<l1_access[i]<<endl;
      	}
      	for(int i=0;i<8;i++){
      		cout<<"l1 miss of core "<<i<<" "<<l1_miss[i]<<endl;
      	}
      	for(int i=0;i<8;i++){
      		cout<<"l2 miss of core "<<i<<" "<<l2_miss[i]<<endl;
      	}
      	cout<<"simulated cycles "<<sim_cycle<<endl;
      	*/

      	cout<<"--------------------------------------------------\n";

		for(int i=0;i<8;i++){
			if(! l1_in_queue[i].empty())
      		cout<<i<<" -->" <<l1_in_queue[i].size()<<endl;
      	}

      	cout<<"--------------------------------------------------\n";

		for(int i=0;i<8;i++){
      		cout<<l2_in_queue[i].empty()<<endl;
      	}
      	cout<<"--------------------------------------------------\n";

		for(int i=0;i<8;i++){
      		cout<<l1_out_queue[i].empty()<<endl;
      	}


	return 0;
}
