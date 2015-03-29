#include <stdio.h>
#include "aux.h"
#include "umix.h"

void Main ()
{
	if (Fork () == 0) {

		if (Fork () == 0) {

			/* Process 4 */
			RequestCPUrate (1, 10);
			SlowPrintf (7, "444444444444444444");
			Exit ();
		}

		/* Process 2 */
		RequestCPUrate (3, 10);
		SlowPrintf (7, "222222222222222222");
		Exit ();
	}

	if (Fork () == 0) {

		/* Process 3 */
		RequestCPUrate (2, 10);
		SlowPrintf (7, "333333333333333333");
		Exit ();
	}

	/* Process 1 */
	RequestCPUrate (4, 10);
	SlowPrintf (7, "111111111111111111");
	Exit ();
}
