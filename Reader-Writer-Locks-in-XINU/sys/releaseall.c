/* int releaseall() */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

int releaseall (int numlocks, int ldes1, ...){
	//Sanity check for locks
	STATWORD ps;
	int *ldes;
	int i,ld;
	unsigned long *a;
	unsigned long   *saddr;
	int return_type = OK;
	struct  lentry  *lptr;
	disable(ps);

	//a = (unsigned long*)(&args) + (numlocks - 1); // last argument
	return_type = OK;
	ldes = &ldes1;//point to the first argument	
	for(i = 0; i < numlocks; i++){
		ld = *(ldes+i);
		lptr = &rwlocks[ld];

		// Last condition is to handle the case if the proc tries to release a lock that it is not holding
		if(isbadlck(ld) || lptr->lstate == LFREE || proctab[currpid].locks_held[ld] == 0){
			return_type = SYSERR;
			continue;
		}
		else{
			// Valid lock to release

				// Decrease rcount if it is a read lock
				if(lptr->ltype == READ){
					lptr->readers_count -= 1;
				}

				// Remove lock-process mappings
				lptr->lbitmask[currpid] = 0;
				proctab[currpid].locks_held[ld] = 0;


				//Case to pass the lock to the next candidat. Else it means we just released a read lock which still has some readers holding it.e
				if((lptr->ltype == READ && lptr->readers_count == 0) || (lptr->ltype == WRITE)){					
					//Chooses next candidate(s), processes them and places them in the ready queue and sets the values for them appropriately
					choose_next_candidate(ld);
					reset_priority();		
				}
		}
	}
	return return_type;
	restore(ps);
}


