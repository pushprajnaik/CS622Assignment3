#include<iostream>
#include<fstream>
#include<cassert>
#include<bits/stdc++.h>
typedef long long int ll;
using namespace std;


enum status
{
  M, E, S, I
};
struct block
{
  bool present;
  ll address;
  bool modified;
  enum status status_;
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
  getX, getmsg, putX, put, inv, invack, swb, ack_getx, upgrd
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
    bitset < 8 > sharer;
  enum status status_;		// only possible status are M, I, S //can't differentiate b/w M and E;
};

//32 KB, 8-way, 64-byte block size, LRU

vector < vector < vector < block >>> l1 (8, vector < vector < block >> (64, vector < block > (8, { false, -1, false, I})));

//4 MB, 16-way, 64-byte block size, LRU,

vector < vector < vector < dir_entry >>> l2 (8, vector < vector < dir_entry >> (2048, vector < dir_entry > (16,{false,false,0,0})));

//----------------LRU FOR L1 AND L2 CACHES-----------------------

std::vector < vector < list < ll >> >lru_l1 (8, vector < list < ll >> (64));
std::vector < vector < list < ll >> >lru_l2 (8, vector < list < ll >> (2048));


//------------------l1_msg_count and l2_msg_count------------------------------------
std::map<string, ll> l1_msg_count, l2_msg_count;


//variables 
ll sim_cycle, l1_access[8], l1_miss[8], l2_miss[8], global_consumed = -1;

//input queue for l1 cache
vector < list < qentry >> l1_in_queue (8);


// other input queue for l1 caches
std::vector < list < outentry >> l1_out_queue (8);


//input queue for l2 caches
std::vector < list < outentry >> l2_in_queue (8);

//l1 cache miss buffer
std::vector < list < outentry >> miss_buff_l1 (8);

bool compareInterval (qentry i1, qentry i2)
{
  return (i1.globalcount < i2.globalcount);
}

//--------------------SET L2 STATUS------------------------------------
void set_l2_status (ll address, int tid, status set_this)
{
  address = address >> 6;
  int set = address % 2048;
  for (int i = 0; i < 16; i++)
    {
      if ((l2[tid][set][i].address == address) && (l2[tid][set][i].present))
	l2[tid][set][i].status_ = set_this;
    }

}

//--------------------SET L1 STATUS------------------------------------
void set_l1_status (ll address, int tid, status set_this)
{
  address = address >> 6;
  int set = address % 64;
  for (int i = 0; i < 8; i++)
    {
      if ((l1[tid][set][i].address == address) && (l1[tid][set][i].present))
	l1[tid][set][i].status_ = set_this;
    }

}

//--------------------READ L2 STATUS-----------------------------------
status read_l2_status (ll address, int tid)
{
  address = address >> 6;
  int set = address % 2048;
  for (int i = 0; i < 16; i++)
    {
      if ((l2[tid][set][i].address == address) && (l2[tid][set][i].present))
	return l2[tid][set][i].status_;
    }
  return I;
}

//-------------------READ L1 STATUS------------------------------------
status read_l1_status (ll address, int tid)
{
  status current;
  address = address >> 6;
  int set = address % 64;
  for (int i = 0; i < 8; i++)
    {
      if ((l1[tid][set][i].address == address) && (l1[tid][set][i].present))
	return l2[tid][set][i].status_;
    }
  return I;
}

//--------------------BITVECOTR TO INT------------------------------------
int btoi (bitset < 8 > bv)
{
  int po2 = 1, k = 0, res = 0;
  while (k < 8)
    {
      if (bv.test (k))
	{
	  res += po2;
	}
      po2 = po2 << 1;
      k++;
    }
  return res;
}

//-----------------SET DIRTY TO ZERO------------------------------------------
void set_dirty_zero (ll address, int tid)
{
  //l1_access[tid]++;     
  address = address >> 6;
  int set = address % 2048;
  for (int i = 0; i < 16; i++)
    {
      if ((l2[tid][set][i].address == address) && (l2[tid][set][i].present))
	l2[tid][set][i].dirty = 0;
    }

}

//------------------HIT MISS FUNCTION OF ALL THE CACHES--------------------------------

bool hit_miss_l1 (ll address, int tid)
{
  l1_access[tid]++;
  address = address >> 6;
  int set = address % 64;
  for (int i = 0; i < 8; i++)
    {
      if ((l1[tid][set][i].address == address) && (l1[tid][set][i].present))
	return true;
    }
  return false;
}

