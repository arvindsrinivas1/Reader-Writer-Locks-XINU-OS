#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sem.h>
#include <stdio.h>
#include <lock.h>

int ldelete (int lck_id){
        STATWORD ps;
        int     pid;

	int i;
	struct  lentry  *lptr;
	disable(ps);

        if (isbadlck(lck_id) || rwlocks[lck_id].lstate==LFREE) {
                restore(ps);
                return(SYSERR);
        }

	// Reset values
	lptr = &rwlocks[lck_id];
	lptr->lstate = LFREE;
	lptr->lcount = 1;
	lptr->ltype = READ; // DOESNT MATTER as we set state to LFREE
	lptr->lprio = -1;
	lptr->readers_count = 0;


	for(i = 0; i < NPROC; i++){
		if(rwlocks[lck_id].lbitmask[i] == 1){
			//If a process is holding the lock
			//release it
			proctab[i].locks_held[lck_id] = 0;

			// We set the value to one only during ldelete(if the process holds the lock)
			proctab[i].deleted_lock_history[lck_id] = 1;
		}
		rwlocks[lck_id].lbitmask[i] = 0;
	}

	//Ready processes waiting for the lock after setting lwaitret = DELETED
        if (nonempty(lptr->lqhead)) {
                while( (pid=getfirst(lptr->lqhead)) != EMPTY)
                  {
		    proctab[pid].plock = -1;
		    proctab[pid].wait_lock_type = -1;
	            proctab[pid].wait_prio = -1;
                    proctab[pid].lwaitret = DELETED;
		     proctab[pid].deleted_lock_history[lck_id] = 1;
                    ready(pid,RESCHNO);
                  }
                resched();
        }
        restore(ps);
        return(OK);
}
