/* mykernel2.c: your portion of the kernel
 *
 *	Below are procedures that are called by other parts of the kernel. 
 *	Your ability to modify the kernel is via these procedures.  You may
 *	modify the bodies of these procedures any way you wish (however,
 *	you cannot change the interfaces).  
 * 
 */

#include "aux.h"
#include "sys.h"
#include "mykernel2.h"

#define TIMERINTERVAL 1		/* in ticks (tick = 10 msec) */
#define L 100				/* Used to calculate stride values */
#define debug 0

/*	A sample process table.  You may change this any way you wish.  
 */

static int cpuCount;

/* Round Robin */

static int rrCurr;		
static int rrNext;		

/* Proportional */

static int proCurr;		
static int proMin;

static float total; 		/* Checks total CPU usage */
static int totUnused;		/* Keeps track of total unused IDs */

static struct {
	int head;
	int tail;
	
	int fqArray[MAXPROCS];
} fq;						/* FIFO queue */

static struct {
	int top;
 
	int lsArray[MAXPROCS];
} ls;						/* LIFO stack */

static struct {
	int valid;		/* is this entry valid: 1 = yes, 0 = no */
	int pid;		/* process id (as provided by kernel) */
	int unused;
	
	float pass;			/* used in proportional */
	float stride;		/* used in proportional */
	float given;		/* used for requestcpurate */
} proctab[MAXPROCS];


/*	InitSched () is called when kernel starts up. First, set the
 *	scheduling policy (see sys.h). Make sure you follow the rules
 *	below on where and how to set it.  Next, initialize all your data
 *	structures (such as the process table).  Finally, set the timer
 *	to interrupt after a specified number of ticks.  
 */

void InitSched ()
{
	int i;

	/* First, set the scheduling policy.  You should only set it
	 * from within this conditional statement. While you are working
	 * on this assignment, GetSchedPolicy will return NOSCHEDPOLICY,
	 * and so the condition will be true and you may set the scheduling
	 * policy to whatever you choose (i.e., you may replace ARBITRARY). 
	 * After the assignment is over, during the testing phase, we will
	 * have GetSchedPolicy return the policy we wish to test, and so
	 * the condition will be false and SetSchedPolicy will not be
	 * called, thus leaving the policy to whatever we chose to test.  
	 */
	if (GetSchedPolicy () == NOSCHEDPOLICY) {	/* leave as is */
		SetSchedPolicy (PROPORTIONAL);		/* set policy here */
	}
		
	/* Initialize all your data structures here */
	for (i = 0; i < MAXPROCS; i++) {
		proctab[i].valid = 0;
		fq.fqArray[i] = 0;
		ls.lsArray[i] = 0;
		proctab[i].unused = 0;
		proctab[i].given = 0;
		proctab[i].pass = 0;
		proctab[i].stride = 0;
	}
	
	/* Initializing values */
	fq.head = 0;
	fq.tail = 0;

	ls.top = 0;
	
	rrCurr = 0;
	rrNext = 0;

	proCurr = 0;
	proMin = 0;

	totUnused = 0;

	/* Set the timer last */
	SetTimer (TIMERINTERVAL);
}


/*	StartingProc (pid) is called by the kernel when the process
 *	identified by pid is starting.  This allows you to record the
 *	arrival of a new process in the process table, and allocate
 *	any resources (if necessary).  Returns 1 if successful, 0 otherwise.  
 */

int StartingProc (pid)
	int pid;
{
	int i;
	totUnused++;

	for (i = 0; i < MAXPROCS; i++) {
		if (! proctab[i].valid) {
			proctab[i].valid = 1;
			proctab[i].pid = pid;
			proctab[i].given = 0;
			proctab[i].unused = 1;

		if (GetSchedPolicy() == FIFO) 
		{
			fq.fqArray[fq.tail] = i;
			fq.tail++;
			fq.tail = fq.tail%MAXPROCS;
		}

		if (GetSchedPolicy() == LIFO) 
		{
			ls.lsArray[ls.top] = i;
			DoSched();
			ls.top++;
		}
			return (1);
		
		}		
	}

	Printf ("Error in StartingProc: no free table entries\n");
	return (0);
}
			

/*	EndingProc (pid) is called by the kernel when the process
 *	identified by pid is ending. This allows you to update the
 *	process table accordingly, and deallocate any resources (if
 *	necessary). Returns 1 if successful, 0 otherwise.  
 */


