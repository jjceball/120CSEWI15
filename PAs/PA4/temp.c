#include <setjmp.h>
#include <string.h>					  //for memset
#include "aux.h"
#include "umix.h"
#include "mythreads.h"

void partition(int parts);			  //partition recursion helper method

static int MyInitThreadsCalled = 0;	/* 1 if MyInitThreads called, else 0 */
static int first = 0;					//pointer to current thread
static int last = 0;						//pointer to last thread
static int recentSpawn = 0;			//last spawned thread
static int current;						//current thread
static int checkSpawned;			   //check if spawned
static int threadExists;			   //number of existing threads
static int threadYield;					//yield check
static int threadExit;					//exit check
static int queue[MAXTHREADS];			//queue of threads

static struct thread {					//thread table
int valid;									//1 for true, 0 for false
jmp_buf env;								//current context
jmp_buf newEnv;							//new context
void (*f)();								//function thread will execute
int p;										//parameter to the function thread will execute
int yielded;								//yield check on thread
} thread[MAXTHREADS];


#define STACKSIZE	65536	/* maximum size of thread stack */

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
		  }

		  if(setjmp(thread[0].env) == 0){
		    memcpy(thread[0].newEnv, thread[0].env, sizeof(thread[0].env));
		  }
		  else{
		    (*thread[0].f)(thread[0].p);
			 MyExitThread();
		  }

		  partition(MAXTHREADS);
		  current = 0;
		  thread[current].valid = 1;
		  queue[0] = current;
		  threadExists = 1;
		  checkSpawned = 0;
		  threadExit = 0;
		  MyInitThreadsCalled = 1;

}

/*	MySpawnThread (func, param) spawns a new thread to execute
 *	func (param), where func is a function with no return value and
 *	param is an integer parameter.  The new thread does not begin
 *	executing until another thread yields to it.  
 */

int MySpawnThread (func, param)
        void (*func)();	/* function to be executed */
	     int param;	/* integer parameter */
{

   	  if (! MyInitThreadsCalled) {
	   	   	 Printf ("SpawnThread: Must call InitThreads first\n");
					 Exit ();
		  }

        int i, j, free;

		  free = 0;
        j = recentSpawn;							//start from last spawned

		  if(threadExists == MAXTHREADS - 1){
		    checkSpawned = 1;
		  }

		  for(i = 0; i < MAXTHREADS; i++){	//find available spot in array
		    if(j == MAXTHREADS -1){
		      j = 0;
		    }
		    else{
		      j++;
		    }
		    if(thread[j].valid == 0){
		      free = 1;
			   break;
		    }
		  }
			
		  if(!free){								//*NEEDED CONDITION* if no free spot, exit
		    return -1;
		  }

		  recentSpawn = j;							//save free spot

		  thread[j].f = func;
		  thread[j].p = param;
		  thread[j].valid = 1;

		  if(checkSpawned){
		    memcpy(thread[j].env, thread[j].newEnv, sizeof(thread[j].newEnv)); //refresh env
		  }

	     if(last == MAXTHREADS - 1){
		    return -1;
		  }
		  else{
		    last++;
		    queue[last] = j;
		  }

		  threadExists++;

		  return j;

}

/*	MyYieldThread (t) causes the running thread to yield to thread t.  
 *	Returns ID of thread that yielded to t (i.e., the thread that called
 *	MyYieldThread), or -1 if t is an invalid ID.  
 */

int MyYieldThread (t)
		  int t;								/* thread being yielded to */
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

		  int i, j;

		  threadYield = current;							//check if yielding to itself
		   
		  if(threadYield == t){
		    return threadYield;
		  }

		  if(setjmp(thread[current].env) == 0){

			 for(i = 0; i < (last+1); i++){
			   if(queue[i] == t){
				  for(; i < last; i++){
				    queue[i] = queue[i+1];
				  }
				  last--;
				  break;
				}
			 }

			 if(!threadExit){
			   if(last == MAXTHREADS - 1){
			     return -1;
			   }
			   else{
				  last++;
				  queue[last] = queue[first];
            }
			 }

			 threadExit = 0;

		    queue[first] = t;

			 thread[t].yielded = current;
			 current = t;
          longjmp(thread[t].env, 1);
        } 
		  else {
          return thread[current].yielded; 
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

		  return current;
}

/*	MySchedThread () causes the running thread to simply give up the
 *	CPU and allow another thread to be scheduled. Selecting which
 *	thread to run is determined here. Note that the same thread may
 * be chosen (as will be the case if there are no other threads).  
 */

void MySchedThread ()
{
		  if (! MyInitThreadsCalled) {
					 Printf ("SchedThread: Must call InitThreads first\n");
					 Exit ();
		  }

		  if((!threadExit) && queue[first] == -1){
		    MyYieldThread(current);
		  }
		  else if(threadExit && queue[first] == -1){
		    Exit();
		  }
		  else{
		    MyYieldThread(queue[first+1]);
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

		  int i = 0;
		  thread[current].valid = 0;
		  threadExists--;
		  threadExit = 1;

		  if(threadExists == 0){
		    Exit();
		  }

		  MySchedThread();

}

/* partition (int parts) recurses partitions. helper method.
 */

void partition (int parts) {

  if(parts <= 1){
    return;
  }
  else{

    char s[STACKSIZE];

    if(((int) &s[STACKSIZE-1]) - ((int) &s[0]) + 1 != STACKSIZE){
                Printf ("Stack space reservation failed\n");
		   		 Exit();
    }

    int updateThread = MAXTHREADS - parts + 1;

    if(setjmp(thread[updateThread].env) == 0){
	
      memcpy(thread[updateThread].newEnv, thread[updateThread].env,
	          sizeof(thread[updateThread].env));
	   
		partition(parts - 1);
    }
    else{
      (thread[current].f)(thread[current].p);
	   MyExitThread();
    }
  }
}
