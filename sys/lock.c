#include <conf.h>
#include <q.h>
#include <kernel.h>
#include <stdio.h>
#include <proc.h>
#include "lock.h"

int lock (int ldesc, int type, int priority) {
	STATWORD ps;
	struct lockentry *lptr;
	struct pentry *pptr;
	pptr = &proctab[currpid];
	lptr = &locktab[ldesc];
	int writePermLock = 0;
	int lockid = getlockid(ldesc);
	disable (ps);
	/* kprintf("LDESC = %d, type=%d, priority = %d\n", ldesc, type, priority); 
 * 	kprintf("LOCK STATE = %d\n", lptr->lstate); */
	if (isbadlock(lockid) || type == DELETED) {
		restore(ps);
		return SYSERR;
	}

	if (lptr->lstate == LFREE) { /* if the lock state is FREE, then any process can acquire it */
		pptr->waitpprio = priority;
		pptr->ltype[lockid] = type;
		pptr->lockid = -1;
		lptr->ltype = type;
		lptr->lstate = LUSED;
		lptr->lProc[currpid] = 1;
		lptr->lprio = pptr->pprio;
		restore(ps);
		return (OK);
	} else if (lptr->lstate != LFREE) { /* if the lock state is not FREE, we have a series of check before giving the lock to the process */
		if (lptr->ltype == WRITE) { /* if a writer process is requesting, it is directly put to wait because writers need exclusive lock */
			pptr->ltype[lockid] = type;
			pptr->pstate = PRWAIT;
			pptr->lockid = ldesc;
			pptr->waitpprio = priority;
			pptr->waitTime = ctr1000;
			priorityRampUp(lockid, pptr->pprio); /* priority ramp up if the process holding lock has less priority than the process requesting it */
			insert(currpid, lptr->lqhead, priority); /* put the process in wait queue */
			resched();
			restore(ps);
			return (OK);
		} else if (lptr->ltype == READ) { /* if a reader process is requesting, we have a series of check before giving the lock to the process */
			if (type == READ) { /* if the existing lock type is READ */
				/*if 1 is returned in writePermLock, there is a higher priority writer already waiting for the lock, so preference has to be given to it */
				writePermLock = checkOtherHighPrioInQueue(ldesc, priority); 
				if (writePermLock == 1) { /* if there is a high priority writer process, it is put in wait */
				/*	kprintf("writepermlock 1"); */
					pptr->ltype[lockid] = type;
					pptr->pstate = PRWAIT;
					pptr->lockid = ldesc;
					pptr->waitpprio = priority;
					pptr->waitTime = ctr1000;
					priorityRampUp(lockid, pptr->pprio);/* priority ramp up if the process holding lock has less priority than the process requesting it */
					insert(currpid, lptr->lqhead, priority); /* put the process in wait queue */
					resched();
					restore(ps);
					return (OK);
				} else if (writePermLock != 1) { /* if there is no high priority writer process, the lock is given to the reader process */
				/*	kprintf("writeperm lock != 1"); */
					lptr->ltype = type;
					pptr->ltype[lockid] = type;
					lptr->lProc[currpid] = 1;
					pptr->waitpprio = priority;
					if (lptr->lprio < pptr->pprio) {
						lptr->lprio = pptr->pprio;
					}
					restore(ps);
					return(OK);
				}
			} else if (type == WRITE) { /* if the existing lock type is WRITE, the requesting process directly put into wait */
				pptr->ltype[lockid] = type;
				pptr->pstate = PRWAIT;
				pptr->lockid = ldesc;
				pptr->waitpprio = priority;
				pptr->waitTime = ctr1000;
				priorityRampUp(ldesc, pptr->pprio);/* priority ramp up if the process holding lock has less priority than the process requesting it */
				insert(currpid, lptr->lqhead, priority); /* put the process in wait queue */
				resched();
				restore(ps);
				return (OK);
			}
		}
	}
	restore(ps);
	return (OK);
}

void priorityInheritance(int pid) {
	int lockid, temp;
	int maxPriority = 0;
	lockid = getlockid(proctab[pid].lockid);
	if(lockid != SYSERR) {
		temp = locktab[lockid].lqtail;
		while(q[temp].qprev != locktab[lockid].lqhead) {
			if( maxPriority < proctab[q[temp].qprev].pprio ) {
				maxPriority = proctab[q[temp].qprev].pprio;
			}
			temp = q[temp].qprev;
		}
		priorityRampUp(lockid, maxPriority);
	}	
}

int checkOtherHighPrioInQueue(int ldesc, int priority) {
	struct lockentry *lptr;
	lptr = &locktab[ldesc];
	int dummy = lptr->lqhead;
	int writePermLock = 0;
	while (q[dummy].qnext != lptr->lqtail) {
		if ((proctab[q[dummy].qnext].ltype[ldesc] == WRITE) && (priority < proctab[q[dummy].qnext].waitpprio)) {
			writePermLock = 1;
		}
		dummy = q[dummy].qnext;
	}
	return writePermLock;
}

void priorityRampUp(int ldesc, int currpprio) {
	int proc;
	for (proc = 0; proc < NPROC; proc ++) {
		if ((proctab[proc].ltype[ldesc] != DELETED) && (locktab[ldesc].lProc[proc] == 1)) {
			if (proctab[proc].pinh < currpprio) { /* if inherited priority is less than the max priority */
				/*	kprintf("HEREEEE"); */
				proctab[proc].pprio = currpprio;
				priorityInheritance(proc);
			} else {
				/*	kprintf("OUTOFHERE");
 *						kprintf("CURRPPRIO:%d",proctab[proc].pinh);
 *											kprintf("PROCPRIO:%d",proctab[proc].pprio); */
				proctab[proc].pprio = proctab[proc].pinh;
			}
		}
	}
}

int getlockid(int ldesc) {
	int lockid;
	if (ldesc > 0) {
		for (ldesc = 0; ldesc < NLOCKS; ldesc ++) {
			if (locktab[ldesc].ldesc == ldesc) {
				lockid = ldesc;
			}
		}
		return lockid;
	}
	return SYSERR;
}

