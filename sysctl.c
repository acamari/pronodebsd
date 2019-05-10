#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/vmmeter.h>
#include <uvm/uvmexp.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "sysctl.h"

int
forks_collector(double *result, char **err)
{
	struct forkstat	fs;
	size_t	sz = sizeof fs;
	int	mib[] = { CTL_KERN, KERN_FORKSTAT };

	if (sysctl(mib, sizeof mib / sizeof mib[0], &fs, &sz, NULL, 0) == -1) {
		*err = strerror(errno);
		return -1;
	}

	*result = fs.cntfork;
	return 0;
}

int
intr_collector(double *result, char **err)
{
	struct uvmexp	uvmexp;
	size_t	sz = sizeof uvmexp;
	int	mib[] = { CTL_VM, VM_UVMEXP };

	if (sysctl(mib, sizeof mib / sizeof mib[0], &uvmexp, &sz, NULL, 0) == -1) {
		*err = strerror(errno);
		return -1;
	}

	*result = uvmexp.intrs;
	return 0;
}
