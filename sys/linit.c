#include <kernel.h>
#include <proc.h>
#include "lock.h"
#include <q.h>

struct lockentry locktab[NLOCKS];

void linit() {
	struct lockentry *lptr;
	int pid;
	int ldesc;
	nextLock = NLOCKS - 1;
	nextldesc = 1;
	for (ldesc = 0; ldesc < NLOCKS; ldesc ++) {
		lptr = &locktab[ldesc];
		lptr->lstate = LFREE;
		lptr->ldesc = -1;
		lptr->ltype = DELETED;
		lptr->lprio = -1;
		lptr->lqtail = 1 + (lptr->lqhead = newqueue());
		for (pid = 0; pid < NPROC; pid++) {
			lptr->lProc[pid] = 0;
		}
	}
}

