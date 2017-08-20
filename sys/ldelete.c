/* ldelete.c - ldelete */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include "lock.h"
#include <stdio.h>

/*------------------------------------------------------------------------
 *  *  * ldelete  --  delete a lock by releasing its table entry
 *   *   *------------------------------------------------------------------------
 *    *    */
SYSCALL ldelete(int ldesc)
{
	STATWORD ps;    
	int	pid;
	int lock;
	struct	lockentry *lptr;

	disable(ps);
	lock = getlockid(ldesc);
	lptr = &locktab[lock];
	if (isbadlock(lock)) {
		restore(ps);
		return(SYSERR);
	}
	lptr->lstate = LFREE;
	lptr->ltype = DELETED;
	lptr->lprio = -1;
	lptr->lProc[currpid] = -1;

	if (nonempty(lptr->lqhead)) {
		pid = getlast(lptr->lqhead);
		while(pid != EMPTY) {
		    proctab[pid].ltype[ldesc] = DELETED;
		    proctab[pid].waitpprio = -1;
		    proctab[pid].pwaitret = DELETED;
		    proctab[pid].lockid = -1;
		    /*dequeue(pid);*/
		    ready(pid, RESCHNO);
		}
		resched();
	}
	restore(ps);
	return(OK);
}

