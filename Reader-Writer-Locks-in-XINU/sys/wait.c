/* wait.c - wait */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sem.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * wait  --  make current process wait on a semaphore
 *------------------------------------------------------------------------
 */
SYSCALL	wait(int sem)
{
	STATWORD ps;    
	struct	sentry	*sptr;
	struct	pentry	*pptr;

	disable(ps);
	if (isbadsem(sem) || (sptr= &semaph[sem])->sstate==SFREE) {
		restore(ps);
		return(SYSERR);
	}
	
	if (--(sptr->semcnt) < 0) {
		(pptr = &proctab[currpid])->pstate = PRWAIT;
		pptr->psem = sem;
		//Note: This enqueue should be based on priority
		enqueue(currpid,sptr->sqtail);
		pptr->pwaitret = OK;
		resched();
		restore(ps);

		//Note: Should return deleted if returning from a deleted lock. Probably
		//set it in sdelete?
		return pptr->pwaitret;
	}
	restore(ps);
	return(OK);
}