//------------------HIT MISS L2---------------------------------------------

int hit_miss_l2 (ll address, int tid)
{
  //l1_access[tid]++;     
  address = address >> 6;
  int set = address % 2048;
  for (int i = 0; i < 16; i++)
    {
      if ((l2[tid][set][i].address == address) && (l2[tid][set][i].present))
	return true;
    }
  return false;
}

//----------------------UPDATE L1 LRU ON L1 HIT OF ALL CACHES------------------

void update_on_hit (ll address, int tid)
{
  address = address >> 6;
  int set = address % 64;
  // Move the block to the top of LRU set
  lru_l1[tid][set].remove (address);
  lru_l1[tid][set].push_front (address);
}


//-------------------FIND BANK ID---------------------------

ll find_bank (ll address)
{
  address = address >> 6;
  ll bank_id = address & 0x7;
  return bank_id;
}

//---------------------SETTING SHARER---------------------------
void set_sharer (int k, ll address, int tid)
{
  address = address >> 6;
  int set = address % 2048;
  int i;
  for (i = 0; i < 16; i++)
    {
      if (((l2[k][set][i].address == address) && l2[k][set][i].present))
	{
	  l2[k][set][i].sharer.set (tid);
	  break;
	}
    }
}

//------------------"GET" SHARER---------------------------------
bitset < 8 > get_sharer (int k, ll address)
{
  address = address >> 6;
  int set = address % 2048;
  int i;
  for (i = 0; i < 16; i++)
    {
      if (((l2[k][set][i].address == address) && l2[k][set][i].present))
	{
	  return l2[k][set][i].sharer;

	}
    }
  abort ();

}

//------------------------SETTING OWNER---------------------------
void
set_owner (int k, ll address, int tid)
{
  address = address >> 6;
  int set = address % 2048;
  int i;
  for (i = 0; i < 16; i++)
    {
      if (((l2[k][set][i].address == address) && l2[k][set][i].present))
	{
	  l2[k][set][i].sharer = tid;
	  break;
	}
    }
}


//---------------------CURRENT CYCLE FUNCTION FOR L1--------------

void current_cycle (ll machineadd, ll globalcount, int tid, int read_write)
{
  outentry send;
  send.tid = tid;
  send.machineadd = machineadd;
  send.globalcount = globalcount;
  send.rw = read_write;

  ll bid = find_bank (machineadd);
  status l1_status = read_l1_status(machineadd, tid);
  if (!hit_miss_l1 (machineadd, tid))
    {
    	// if miss in l1 cache.
    	 l1_miss[tid]++;
    	 //request from respective bank according to read/ write
    	 if(read_write == 0)
    	 {
    	 	send.dir_msg = getmsg;
    	 	l2_in_queue[bid].push_front(send);
    	 	l2_msg_count["GET"]++;
    	 }
    	 else
    	 {
    	 	send.dir_msg = getX;
			l2_in_queue[bid].push_back (send);
			l2_msg_count["GETX"]++;
    	 }
    }
    
  else
    {
    	if(l1_status == 1 && read_write == 1)
    	{
              set_l1_status(machineadd, tid, M);
              //update lru also;
              update_on_hit(machineadd, tid);
        }
        if(l1_status == 2 && read_write == 1)
        {
              send.dir_msg = upgrd;
              l2_in_queue[bid].push_back(send);
              l2_msg_count["UPGRADE"]++;
              update_on_hit(machineadd, tid);
        }                                   
        if(l1_status == 3 && read_write == 0)
        {
              send.dir_msg = getmsg;
              l2_in_queue[bid].push_back(send);
              l2_msg_count["GET"]++;
        }                                                                       
        if(l1_status == 3 && read_write == 1)
        {
              send.dir_msg = getX;
              l2_in_queue[bid].push_back(send);
              l2_msg_count["GETX"]++;
              
        }
        if(l1_status == 0 )
        {
        	 update_on_hit(machineadd, tid);
        }                 
                                    
      

    }


}

//----------------------ADD BLOCK TO L2 CACHE ON L2 MISS----------------------
ll add_block_l2 (ll address, int k)
{
  address = address >> 6;
  int set = address % 2048;
  // Check if a way is free in the set

  for (int i = 0; i < 16; i++)
    {
      if (l2[k][set][i].present)
	continue;
      // Found an empty slot
      l2[k][set][i].present = true;
      l2[k][set][i].address = address;
      lru_l2[k][set].push_front (address);
      return 0;
    }
  // All 'ways' in the set are valid, evict one
  ll evict_block = lru_l2[k][set].back ();
  lru_l2[k][set].pop_back ();

  for (int i = 0; i < 16; i++)
    {
      if (l2[k][set][i].address != evict_block)
	continue;
      // Found the block to be evicted
      l2[k][set][i].address = address;
      lru_l2[k][set].push_front (address);
      return evict_block;
    }
  abort ();

}

