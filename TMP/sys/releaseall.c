#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <sleep.h>
#include "lock.h"

int releaseall(int numlocks, int args, ...) {
	STATWORD ps;
	int retVal = OK;
	int i, ldesc;
	int lockid;
	disable(ps);
	for (i = 0; i < numlocks; i ++) {
		ldesc = *(&args) + 1;
		lockid = getlockid(ldesc);
		if (!isbadlock(lockid) && locktab[lockid].lProc[currpid] == 1) { /* if the lock is not a bad lock and it is acquired by some process, release it */
			retVal = releaseLock(currpid, lockid, 0);
		} else {
			retVal = SYSERR;
		}
	}
	resched();
	restore(ps);
	return retVal;
}

int releaseLock(int pid, int lockid, int flag) {
	struct lockentry *lptr;
	struct pentry *pptr;
	int nextpid;
	int proc;
	int templock1, templock2;
	int readerFlag = 0;
	int writerFlag = 0;
	int writerProc = 0;
	int currpprio = 0;
	unsigned long timediff = 0;


	lptr = &locktab[lockid];
	pptr = &proctab[pid];

	lptr->lProc[pid] = -1;
	lptr->lstate = LFREE;

	if (flag == 0) { /* for non-waiting processes */
		pptr->ltype[lockid] = DELETED;
		pptr->waitpprio = -1;
		pptr->lockid = -1;
		currpprio = pptr->pprio;
		lptr->lProc[pid] = -1;
		lptr->ltype = DELETED;
		lptr->lstate = LFREE;
		proctab[pid].pprio = proctab[pid].pinh;
		priorityInheritance(pid);

		if (nonempty(lptr->lqhead)) {
			if (proctab[q[lptr->lqtail].qprev].ltype[lockid] == READ) { /* if lock is to be released from a reader process */
				/* check for other writer process in queue */
				templock1 = lptr->lqtail;
				while(q[templock1].qprev != lptr->lqhead) {
					if(proctab[q[templock1].qprev].ltype[lockid] == WRITE) {
						writerFlag = 1;
						writerProc = q[templock1].qprev;
						break;
					}
					templock1 = q[templock1].qprev;
				}

				if (writerFlag == 1) { /* if found a writer process in queue */
					if (proctab[q[lptr->lqtail].qprev].waitpprio == proctab[writerProc].waitpprio) {
						timediff = (proctab[writerProc].waitTime - proctab[q[lptr->lqtail].qprev].waitTime);
						if(timediff < 0) {
							timediff = timediff * (-1);
						}
						if(timediff < 1000) { /* check for the waiting time of the process and decide whether it is waiting to reade or writer */
							nextpid =  dequeue(writerProc);
							lptr->ltype = WRITE;
							lptr->lstate = LUSED;
							proctab[nextpid].lockid = -1;		
							lptr->lProc[nextpid] = 1;
							proctab[nextpid].waitTime = ctr1000 - proctab[nextpid].waitTime;
							ready(nextpid, RESCHNO);
						} else {
							templock2 = lptr->lqtail;
							while(q[lptr->lqtail].qprev != writerProc) {
								nextpid =  getlast(lptr->lqtail);
								lptr->ltype = READ;
								lptr->lstate = LUSED;
								proctab[nextpid].lockid = -1;		
								lptr->lProc[nextpid] = 1;
								proctab[nextpid].waitTime = ctr1000 - proctab[nextpid].waitTime;
								ready(nextpid, RESCHNO);
							}
						}
					} else {
						templock2 = lptr->lqtail;
						while(q[lptr->lqtail].qprev != writerProc) {
							nextpid =  getlast(lptr->lqtail);
							lptr->ltype = READ;
							lptr->lstate = LUSED;
							proctab[nextpid].lockid = -1;		
							lptr->lProc[nextpid] = 1;
							proctab[nextpid].waitTime = ctr1000 - proctab[nextpid].waitTime;
							ready(nextpid, RESCHNO);
						} 
					}
				}
				else if (writerFlag != 1) { /* if no writer process in queue, assign lock to reader process */
					templock1 = lptr->lqtail;
					while(q[templock1].qprev != lptr->lqhead ) {
						nextpid =  dequeue(q[templock1].qprev);
						lptr->ltype = READ;
						lptr->lstate = LUSED;
						proctab[nextpid].lockid = -1;		
						lptr->lProc[nextpid] = 1;
						proctab[nextpid].waitTime = ctr1000 - proctab[nextpid].waitTime;
						ready(nextpid, RESCHNO);
					}
				}
			} else if (proctab[q[lptr->lqtail].qprev].ltype[lockid] == WRITE) { /* if lock to be released from writer process */
				for (proc = 0; proc < NPROC; proc++) { /* we can allocate the lock to all other read process */
					if ((proctab[proc].ltype[lockid] == READ) && (locktab[lockid].lProc[proc] == 1) && (proctab[proc].waitpprio >= proctab[q[lptr->lqtail].qprev].waitpprio)) {
						readerFlag = 1;
						break;
					}
				}
				if (readerFlag == 0) {
					lptr->ltype = WRITE;
					lptr->lstate = LUSED;
					nextpid = getlast(lptr->lqtail);
					lptr->lProc[nextpid] = 1;
					proctab[nextpid].waitTime = ctr1000 - proctab[nextpid].waitTime;
					proctab[nextpid].lockid = -1;
					ready(nextpid, RESCHNO);
				}
			}		
		}
	} else if (flag == 1) { /* for already waiting process */
		proctab[pid].pprio = proctab[pid].pinh;
		priorityInheritance(pid);
		pptr->ltype[lockid] = DELETED;
		pptr->waitpprio = -1;
		pptr->lockid = -1;
		currpprio = pptr->pprio;
		lptr->lProc[pid] = -1;
		lptr->ltype = DELETED;
		lptr->lstate = LFREE;
		proctab[pid].pprio = proctab[pid].pinh;
		priorityInheritance(pid);
	}
}

