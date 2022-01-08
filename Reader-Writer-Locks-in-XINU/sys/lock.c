#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>


int lock (int ldes1, int type, int priority){
	STATWORD ps;
	struct lentry *lptr;
	struct pentry *pptr;
	int highest_wait_prio;
	int holding_proc_prio;
	disable(ps);
	//return if ldes1 is out of range or if its is still LFREE
	//When lock is deleted, set it to LFREE
        if (isbadlck(ldes1) || (lptr= &rwlocks[ldes1])->lstate==LFREE) {
                restore(ps);
                return(SYSERR);
        }


	//When it comes here, it is not LFREE. And if it is there in the history, we should not allow it.
	if(proctab[currpid].deleted_lock_history[ldes1] == 1){
		restore(ps);
		return(SYSERR);
	}
	

	//proctab[currpid].lock_history[ldes1] = 1;

	if(isFree(ldes1)){
		//created but not acquired yet
		rwlocks[ldes1].ltype = type;
		rwlocks[ldes1].lbitmask[currpid] = 1;
		if(type == READ){
			++rwlocks[ldes1].readers_count;
		}
		proctab[currpid].locks_held[ldes1] = 1;
		restore(ps);
		return OK;
	}

	if(type == READ){
		if(rwlocks[ldes1].ltype == READ){
			highest_wait_prio = get_highest_wait_writer_priority(ldes1);
			if(priority >= highest_wait_prio){
				//We can let the reader to acquire the read lock
				proctab[currpid].locks_held[ldes1] = 1;
				rwlocks[ldes1].readers_count += 1;
				rwlocks[ldes1].lbitmask[currpid] = 1;
			}
			else{
				//Have towait
				
				//insert into the wait priortiy q of the lock

				insert(currpid, rwlocks[ldes1].lqhead, priority);

				proctab[currpid].lock_wait_time = ctr1000;
				proctab[currpid].plock = ldes1;
				proctab[currpid].wait_lock_type = READ;
				proctab[currpid].wait_prio = priority;
				proctab[currpid].pstate = PRWAIT;
				//recalculate_lprio(ldes1, proctab[currpid]priority);
				//pinh will obv be 0 as we are inserting for the first time. So just use pprio
				//recalculate_lprio(ldes1, getprio(currpid));

				update_lprio(ldes1);
				maxify_pinh_for_all_holding_procs(ldes1);

				//We have new waiting process now. So recalculate pinh for the holding process

				//Recalculate pinh
				//recalculate_pinh(ldes1);
				proctab[currpid].lwaitret = OK;
				resched();
				restore(ps);

				return proctab[currpid].lwaitret;
				//check if its already holding a lock? Never happening case. But to test.
				
			}
		}
		else if(rwlocks[ldes1].ltype == WRITE){
			//Have towait
	
			insert(currpid, rwlocks[ldes1].lqhead, priority);

			proctab[currpid].lock_wait_time = ctr1000;
                        proctab[currpid].plock = ldes1;
                        proctab[currpid].wait_lock_type = READ;
                        proctab[currpid].wait_prio = priority;
			proctab[currpid].pstate = PRWAIT;

			proctab[currpid].lwaitret = OK;

                        //recalculate_lprio(ldes1, getprio(currpid));
			//recalculate_pinh(ldes1);

                        update_lprio(ldes1);
                        maxify_pinh_for_all_holding_procs(ldes1);


			resched();
			restore(ps);
			return proctab[currpid].lwaitret;
		}

		else{
			//"CAN NEVER COME HERE !!!!!!!1\n";

		}
	}


	else if(type == WRITE){
		insert(currpid, rwlocks[ldes1].lqhead, priority);

		proctab[currpid].lock_wait_time = ctr1000;
		proctab[currpid].plock = ldes1;
		proctab[currpid].wait_lock_type = WRITE;
		proctab[currpid].wait_prio = priority;
		proctab[currpid].pstate = PRWAIT;


		proctab[currpid].lwaitret = OK;

		//recalculate_lprio(ldes1, getprio(currpid));
		//recalculate_pinh(ldes1);

                update_lprio(ldes1);
                maxify_pinh_for_all_holding_procs(ldes1);


		resched();

		// Control comes back after lock is acquired
		restore(ps);
		return proctab[currpid].lwaitret;	
	}
	restore(ps);
	return OK;
}


int isFree(int lck_id){
	int i, free = 1;
        for(i = 0; i<NPROC;i++){

                if(rwlocks[lck_id].lbitmask[i] == 1){
                        free = 0;
                        break;
                }
        }

        return free;
}