int choose_next_candidate(int lck_id){
	//SHould remove the next candidate from the lock wait q and put it in the rdy q too
	struct  lentry  *lptr = &rwlocks[lck_id];
	int candidate_changed = 0;
	int candidate = q[lptr->lqtail].qprev;
	
	int writer_found = 0;

	int prev_temp;

	int max_wait_priority = proctab[candidate].wait_prio;
	int ltype_of_candidate = proctab[candidate].wait_lock_type;
	int prev = candidate;

	lptr->ltype = ltype_of_candidate;

	if(ltype_of_candidate == READ){
		//Iterate back till either the end of the wait q, or end of same priority or till same priorirty and write
		while(prev != lptr->lqhead && prev < NPROC && proctab[prev].wait_lock_type == READ){
			prev = q[prev].qprev;
		}
		
		if(prev != lptr->lqhead){
			//It will be a waiting writer with same priority/lower priority or a reader with lower priority

			if(prev >= NPROC){
				//IMPOSSIBLE CONDITION!!!!!!!
			}

			//If it is a writer with same priority as our candidate reader and time diff is < 1000, then that writer is considered
			if(proctab[prev].wait_lock_type == WRITE){
				//Should Ideally come only into this.
				if(proctab[prev].wait_prio == max_wait_priority){
					//Writer has same priority as the reader(s)
					writer_found = 1;
					if(proctab[prev].lock_wait_time - proctab[candidate].lock_wait_time < 1000){
						candidate = prev;
						max_wait_priority = proctab[candidate].wait_prio;						
						ltype_of_candidate = proctab[candidate].wait_lock_type;
						lptr->ltype = ltype_of_candidate;
						candidate_changed = 1;

						lptr->lbitmask[candidate] = 1;

						proctab[candidate].locks_held[lck_id] = 1;
						proctab[candidate].wait_lock_type = -1;
						proctab[candidate].wait_prio = -1;
						proctab[candidate].plock = -1;				

						//dequeue from wait and insert into ready
						dequeue(candidate);
						//ready(candidate, RESCHYES);


                                                if (isbadpid(candidate))
                                                        return(SYSERR);
                                                proctab[candidate].pstate = PRREADY;
                                                insert(candidate,rdyhead,max(proctab[candidate].pprio, proctab[candidate].pinh));


						//This should not be needed ideall. Check while testing
						//recalculate_lprio(lck_id, proctab[candidate].pprio);
						update_lprio(lck_id);
						maxify_pinh_for_all_holding_procs(lck_id);
						reset_priority();
						resched();	
				}
					//This should only update the pinh of the candidate(s) ideally and not recurse.Check while testing
					//recalculate_pinh(lck_id);
					//maxify_pinh_for_all_holding_procs(lck_id);
					
				}

			}
		}

		if(candidate_changed == 0){
			//Choose all readers with the same priority of the candidate
			prev = candidate;
                	//while(prev != lptr->lqhead && prev < NPROC && proctab[prev].wait_lock_type == READ){

			//If there was a writer with same prio as max prio(which is read) but it wasnt chosen
			if(writer_found == 1){
				while(prev != lptr->lqhead && prev < NPROC && proctab[prev].wait_prio == max_wait_priority){
					if(proctab[prev].wait_lock_type ==  READ){
						// Set lock hold values
                        			++lptr->readers_count;
                        			lptr->lbitmask[prev] = 1;
                        			proctab[prev].locks_held[lck_id] = 1;

						//unset lock wait values
                        			proctab[prev].wait_lock_type = -1; // No longer waiting
                        			proctab[prev].wait_prio = -1; // No longer waiting
                        			proctab[prev].plock = -1; // No longer waiting

						//Store the next prev before you dequeue and insert this one
						prev_temp = q[prev].qprev;

						//Remove from wait lock q and put it into ready q
                        			//ready(getlast(lptr->lqtail), RESCHYES);
						dequeue(prev);
				        	if (isbadpid(prev))
                					return(SYSERR);
        					proctab[prev].pstate = PRREADY;
        					insert(prev,rdyhead,max(proctab[candidate].pprio, proctab[candidate].pinh) );


						//Recalc lprio for lck_id and pinh for procs holding it(recursive)
                        			//recalculate_lprio(lck_id, proctab[prev].pprio);
						update_lprio(lck_id);
						maxify_pinh_for_all_holding_procs(lck_id);
						reset_priority();
				//		maxify_pinh_for_all_holding_procs(lck_id);


						//resched();

	                        		prev = prev_temp;
					}
					else{
						//SKIPPING THE WRITER
						prev = q[prev].qprev;
					}
                		}
				resched();
				//recalculate_pinh(lck_id);
				//maxify_pinh_for_all_holding_procs(lck_id);
			}
			else if(writer_found == 0){
				while(prev != lptr->lqhead && prev < NPROC && proctab[prev].wait_lock_type == READ){
					++lptr->readers_count;
					lptr->lbitmask[prev] = 1;

					proctab[prev].locks_held[lck_id] = 1;
					proctab[prev].wait_lock_type = -1;
					proctab[prev].wait_prio = -1;
					proctab[prev].plock = -1;

					//Store the next prev before dequeue and insert
					prev_temp = q[prev].qprev;

					//ready(getlast(lptr->lqtail), RESCHYES);
                                        dequeue(prev);
                                        if (isbadpid(prev))
                                        	return(SYSERR);
                                        proctab[prev].pstate = PRREADY;
                                        insert(prev,rdyhead,max(proctab[candidate].pprio, proctab[candidate].pinh));

					update_lprio(lck_id);
					maxify_pinh_for_all_holding_procs(lck_id);
					reset_priority();

					//resched();
					prev = prev_temp;
				}
//				maxify_pinh_for_all_holding_procs(lck_id);
				resched();
			}

		} // End of if candidate was not changed(If it was handled, it is handled before itself)
	}//End of first candidate = READ

	else if(ltype_of_candidate == WRITE){
		lptr->lbitmask[prev] = 1; //Will be the candidate

		proctab[prev].locks_held[lck_id] = 1;
		proctab[prev].wait_lock_type = -1;
		proctab[prev].wait_prio = -1;
		proctab[prev].plock = -1;

		//proctab[prev].pstate = PRREADY;

//		ready(getlast(lptr->lqtail), RESCHYES);

                dequeue(prev);
                if (isbadpid(prev))
                	return(SYSERR);
                proctab[prev].pstate = PRREADY;
                insert(prev,rdyhead,max(proctab[candidate].pprio, proctab[candidate].pinh));


		update_lprio(lck_id);
		maxify_pinh_for_all_holding_procs(lck_id);
		reset_priority();
		resched();
		//recalculate_lprio(lck_id, proctab[prev].pprio);
		//recalculate_pinh(lck_id);	
	}

	return candidate;
}



int reset_priority(){
	int i,max_priority = -1;
	for(i = 0; i < NLOCKS; i++){
		if(proctab[currpid].locks_held[i] == 1){
			if(rwlocks[i].lprio > max_priority){
				max_priority = rwlocks[i].lprio;
			}
		}
	}

	if(max_priority != -1){

		if(max_priority <= proctab[currpid].pprio){
			proctab[currpid].pinh = 0;
		}
		else{
			 proctab[currpid].pinh = max_priority;
		}

		//proctab[currpid].pprio = max_priority;
	}
	else{
		proctab[currpid].pinh = 0;
	}
	if(proctab[currpid].plock != -1){
		update_lprio(proctab[currpid].plock);
		maxify_pinh_for_all_holding_procs(proctab[currpid].plock);
	}

}
