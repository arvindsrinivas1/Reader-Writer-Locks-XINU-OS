#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
//#include <sem.h>
#include <stdio.h>
#include <lock.h>


LOCAL int newlock();


SYSCALL lcreate(){
	STATWORD ps;
	int lck;
	disable(ps);

        if ((lck=newlock())==SYSERR ) {
                restore(ps);
                return(SYSERR);
        }

	restore(ps);
	return(lck);
}



LOCAL int newlock(){
	int lck,i;
	for(i=0; i<NLOCKS; i++){
        	lck=nextlock--;
                if (nextlock < 0)
                        nextlock = NLOCKS-1;
                if (rwlocks[lck].lstate==LFREE) {
                        rwlocks[lck].lstate = LUSED;
                        return(lck);
                }
	}
	return(SYSERR);
}