//----------------------ADD BLOCK L1---------------------------------------
ll add_block_l1 (ll address, int tid)
{
  address = address >> 6;
  int set = address % 64;
  // Check if a way is free in the set
  for (int i = 0; i < 8; i++)
    {
      if (l1[tid][set][i].present)
	continue;
      // Found an empty slot

      l1[tid][set][i].present = true;
      l1[tid][set][i].address = address;
      lru_l1[tid][set].push_front (address);
      return 0;
    }

  // All 'ways' in the set are valid, evict one
  ll evict_block = lru_l1[tid][set].back ();
  lru_l1[tid][set].pop_back ();

  for (int i = 0; i < 8; i++)
    {
      if (l1[tid][set][i].address != evict_block)
	continue;

      // Found the block to be evicted
      l1[tid][set][i].address = address;
      lru_l1[tid][set].push_front (address);
      return evict_block;
    }
  abort ();
}

//-----------------------INVALIDATE L1 CACHE BLK WHEN EVICTED BLK FROM L2 IN PRESENT IN L1;-------------------
void invalidate_block_l1 (ll address, int tid)
{
  address = address >> 6;
  int set = address % 64;
  for (int i = 0; i < 8; i++)
    {
      if (l1[tid][set][i].address != address)
	continue;
      l1[tid][set][i].present = false;		// Found the block; Invalidate it
      lru_l1[tid][set].remove (address);
      return;
    }
  abort ();
}



