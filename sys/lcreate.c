/* lcreate.c - lcreate, newlock */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include "lock.h"

LOCAL int newlock();

/*---------------------------------------------------------------------------
 *  *  * lcreate  --  create and initialize a lock, returning its lock description
 *   *   *---------------------------------------------------------------------------
 *    *    */
int lcreate(void)
{
	STATWORD ps;    
	int	lock;

	disable(ps);
	if ((lock=newlock())==SYSERR) {
		restore(ps);
		return(SYSERR);
	}
	
	restore(ps);
	return(locktab[lock].ldesc);
}

LOCAL int newlock()
{
	int	lock;
	int	i;

	for (i=0 ; i<NLOCKS ; i++) {
		lock = nextLock--;
		if (nextLock < 0)
			nextLock = NLOCKS - 1;
		if (locktab[lock].lstate == LFREE) {
			locktab[lock].ldesc = nextldesc ++;
			locktab[lock].lstate = LUSED;
			return(lock);
		}
	}
	return(SYSERR);
}


