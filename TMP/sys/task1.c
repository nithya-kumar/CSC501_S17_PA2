#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>
#include <stdio.h>

#define DEFAULT_LOCK_PRIO 20

#define assert(x,error) if(!(x)){ \
            kprintf(error);\
            return;\
            }
int mystrncmp(char* des,char* target,int n){
    int i;
    for (i=0;i<n;i++){
        if (target[i] == '.') continue;
        if (des[i] != target[i]) return 1;
    }
    return 0;
}

void reader1 (char *msg, int sem)
{
        kprintf ("  %s: to acquire sem\n", msg);
        wait(sem);
        kprintf ("  %s: acquired sem\n", msg);
        kprintf ("  %s: to release sem\n", msg);
        signal(sem);
}

void writer1 (char *msg, int sem)
{
        kprintf ("  %s: to acquire sem\n", msg);
        wait(sem);
        kprintf ("  %s: acquired sem, sleep 10s\n", msg);
        sleep (10);
        kprintf ("  %s: to release sem\n", msg);
        signal(sem);
}

void testsemaphore ()
{
        int     sem;
        int     rd1, rd2;
        int     wr1;

        kprintf("\nSEMAPHORES:::\n");
        sem  = screate (1);

        rd1 = create(reader1, 2000, 25, "reader3", 2, "reader A Sem", sem);
        rd2 = create(reader1, 2000, 30, "reader3", 2, "reader B Sem", sem);
        wr1 = create(writer1, 2000, 20, "writer3", 2, "writer Sem", sem);

        kprintf("-start writer, then sleep 1s. sem granted to write (prio 20)\n");
        resume(wr1);
        sleep (1);

        kprintf("-start reader A, then sleep 1s. reader A(prio 25) blocked on the lock\n");
        resume(rd1);
        sleep (1);
        kprintf("priority of writer:%d\n", getprio(wr1));

        kprintf("-start reader B, then sleep 1s. reader B(prio 30) blocked on the lock\n");
        resume (rd2);
        sleep (1);
        kprintf("priority of writer:%d\n", getprio(wr1));
    
        kprintf("-kill reader B, then sleep 1s\n");
        kill (rd2);
        sleep (1);
        kprintf("priority of writer:%d\n", getprio(wr1));

        kprintf("-kill reader A, then sleep 1s\n");
        kill (rd1);
        sleep(1);
        kprintf("priority of writer:%d\n", getprio(wr1));

        sleep (8);
}


void reader3 (char *msg, int lck)
{
        int     ret;

        kprintf ("  %s: to acquire lock\n", msg);
        lock (lck, READ, DEFAULT_LOCK_PRIO);
        kprintf ("  %s: acquired lock\n", msg);
        kprintf ("  %s: to release lock\n", msg);
        releaseall (1, lck);
}

void writer3 (char *msg, int lck)
{
        kprintf ("  %s: to acquire lock\n", msg);
        lock (lck, WRITE, DEFAULT_LOCK_PRIO);
        kprintf ("  %s: acquired lock, sleep 10s\n", msg);
        sleep (10);
        kprintf ("  %s: to release lock\n", msg);
        releaseall (1, lck);
}

void testlocks ()
{
        int     lck;
        int     rd1, rd2;
        int     wr1;

        kprintf("\nLOCKS:::\n");
        lck  = lcreate ();
        assert (lck != SYSERR, "Test 3 failed 1");

        rd1 = create(reader3, 2000, 25, "reader3", 2, "reader A Lock", lck);
        rd2 = create(reader3, 2000, 30, "reader3", 2, "reader B Lock", lck);
        wr1 = create(writer3, 2000, 20, "writer3", 2, "writer Lock", lck);

        kprintf("-start writer, then sleep 1s. lock granted to write (prio 20)\n");
        resume(wr1);
        sleep (1);

        kprintf("-start reader A, then sleep 1s. reader A(prio 25) blocked on the lock\n");
        resume(rd1);
        sleep (1);
        kprintf("priority of writer:%d\n", getprio(wr1));

        kprintf("-start reader B, then sleep 1s. reader B(prio 30) blocked on the lock\n");
        resume (rd2);
        sleep (1);
        kprintf("priority of writer:%d\n", getprio(wr1));
    
        kprintf("-kill reader B, then sleep 1s\n");
        kill (rd2);
        sleep (1);
        kprintf("priority of writer:%d\n", getprio(wr1));

        kprintf("-kill reader A, then sleep 1s\n");
        kill (rd1);
        sleep(1);
        kprintf("priority of writer:%d\n", getprio(wr1));

        sleep (8);
}

int task1( )
{
	kprintf("Semaphore priority inversion\n");
    testsemaphore();
	kprintf("Locks priority inversion\n");
	testlocks();

        /* The hook to shutdown QEMU for process-like execution of XINU.
 *  *          * This API call exists the QEMU process.
 *   *                   */
        shutdown();
}


