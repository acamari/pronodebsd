#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <err.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include "sysctl.h"

struct mresult {
	int	labelsz;	/* size allocated for labels */
	int	nlabelsz;	/* size should be allocated for labels */
	char	*label;		/* one or more labels in prometheus format */
	double	value;
};


void		procmetrics(void);

static int	loadavg_collector1(double *, char **);
static int	loadavg_collector5(double *, char **);
static int	loadavg_collector15(double *, char **);
static int	boottime_collector(double *, char **);

static int	uname_collector(struct mresult *, int, char **, int *);

enum METRIC_TYPES { MTYPE_UNTYPED = 0, MTYPE_COUNTER, MTYPE_GAUGE, MTYPE_HISTOGRAM, MTYPE_SUMMARY };

static struct metrics_t {
	char	*name;
	char	*help;
	enum METRIC_TYPES	type;
	/* use only one of collector or collectorv, if both are non-null collector has preference */
	int	(*collector)(double *, char **);
	int	(*collectorv)(struct mresult *, int, char **, int *);
	int	nelem; /* number of elements the collector returned in the last invocation */
	/* storage for collectors that return complex results */
	struct mresult	*mres;
} metrics[] = {
	{ "node_load1", "1m load average", MTYPE_GAUGE, loadavg_collector1, NULL, 0, NULL },
	{ "node_load5", "5m load average", MTYPE_GAUGE, loadavg_collector5, NULL, 0, NULL },
	{ "node_load15", "15m load average", MTYPE_GAUGE, loadavg_collector15, NULL, 0, NULL },
	{ "node_boot_time_seconds", "Node boot time, in unixtime.", MTYPE_GAUGE, boottime_collector, NULL, 0, NULL },
	{ "node_context_switches_total", "Total number of context switches.", MTYPE_GAUGE, csw_collector, NULL, 0, NULL },
	{ "node_forks_total", "Total number of forks.", MTYPE_GAUGE, forks_collector, NULL, 0, NULL },
	{ "node_intr_total", "Total number of interrupts serviced.", MTYPE_GAUGE, intr_collector, NULL, 0, NULL },
	{ "node_uname_info", "Labeled system information as provided by the uname system call.", MTYPE_GAUGE, NULL, uname_collector, 0, NULL },
	{ NULL, NULL, 0, NULL, NULL, 0, NULL }
};

void
procmetrics(void)
{
	int	i;

	for (i = 0; metrics[i].name != NULL; i++) {
		struct	metrics_t	*m;
		double	value = 0;
		char	*err;

		m = &metrics[i];
		if (m->collector) { /* scalar collector */
			int	c; /* char count */

			if (m->collector(&value, &err) == -1) {
				warnx("%s: %s", m->name, err);
				return;
			}
			c = printf(	"# HELP %s %s\n"
					"# TYPE %s counter\n"
					"%s %.10lg\n",
					m->name, m->help,
					m->name,
					m->name, value);
			if (c < 0) {
				warn("printf");
				return;
			}
			continue;
		}

		if (!m->collectorv)
			errx(1, "no collector defined for metric %s", m->name);

		/* process metric with collector that returns several results */
		probecollector:	for (;;) {
			struct mresult *r;
			int newnelem;
			int el;

			if (m->collectorv(m->mres, m->nelem, &err, &newnelem) == -1) {
				warnx("%s: %s", m->name, err);
				return;
			}
			/* assign enough memory for new number of
			 * elements */
			if (newnelem > m->nelem) {
				struct mresult *tmp;

				warnx("realloc, %p", m->mres);
				tmp = recallocarray(m->mres, m->nelem,
						newnelem,
						sizeof(*m->mres));
				if (tmp == NULL) {
					errx(1, "realloc");
				}
				m->mres = tmp;
				m->nelem = newnelem;
				warnx("realloced, %p, %zd", m->mres,
						sizeof(*m->mres) * m->nelem);
				continue;
			}

			/* look into each element and see if
			 * they have enough space */
			for (el = 0, r = m->mres;
					el < m->nelem;
					el++, r++) {
				if (r->nlabelsz > r->labelsz) {
					char *tmp;

					tmp = realloc(r->label, r->nlabelsz);
					if (tmp == NULL) {
						errx(1, "realloc");
					}
					r->label = tmp;
					r->labelsz = r->nlabelsz;
					/*
					warnx("probecollector enospace in label");
					*/
					goto probecollector;
				}
			}

			break;

		}

		{
			int	c; /* char count */

			c = printf(	"# HELP %s %s\n"
					"# TYPE %s counter\n",
					m->name, m->help,
					m->name);
			if (c < 0) {
				warn("printf");
				return;
			}
		}

		for (int i = 0; i < m->nelem; i++) {
			int	c; /* char count */

			c = printf(	"%s{%s} %.10lg\n",
					m->name, m->mres[i].label,
					m->mres[i].value);
			if (c < 0) {
				warn("printf");
				return;
			}

		}
	}
	return;
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

static int
uname_collector(struct mresult *mres, int nelem, char **err, int *newnelem)
{
	char	lbl[] = "domainname=\"(none)\"";
	char	lbl2[] = "domai__ome=\"(nane)\"";
	size_t sz = sizeof(lbl);

	*newnelem = 2;
	if (nelem < 2) {
		return 0;
	}

	if (mres[0].labelsz < sz) { /* not enough space */
		mres[0].nlabelsz = sz;
		return 0;
	}
	if (mres[1].labelsz < sz) { /* not enough space */
		mres[1].nlabelsz = sz;
		return 0;
	}

	mres[0].labelsz = sz;
	mres[0].nlabelsz = sz;
	mres[0].value = 1;
	strlcpy(mres[0].label, lbl, mres[0].labelsz);
	mres[1].labelsz = sz;
	mres[1].nlabelsz = sz;
	mres[1].value = 2;
	strlcpy(mres[1].label, lbl2, mres[1].labelsz);
	return 0;
}

int
main(int argc, char *argv[])
{
	for (;;) {
		procmetrics();
	}
	return 0;
}
