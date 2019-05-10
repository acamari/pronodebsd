#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/vmmeter.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "sysctl.h"

int
forks_collector(double *result, char **err)
{
	struct forkstat	fs;
	size_t	fssz = sizeof fs;
	int	mib[] = { CTL_KERN, KERN_FORKSTAT };

	if (sysctl(mib, sizeof mib / sizeof mib[0], &fs, &fssz, NULL, 0) == -1) {
		*err = strerror(errno);
		return -1;
	}

	*result = fs.cntfork;
	return 0;
}