int get_highest_wait_writer_priority(int lck_id){
        int highest_priority = -1, i;

	int head = rwlocks[lck_id].lqhead;
	int tail = rwlocks[lck_id].lqtail;
	int next;

	next = q[head].qnext;
	while(next != tail && next < NPROC){
		if(proctab[next].wait_lock_type == WRITE){
			if(highest_priority < proctab[next].wait_prio){
				highest_priority = proctab[next].wait_prio;
			}
		}
		next = q[next].qnext;
	}

	//Will return -1 if no writer is there in the lock wait queue
	return highest_priority;
}

/*
int recalculate_values(int lck_id, int pprio){
	recalculate_lprio(lck_id, pprio);
	recalculate_pinh(lck_id);
}
*/

/*
int recalculate_lprio(int lck_id, int pprio){
	// Since we are sending getprio() it will be pinh or pprio accordingly

	if(rwlocks[lck_id].lprio < pprio){
		rwlocks[lck_id].lprio = pprio;
	}
}

int recalculate_pinh(int lck_id){
	int i, p, proc_prio;
	struct  pentry  *pptr;
	int changed = 0;
	for(i = 0; i < NPROC; i++){
		changed = 0;
		if(rwlocks[lck_id].lbitmask[i] == 1){
			pptr = &proctab[i];

	// pinh of a process will always be the maximum of lprios of all the locks it is holding currently. If none of those locks have a queue, then we can reset it to 0y
			if(pptr->pinh == 0){
				//proc_prio = pptr->pprio;
				if(pptr->pprio != rwlocks[lck_id].lprio){
					pptr->pinh = rwlocks[lck_id].lprio;	
					changed = 1;
				}
			}
			else{
				if(pptr->pinh != rwlocks[lck_id].lprio){
					pptr->pinh = rwlocks[lck_id].lprio;
					changed = 1;
				}
			}

			


			if (changed == 1){
				//transitive property
				if(pptr->plock >= 0){
					// It is waiting on some lock
					//Now it is basically like I have inserted a proc with priority = pinh of i. so recalculatelprio for this lock and then recalculate pinh of plock's holding processes.
					recalculate_lprio(pptr->plock, pptr->pinh);
					recalculate_pinh(pptr->plock);
				}
			}
		}
	}

	return OK;
}
*/
int maxify_pinh_for_all_holding_procs(int lck_id){
	int i;
	for(i = 0; i < NPROC; i++){
		if(rwlocks[lck_id].lbitmask[i] == 1){
			maxify_pinh(lck_id, i);
		}
	}
}


int maxify_pinh(int lck_id, int proc_id){
        int i;
        int max_lprio = -1;
	int changed = 0;

        for(i = 0; i < NLOCKS; i++){
                if(proctab[proc_id].locks_held[i] == 1){
                        if(rwlocks[i].lprio > max_lprio){
                                max_lprio = rwlocks[i].lprio;
                        }
                }
        }

	//if pinh wasnt from this lprio. (ie) max_lprio = pinh, then not changed.
	if(max_lprio == proctab[proc_id].pinh){
		//Do nothing
		return OK;
	}

	// If max_lprio is not the same as the existing pinh, thtn we have to change the pinh of the holding proc.
	// This must be followed on recursively.


	// if it is not the same( only possiblilty is pinh decrease)
	// ie if max_lprio < pinh
	if(max_lprio == -1){
		// all lcks held by this process has no waiting process. So reset.
		changed = 1;
		proctab[proc_id].pinh = 0;	
	}
	else{
		//pinh was decided by this lock's lprio and since it has changed now, pinh has to change.

		if(proctab[proc_id].pinh != 0){
			changed = 1;
			proctab[proc_id].pinh = max(proctab[proc_id].pprio, max_lprio);
		}
		else if(proctab[proc_id].pinh == 0){
			//changed = 1;
			if(max_lprio > proctab[proc_id].pprio){
				changed = 1;
				proctab[proc_id].pinh = max_lprio;
			}
		}
	//	if(max_lprio < proctab[proc_id].pprio){
	//		//or should i make it 0?
	//		changed = 1;
	//		proctab[proc_id].pinh = proctab[proc_id].pprio;
	//	}
	//	else{
	//		// This max_lprio could have been from some other lock held by the process too.
	//		changed = 1;
        //		proctab[proc_id].pinh = max_lprio;
	//	}
	}


	// If the pinh of this proc has changed and this proc is waiting for a lock, then change the pinh of the holding locks too(after checking conditions)
	if(changed == 1 && proctab[proc_id].plock != -1){
		//transitive case
		update_lprio(proctab[proc_id].plock);	
		maxify_pinh_for_all_holding_procs(proctab[proc_id].plock);
	}

	return OK;
}