int EndingProc (pid)
	int pid;
{
	int i;
	int j;

	if (GetSchedPolicy() == FIFO) 
	{
		fq.head++;
		fq.head = fq.head%MAXPROCS;
	}

	if (GetSchedPolicy() == LIFO) 
	{
		ls.top--;
		ls.top = ls.top%MAXPROCS;
	}

	for (i = 0; i < MAXPROCS; i++) {
		if (proctab[i].valid && proctab[i].pid == pid) {
			proctab[i].valid = 0;
			return (1);
		}
	}

	Printf ("Error in EndingProc: can't find process %d\n", pid);
	return (0);
}


/*	SchedProc () is called by kernel when it needs a decision for
 *	which process to run next.  It calls the kernel function
 *	GetSchedPolicy () which will return the current scheduling policy
 *	which was previously set via SetSchedPolicy (policy).  SchedProc ()
 *	should return a process id, or 0 if there are no processes to run.  
 */

int SchedProc ()
{
	int i;
	int j;

	switch (GetSchedPolicy ()) {

	case ARBITRARY:

		for (i = 0; i < MAXPROCS; i++) {
			if (proctab[i].valid) {
				return (proctab[i].pid);
			}
		}
		break;

	case FIFO:

		/* your code here */
		i = fq.fqArray[fq.head];
		
		if (proctab[i].valid)
		{
			return (proctab[i].pid);
		}

		break;

	case LIFO:

		/* your code here */
		j = ls.top - 1;
		i = ls.lsArray[j];
		
		if (proctab[i].valid) 
		{
			return (proctab[i].pid);
		}
		
		break;

	case ROUNDROBIN:
		/* your code here */
		for (i = 0; i < MAXPROCS; i++) 
		{
			rrCurr = rrNext%MAXPROCS;
			rrNext = rrNext + 1;
			
			if (proctab[rrCurr].valid){
				return (proctab[rrCurr].pid);
			}
		}
		
		break;

	case PROPORTIONAL:

		/* your code here */
		proMin = -1;
		proCurr = -1;

		for (i = 0; i < MAXPROCS; i++) {
			if (proctab[i].valid && proCurr == -1) 
			{
				proMin = proctab[i].pass;
				proCurr = i;
			}

			if (proctab[i].valid && (proctab[i].pass < proMin)) 
			{
				proMin = proctab[i].pass;
				proCurr = i;
			}
		}
		
		proctab[proCurr].pass += proctab[proCurr].stride;
		return proctab[proCurr].pid;
	
		break;
	}
	
	return (0);
}


/*	HandleTimerIntr () is called by the kernel whenever a timer
 *	interrupt occurs. 
 */

void HandleTimerIntr ()
{
	SetTimer (TIMERINTERVAL);

	switch (GetSchedPolicy ()) {	/* is policy preemptive? */

	case ROUNDROBIN:		/* ROUNDROBIN is preemptive */
	case PROPORTIONAL:		/* PROPORTIONAL is preemptive */

		DoSched ();		/* make scheduling decision */
		break;

	default:			/* if non-preemptive, do nothing */
		break;
	}
}

/*	MyRequestCPUrate (pid, m, n) is called by the kernel whenever a process
 *	identified by pid calls RequestCPUrate (m, n). This is a request for
 *	a fraction m/n of CPU time, effectively running on a CPU that is m/n
 *	of the rate of the actual CPU speed.  m of every n quantums should
 *	be allocated to the calling process.  Both m and n must be greater
 *	than zero, and m must be less than or equal to n.  MyRequestCPUrate
 *	should return 0 if successful, i.e., if such a request can be
 *	satisfied, otherwise it should return -1, i.e., error (including if
 *	m < 1, or n < 1, or m > n).  If MyRequestCPUrate fails, it should
 *	have no effect on scheduling of this or any other process, i.e., as
 *	if it were never called. 
 */

int MyRequestCPUrate (pid, m, n)
	int pid;
	int m;
	int n;
{
	/* your code here */
	int tmp = 0;
	int j = 0;
	int k = 0;

	float newTotal = total + (float)m/(float)n;

	/* Error Checking */
	if ((m < 1) || (n < 1) || (m > n) || (newTotal > 1)) 
	{
		return (-1);
	}
	
	for (j = 0; j < MAXPROCS; j++) {
		tmp = total;
		if ((proctab[j].pid == pid) && proctab[j].valid) 
		{
			proctab[j].given = (((float)m)/(float)n);
			proctab[j].stride = (float)L/(float)proctab[j].given;
			total += proctab[j].given;
			proctab[j].unused = 0; 
			totUnused--;
			
			if (total > 1) 
			{
				total = tmp;
				for (k = 0; k < MAXPROCS; k++) {
					if( proctab[k].unused = 1 )
						proctab[k].pass = (1-total)/totUnused;
				}
			}
		}
	}
	
	return (0);
}
