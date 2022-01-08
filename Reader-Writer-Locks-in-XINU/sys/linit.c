/* Implement linit.c */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
//#include <sem.h>
#include <stdio.h>
#include <lock.h>


int linit(){
	int i, p;
	for(i = 0; i < NLOCKS; i++){
		rwlocks[i].lstate = LFREE;
		rwlocks[i].lcount = 1;
		//rwlocks[i].plock = -1;
		//rwlocks[i].lqhead = ;
		//rwlocks[i].lqtail = ;
		rwlocks[i].ltype = READ;//Doesnt matter if lstate is LFREE
		rwlocks[i].lprio = -1;
		
		for(p = 0; p < NPROC; p++){
			rwlocks[i].lbitmask[p] = 0;
		}
		rwlocks[i].readers_count = 0;
	}
}
