#ifndef _LOCK_H_
#define _LOCK_H_

#ifndef	NLOCKS
#define	NLOCKS 50		/* number of locks */
#endif

#define LFREE 111 	/* this lock is free */
#define LUSED 222 	/* this lock is used */

#ifndef DELETED
#define DELETED	100
#endif

#define READ 200
#define WRITE 300

struct lockentry {   	/* lock table entry	*/
	int lstate;		/* the state LFREE or LUSED	*/
	int	lqhead;			/* q index of head of list */
	int	lqtail;			/* q index of tail of list */
	int ltype;			/* the type DELETED, READ or WRITE */
	int ldesc;			/* lock descriptor */
	int lprio;			/* lock priority */	
	int lProc[NPROC];	/* process that acquires the lock */
};
extern	struct lockentry locktab[];
extern	int	nextLock;
extern int nextldesc;
extern unsigned long ctr1000;

#define	isbadlock(l) (l<0 || l>=NLOCKS)

extern void linit();		/* Intialize locks */
extern int lcreate (void);	/* create locks */
extern int ldelete (int lockdescriptor);	/* delete locks */	
extern int lock (int ldesc, int type, int priority);	/* lock a lock */
extern int releaseall (int numlocks, int args, ...);	/* release locks */
extern int releaseLock(int pid, int ldesc, int flag);	/* release a particular lock */
extern int getlockid (int ldesc);	/* to get the lock id corresponding to lock descriptor */
extern void priorityInheritance(int pid); /* the priority inheritance code */
extern void priorityRampUp(int ldesc, int currpprio); /* code to ramp up priority */
extern int checkOtherHighPrioInQueue(int ldesc, int priority); /* code to check for higher priority writers */


#endif

