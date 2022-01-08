#define READ    	0
#define WRITE   	1
#define LDELETED	2

#define NLOCKS  	50
#define LFREE 		'\01'
#define LUSED 		'\02'

#define isbadlck(l)     (l<0 || l>=NLOCKS)


struct lentry{
	char lstate; 		// state of lentry(LFREE or LUSED) 
	int lcount;
	int lqhead;  		// index of element pointing to the head in the q structure
	int lqtail;  		// index  " "" " " " " "  " "" " ""  tail  """ " " " "" " 
	int ltype;   		// current type of this lock 
	int lprio;   		// Max wait priority for this lock. calculate from its queue.
	int lbitmask[NPROC];    // 1 if process is holding this lock else 0
	int readers_count;	// To count number of readers if ltype = READER. Set to -1 if WRITE
};


extern struct lentry rwlocks[];
extern int linit();
extern  int nextlock;
extern int update_lprio(int lck_id);

int isFree(int lck_id);
int get_highest_writer_priority(int lck_id);
