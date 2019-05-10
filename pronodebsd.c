#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <err.h>
#include <errno.h>
#include <time.h>

#include "sysctl.h"

int		metricsf(void);

static int	loadavg_collector1(double *, char **);
static int	loadavg_collector5(double *, char **);
static int	loadavg_collector15(double *, char **);
static int	boottime_collector(double *, char **);

enum METRIC_TYPES { MTYPE_UNTYPED = 0, MTYPE_COUNTER, MTYPE_GAUGE, MTYPE_HISTOGRAM, MTYPE_SUMMARY };

static struct metrics_t {
	char	*name;
	char	*help;
	enum METRIC_TYPES	type;
	int	(*collector)(double *, char **);
} metrics[] = {
	{ "node_load1", "1m load average", MTYPE_GAUGE, loadavg_collector1 },
	{ "node_load5", "5m load average", MTYPE_GAUGE, loadavg_collector5 },
	{ "node_load15", "15m load average", MTYPE_GAUGE, loadavg_collector15 },
	{ "node_boot_time_seconds", "Node boot time, in unixtime.", MTYPE_GAUGE, boottime_collector },
	{ "node_forks_total", "Total number of forks.", MTYPE_GAUGE, forks_collector },
	{ "node_intr_total", "Total number of interrupts serviced.", MTYPE_GAUGE, intr_collector },
	{ NULL, NULL, 0, NULL }
};

int
metricsf(void)
{
	char	buf[4096] = "\0";
	char	*p = buf;
	int	i;

	for (i = 0; metrics[i].name != NULL; i++) {
		struct	metrics_t	*m;
		double	value = 0;
		int	c; /* char count */
		char	*err;

		m = &metrics[i];
		if (m->collector(&value, &err) == -1) {
			warnx("%s: %s", m->name, err);
			return (-1);
		}
		c = snprintf(p, (buf + sizeof buf - p),
				"# HELP %s %s\n"
				"# TYPE %s counter\n"
				"%s %.10lg\n",
				m->name, m->help,
				m->name,
				m->name, value);
		if (c < 0) {
			warn("snprintf");
			return (-1);
		}
		p += c;

	}

	printf("%s", buf);
	return (0);
}

static int
loadavg_collector(double *result, char **err, int idx)
{
	double loadavg[3] = { 0, 0, 0 };

	if (idx < 0 || idx > 2) {
		*err = "invalid idx";
		return -1;
	}

	if (getloadavg(loadavg, sizeof loadavg) == -1) {
		*err = strerror(errno);

		return -1;
	}
	*result = loadavg[idx];
	return 0;
}

static int
loadavg_collector1(double *result, char **err)
{
	return loadavg_collector(result, err, 0);
}

static int
loadavg_collector5(double *result, char **err)
{
	return loadavg_collector(result, err, 1);
}

static int
loadavg_collector15(double *result, char **err)
{
	return loadavg_collector(result, err, 2);
}

static int
boottime_collector(double *result, char **err)
{
	struct timespec	tm;

	if (clock_gettime(CLOCK_BOOTTIME, &tm) == -1) {
		*err = strerror(errno);

		return -1;
	}
	*result = (double)(time(NULL) - tm.tv_sec);
	return 0;
}

int
main(int argc, char *argv[])
{
	metricsf();
	return 0;
}
