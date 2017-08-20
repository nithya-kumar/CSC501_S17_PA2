/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include "lock.h"

/*------------------------------------------------------------------------
 *  *  * kill  --  kill a process and remove it from the system
 *   *   *------------------------------------------------------------------------
 *    *    */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;
	struct lockentry *lptr;
	int ldesc, lockid;
	int reschedStat = RESCHNO;

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

	freestk(pptr->pbase, pptr->pstklen);

	for (ldesc = 0; ldesc < NLOCKS; ldesc++) {
		lptr = &locktab[ldesc];
		if (lptr->lProc[pid] == 1) { // if process is already acquiring some lock, release the lock and reschedule the process
			releaseLock(pid, ldesc, 0);
			reschedStat = RESCHYES;
		}
	}
	switch (pptr->pstate) {

	case PRCURR:
			pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:
			semaph[pptr->psem].semcnt++;
			lockid = getlockid(pptr->lockid);
			if (!isbadlock(lockid) || locktab[lockid].lstate != LFREE) {
				proctab[pid].pprio = -1;
				proctab[pid].pinh = -1;
				releaseLock(pid, lockid, 1);
			}

	case PRREADY:
			dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	
			pptr->pstate = PRFREE;
	}
	if (reschedStat == RESCHYES) {
		resched();
	}
	restore(ps);
	return(OK);
}


