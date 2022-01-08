/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;

	int i;
	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);


	//Release all locks held by this process
	for(i = 0; i < NLOCKS; i++){
		if(proctab[pid].locks_held[i] == 1){
			releaseall(1, i);
		}
	}


	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;
			if(proctab[pid].plock != -1){
				dequeue(pid); // See if this dequeues from the lock wait queue and does not work wrongly for a normal waiting process

				update_lprio(proctab[pid].plock);
//				recalculate_pinh(proctab[pid].plock);
				maxify_pinh_for_all_holding_procs(proctab[pid].plock);
				//recalculate_pinh(proctab[pid].plock);

				proctab[pid].wait_lock_type = -1;
				proctab[pid].lock_wait_time = -1;
				proctab[pid].plock = -1;
				proctab[pid].wait_prio = -1;
				proctab[pid].pinh = 0;
                		//update_lprio(proctab[pid].plock);
                		//recalculate_pinh(proctab[pid].plock);
        		}
	case PRREADY:	dequeue(pid);
			pptr->pinh = 0;
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}



	restore(ps);
	return(OK);
}



int update_lprio(int lck_id){
	int i;
	int max_prio = -1;
        int head = rwlocks[lck_id].lqhead;
        int tail = rwlocks[lck_id].lqtail;
        int next;
	int curr_proc_prio;
        next = q[head].qnext;
        while(next != tail && next < NPROC){
		curr_proc_prio = getprio(next);

		if(max_prio < curr_proc_prio){
			max_prio = curr_proc_prio;
		}
 
	       	next = q[next].qnext;
        }

//	if(max_prio == -1){
		// All processes in the queue have been deleted. So we reset lprio to 0.
//		rwlocks[lck_id].lprio = 0;		
//	}
//	else{
		rwlocks[lck_id].lprio = max_prio;
//	}
	return max_prio;
}
