/* chprio.c - chprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>
/*------------------------------------------------------------------------
 * chprio  --  change the scheduling priority of a process
 *------------------------------------------------------------------------
 */
SYSCALL chprio(int pid, int newprio)
{
	STATWORD ps;    
	struct	pentry	*pptr;

	disable(ps);
	if (isbadpid(pid) || newprio<=0 ||
	    (pptr = &proctab[pid])->pstate == PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	pptr->pprio = newprio;

	if(pptr->pinh != 0){
		if(pptr->pinh < pptr->pprio){
			pptr->pinh = pptr->pprio;
		}
	}

	//Handle locks
	if(pptr->plock != -1){
		//recalculate_lprio(pptr->plock, pptr->pprio);
		update_lprio(pptr->plock);
		//recalculate_pinh(pptr->plock);
		maxify_pinh_for_all_holding_procs(pptr->plock);
	}

	restore(ps);
	return(newprio);


}