int main (int argc, char const *argv[])
{
  
  FILE *fp;
  int tid, rw;
  ll machine_add, gc;

  fp = fopen (argv[1], "rb");
  while (!feof (fp))
    {
      fread (&tid, sizeof (int), 1, fp);
      fread (&machine_add, sizeof (ll), 1, fp);
      fread (&rw, sizeof (int), 1, fp);
      fread (&gc, sizeof (ll), 1, fp);

      qentry entry;
      entry.tid = tid;
      entry.machineadd = machine_add;
      entry.globalcount = gc;
      entry.read_write = rw;
      
      l1_in_queue[tid].push_back(entry);

    }
  fclose (fp);

  while ((l1_in_queue[0].size () !=1) || !l1_in_queue[1].empty ()
	 || !l1_in_queue[2].empty () || !l1_in_queue[3].empty ()
	 || !l1_in_queue[4].empty () || !l1_in_queue[5].empty ()
	 || !l1_in_queue[6].empty () || !l1_in_queue[7].empty ())
    {
      sim_cycle++;
      std::vector<qentry> temp_madd;
      for (int i = 0; i < 8; i++)
	{
		if(! l1_in_queue[i].empty()){

		  temp_madd.push_back(l1_in_queue[i].front ());
		  l1_in_queue[i].pop_front ();
		}

	}
      
      sort (temp_madd.begin(), temp_madd.end(), compareInterval);		// sort the temp_madd according to global count
      std::vector<qentry> this_cycle;
      int i = 0;
      while (i < 8 && (temp_madd.front().globalcount == global_consumed + 1))
	{
	  this_cycle.push_back(temp_madd.front());
	  temp_madd.erase(temp_madd.begin());
	  global_consumed += 1;
	  i++;
	}

      int i1 = i;
      while (i != 8 && temp_madd.size())
	{

	  int tid = temp_madd.front().tid;
	  // return to respective input queue
	  l1_in_queue[tid].push_front (temp_madd.front());
	  temp_madd.erase(temp_madd.begin());
	  i++;
	}

      int j = 0;
      while (i1 > 0)
	{
	  
		ll machineadd_ = this_cycle.front().machineadd;
		ll globalcount_ = this_cycle.front().globalcount;
		int tid_ = this_cycle.front().tid;
		int read_write_ = this_cycle.front().read_write;
		current_cycle(machineadd_, globalcount_, tid_, read_write_);
		this_cycle.erase(this_cycle.begin());
	  i1--;
	}

      // process other queue of l1 (if not empty) && now process l2 input queue (if not empty) 
      outentry ohterinput[8], l2input[8];
      int k = 0;
      while (k < 8)
	{
	      ohterinput[k] = l1_out_queue[k].front ();
	      ll machineadd_ = ohterinput[7].machineadd;
	  if (!l1_out_queue[k].empty ())
	    {
	      ohterinput[k] = l1_out_queue[k].front ();
	      ll machineadd_ = ohterinput[k].machineadd;
	      int tid_ = ohterinput[k].tid;
	      ll globalcount_ = ohterinput[k].globalcount;
	      int rw_ = ohterinput[k].rw;
	      msg l1_msg = ohterinput[k].dir_msg;
	      status l1_status = read_l1_status (machineadd_, k);


	      outentry send;
	      send.tid = tid_;
	      send.machineadd = machineadd_;
	      send.globalcount = globalcount_;
	      send.rw = rw_;
	     
	      if (l1_msg == put && l1_status == 3)		 //1. reply of get --> change status to S;
		{
		  set_l1_status (machineadd_, k, S);
		}
	      if (l1_msg == getmsg && (l1_status == 1 || l1_status == 0))
		{

		  send.dir_msg = put;
		  l1_out_queue[tid_].push_back (send);
		  l1_msg_count["PUT"]++;

		  if (l1_status == 0)
		    {
		      send.dir_msg = swb;
		      l2_in_queue[find_bank (machineadd_)].push_back (send);
		      l2_msg_count["SWB"]++;
		    }

		  set_l1_status (machineadd_, k, S);
		}

	       
	      if (l1_msg == getX)		//2. reply of getX -->
		{
		  if (l1_status == 1)
		    {
		      send.dir_msg = putX;
		      set_l1_status (machineadd_, k, S);		
		    }
		  else if (l1_status == 0)
		    {
		      send.dir_msg = swb;
		      l2_in_queue[find_bank (machineadd_)].push_back (send);
		      l2_msg_count["SWB"]++;
		      send.dir_msg = putX;
		      l1_out_queue[tid_].push_back (send);
		      l1_msg_count["PUTX"]++;
		      set_l1_status (machineadd_, k, I);
		    }
		}

	      
	      if (l1_msg == inv)		//3. send invack to requester.
		{
		  send.dir_msg = invack;
		  l1_out_queue[tid_].push_back (send);
		  l1_msg_count["INVACK"]++;
		}
	      l1_out_queue[k].pop_front ();
	    }

	  if (!l2_in_queue[k].empty ())	//process this entry from l2 input queue and then remove it 
	      					// check if dirty is set or not
	    {
	      l2input[k] = l2_in_queue[k].front ();
	      ll machineadd_ = l2input[k].machineadd;
	      int tid_ = l2input[k].tid;
	      ll globalcount_ = l2input[k].globalcount;
	      int rw_ = l2input[k].rw;
	      status l2_status = read_l2_status (machineadd_, k);
	      msg l2_msg = l2input[k].dir_msg;	      
	      outentry send;
	      send.tid = tid_;
	      send.machineadd = machineadd_;
	      send.globalcount = globalcount_;
	      send.rw = rw_;
	      if (hit_miss_l2 (machineadd_, k))
		{		   
		  if (l2_msg == getmsg)		//1. if msg is get then 3 cases for each status M S I
		    {
		      if (l2_status == M)
			{
			  bitset < 8 > curr_owner_bit = get_sharer (k, machineadd_);
			  int curr_owner = btoi (curr_owner_bit);
			  send.dir_msg = getmsg;			  
			  l1_out_queue[curr_owner].push_back (send);	//1. forwarded get msg to current owner
			  l1_msg_count["GET"]++;
			  set_dirty_zero (machineadd_, k);	//2.set dirty bit to zero and set bitvector to store k and curr_owner as sharer;
			  set_l2_status (machineadd_, k, S);	// updating status from M to S;
			  set_sharer (k, machineadd_, tid_);
			  set_sharer (k, machineadd_, curr_owner);
			}
		      else if (l2_status == S)
			{			  
			  set_sharer (k, machineadd_, tid_);		//send put and set bit corresponding to k(means sharer) in bitvector
			  send.dir_msg = put;
			  l1_out_queue[tid_].push_back (send);	//send msg from home bank to requester l1 cache.
			  l1_msg_count["PUT"]++;
			}
		      else if (l2_status == I)
			{
			  set_sharer (k, machineadd_, tid_);

			  send.dir_msg = put;
			  set_l2_status (machineadd_, k, S);	// updating status from I to S;
			  l1_out_queue[tid_].push_back (send);
			  l1_msg_count["PUT"]++;
			}
		    }
		  if (l2_msg == getX)
		    {
		      if (l2_status == M)
			{			  
			  bitset < 8 > curr_owner_bit = get_sharer (k, machineadd_);		//1. check owner and send him getX
			  int curr_owner = btoi (curr_owner_bit);
			  send.dir_msg = getX;

			  l1_out_queue[curr_owner].push_back (send);
			  l1_msg_count["GETX"]++;			   
			  set_owner (k, machineadd_, tid_);	//2. set tid_ as new owner
			}
		      else if (l2_status == S)
			{
			  send.dir_msg = inv;

			  bitset < 8 > temp = get_sharer (k, machineadd_);
			  int inv_to_who = 0;
			  int count_inv = 0;

			  while (inv_to_who < 8)
			    {
			      if (temp.test (inv_to_who))
				{
				  //send inv to inv_to_who
				  l1_out_queue[inv_to_who].push_back (send);	
				  l1_msg_count["INV"]++;
				  count_inv++;
				}
			      inv_to_who++;
			    }

			  					
			  set_owner (k, machineadd_, tid_);	//2. set bit vector to store k as owner			  					
			  send.dir_msg = putX;			//3. send putX to k
			  send.how_many_ack = count_inv;
			  l1_out_queue[tid_].push_back (send);
			  l1_msg_count["PUTX"]++;
			  set_l2_status (machineadd_, k, M);	// updating status from S to M;
			}
		      else if (l2_status == I)
			{
			  set_owner (k, machineadd_, tid_);
			  send.dir_msg = putX;
			  set_l2_status (machineadd_, k, M);	// updating status from I to M;
			  l1_out_queue[tid_].push_back (send);
			  l1_msg_count["PUTX"]++;
			}
		    }
		  if (l2_msg == upgrd)
		    {
		      if (l2_status == S)
			{
			  set_owner (k, machineadd_, tid_); 	//1. set requester as new owner
			  set_l2_status (machineadd_, k, M);	//2. change l2 status from S to M
			  send.dir_msg = putX;
			  l1_out_queue[tid_].push_back (send);
			  l1_msg_count["PUTX"]++;
			}
		    }
		  l2_in_queue[k].pop_front ();
		}
	      else
		{
		  // if asked blk is not present in l2 cache then
		  l2_miss[k]++;
		  ll evicted = add_block_l2 (machineadd_, k);
		  if (evicted && hit_miss_l1 (evicted, tid_))
		    {
		      invalidate_block_l1 (evicted, tid_);
		    }
		  add_block_l1 (machineadd_, tid_);
		}
	    }
	  k++;
	}   
    }
    	
    	cout << "--------------------------------------------------\n";
	
	cout<<"Number of simulated cycles "<<sim_cycle<<endl;
   	
   	cout << "--------------------------------------------------\n";
	
	cout<<"Number of L1 cache accesses and misses"<<endl;
	cout<<"Core   accesses    misses"<<endl;
	
    for(int i=0;i<8;i++){
     cout<<i<<"      "<<l1_access[i]<<"      "<<l1_miss[i]<<endl;
     }
     cout << "--------------------------------------------------\n";
     
     ll total_l2_miss =0;
     for(int i=0;i<8;i++){
     	total_l2_miss += l2_miss[i];
     }
     
     
	cout<<"Number of L2 cache misses : "<<total_l2_miss<<endl;

	cout << "--------------------------------------------------\n";

    cout<<"Names and counts of all messages received by the L1 caches."<<endl;
    cout<<"GET       "<<l1_msg_count["GET"]<<endl;
    cout<<"PUT       "<<l1_msg_count["PUT"]<<endl;
    cout<<"INV       "<<l1_msg_count["INV"]<<endl;
    cout<<"GETX      "<<l1_msg_count["GETX"]<<endl;
    cout<<"PUTX      "<<l1_msg_count["PUTX"]<<endl;
    cout<<"INVACK    "<<l1_msg_count["INVACK"]<<endl;
    
    cout << "--------------------------------------------------\n";
    
    cout<<"Names and counts of all messages received by the L2 caches."<<endl;
    cout<<"GET       "<<l2_msg_count["GET"]<<endl;
    cout<<"GETX      "<<l2_msg_count["GETX"]<<endl;
    cout<<"UPGRADE   "<<l2_msg_count["UPGRADE"]<<endl;
    cout<<"SWB       "<<l2_msg_count["SWB"]<<endl;
    
    cout << "--------------------------------------------------\n";
    
  return 0;
}
