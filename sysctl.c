#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/vmmeter.h>
#include <uvm/uvmexp.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "sysctl.h"


static int
uvmexp_collector(struct uvmexp *uvmexp, char **err)
{
	size_t	sz = sizeof *uvmexp;
	int	mib[] = { CTL_VM, VM_UVMEXP };

	if (sysctl(mib, sizeof mib / sizeof mib[0], uvmexp, &sz, NULL, 0) == -1) {
		*err = strerror(errno);
		return -1;
	}
	return 0;
}

int
forks_collector(double *result, char **err)
{
	struct uvmexp	uvmexp;

	if (uvmexp_collector(&uvmexp, err) == -1) {
		return -1;
	}

	*result = uvmexp.forks;
	return 0;
}

int
csw_collector(double *result, char **err)
{
	struct uvmexp	uvmexp;

	if (uvmexp_collector(&uvmexp, err) == -1) {
		return -1;
	}

	*result = uvmexp.swtch;
	return 0;
}


int
intr_collector(double *result, char **err)
{
	struct uvmexp	uvmexp;

	if (uvmexp_collector(&uvmexp, err) == -1) {
		return -1;
	}

	*result = uvmexp.intrs;
	return 0;
}
