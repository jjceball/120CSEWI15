/*	User-level thread system
 *
 */

#include <setjmp.h>

#include "aux.h"
#include "umix.h"
#include "mythreads.h"

static int MyInitThreadsCalled = 0;	/* 1 if MyInitThreads called, else 0 */
static int curr = 0;				/* Position of current thread */
static int next = -1;				/* Next thread value */
static int newIndex = -1;			/* New index value */

static struct thread {				/* Thread table */
	int valid;						/* 1 if entry is valid, else 0 */
	int id;							/* Keep track of thread ID */
	int yieldFlag;					/* Keep track of thread yielded too */
	int pt;
	void (*ft)();
	jmp_buf env;					/* Current context */
	jmp_buf newEnv;					/* New context */
} thread[MAXTHREADS];

static int waiting[MAXTHREADS];		/* Queue to hold waiting thread ID */

#define STACKSIZE	65536			/* Maximum size of thread stack */

/* Helper function used to shift items in 
   the queue to the left at a specific index */
void shiftQueue (int index) 
{
	for (int i = index; i < MAXTHREADS - 1; i++) 
	{
		waiting[i] = waiting[i+1];
		waiting[i+1] = -1;
	}
}

/* Helper function to add an element to the end of the queue */
void addToQueue (int toInsert) 
{
	for (int i = 0; i < MAXTHREADS; i++) 
	{
		if (waiting[i] == -1) 
		{
			waiting[i] = toInsert;
			break;
		}
	}
}

/* Function that prints the queue */
void printQ () 
{
	for (int i = 0; i < MAXTHREADS; i++) 
	{
		Printf ("waiting = %d\n", waiting[i]);
	}
}

/* Function that partitions the stack in initThread() */
void stackPartition (int numPartitions) 
{
	if (numPartitions <= 1) 
	{
		return;
	}
	else 
	{
		char s[STACKSIZE];

		if(((int) &s[STACKSIZE-1]) - ((int) &s[0]) + 1 != STACKSIZE) 
		{
			Printf("Stack space reservation failed\n");
			Exit();
		}

		int updateThread = MAXTHREADS - numPartitions + 1;

		if(setjmp(thread[updateThread].env) == 0) 
		{
			memcpy(thread[updateThread].newEnv, thread[updateThread].env, sizeof(jmp_buf));
			stackPartition(numPartitions -1);
		}	
		else 
		{
			(thread[curr].ft)(thread[curr].pt);
			MyExitThread();
		}
	}
}

/*	MyInitThreads () initializes the thread package. Must be the first
 *	function called by any user program that uses the thread package.  
 */

void MyInitThreads ()
{
	int i;

	if (MyInitThreadsCalled) {                /* run only once */
		Printf ("InitThreads: should be called only once\n");
		Exit ();
	}
	
	for (i = 0; i < MAXTHREADS; i++) {	/* initialize thread table */
		thread[i].valid = 0;
		thread[i].id = -1;
		waiting[i] = -1;
	}
	
	if (setjmp(thread[0].env) == 0) 
	{
		memcpy(thread[0].newEnv, thread[0].env, sizeof(jmp_buf));
	}
	else 
	{
		(thread[0].ft)(thread[0].pt);
		MyExitThread();
	}

	stackPartition(MAXTHREADS);
	
	thread[0].valid = 1;			/* initialize thread 0 */
	waiting[0] = curr;	
	MyInitThreadsCalled = 1;
}

/*	MySpawnThread (func, param) spawns a new thread to execute
 *	func (param), where func is a function with no return value and
 *	param is an integer parameter.  The new thread does not begin
 *	executing until another thread yields to it.  
 */

int MySpawnThread (func, param)
	void (*func)();		/* function to be executed */
	int param;		/* integer parameter */
{
	if (! MyInitThreadsCalled) {
		Printf ("SpawnThread: Must call InitThreads first\n");
		Exit ();
	}
	
	int validSlot = 0; 
	
	// Count the # of empty slots available
	for (int i = 0; i < MAXTHREADS; i++) 
	{
		validSlot += thread[i].valid;	
	}
	
	// If all the slots are full, return -1
	if (validSlot == MAXTHREADS) 
	{
		return -1;
	}
	
	newIndex = ((newIndex+1)%MAXTHREADS);	
	
	// Loop through the thread table to find the last index
	for (int i = 0; i < MAXTHREADS; i++) 
	{
		if (thread[newIndex].valid == 0) 
		{ 										// Empty
			thread[newIndex].valid = 1; 		// Reset valid
			thread[newIndex].id = newIndex; 	// Change the ID value
			thread[newIndex].ft = func; 		// Saves the function
			thread[newIndex].pt = param; 		// Saves the parameter
			addToQueue(newIndex); 				// Put new ID in waiting queue
			
			// Make sure the previous environment is overwritten
			memcpy(thread[newIndex].env, thread[newIndex].newEnv, sizeof(jmp_buf));
			return (newIndex);
		}
		else 
		{ 	// Loop around 
			newIndex = ((newIndex+1)%MAXTHREADS);
		}
	}

	return (-1);		/* done spawning, return new thread ID */
}

/*	MyYieldThread (t) causes the running thread, call it T, to yield to
 *	thread t.  Returns the ID of the thread that yielded to the calling
 *	thread T, or -1 if t is an invalid ID.  Example: given two threads
 *	with IDs 1 and 2, if thread 1 calls MyYieldThread (2), then thread 2
 *	will resume, and if thread 2 then calls MyYieldThread (1), thread 1
 *	will resume by returning from its call to MyYieldThread (2), which
 *	will return the value 2.
 */

int MyYieldThread (t)
	int t;				/* thread being yielded to */
{
	if (! MyInitThreadsCalled) {
		Printf ("YieldThread: Must call InitThreads first\n");
		Exit ();
	}

	if (t < 0 || t >= MAXTHREADS) {
		Printf ("YieldThread: %d is not a valid thread ID\n", t);
		return (-1);
	}
	if (! thread[t].valid) {
		Printf ("YieldThread: Thread %d does not exist\n", t);
		return (-1);
	}

	if (t == curr) 
	{
		return t; 
	}
	
	if (setjmp (thread[curr].env) == 0) 
	{
		for (int i = 0; i < MAXTHREADS; i++) 
		{
			// Get the thread out of the waiting queue
			if (waiting[i] == t) 
			{
				shiftQueue(i);
				waiting[0] = t;
				break;		
			}
		}
	
		// Add the value to the end of the queue	
		addToQueue(curr);
		thread[t].yieldFlag = curr;
		curr = t;
        longjmp (thread[t].env, 1); // Go to the next thread
	}
	else 
	{
		return thread[curr].yieldFlag; // Return the value yielded too
	}
}

/*	MyGetThread () returns ID of currently running thread.  
 */

int MyGetThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("GetThread: Must call InitThreads first\n");
		Exit ();
	}
	return curr;

}

/*	MySchedThread () causes the running thread to simply give up the
 *	CPU and allow another thread to be scheduled. Selecting which
 *	thread to ADrun is determined here. Note that the same thread may
 * 	be chosen (as will be the case if there are no other threads).  
 */

void MySchedThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("SchedThread: Must call InitThreads first\n");
		Exit ();
	}
	
	int old = waiting[0];

	if (waiting[1] == -1) 
	{
		MyYieldThread(old);
	}
	else if (waiting[1] != -1) 
	{
		MyYieldThread(waiting[1]);
	}
	else 
	{
		Exit();
	}	
}

/*	MyExitThread () causes the currently running thread to exit.  
 */

void MyExitThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("ExitThread: Must call InitThreads first\n");
		Exit ();
	}

	thread[curr].valid = 0;	
	shiftQueue(0);

	// If the waiting queue is empty, check the next available
	if (waiting[0] == -1) 
	{
		Exit();
	}
	else 
	{
		thread[waiting[0]].yieldFlag = curr;
		curr = waiting[0];
		longjmp(thread[curr].env, 1); 
	}
}
