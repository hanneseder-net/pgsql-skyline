/*
 *	sldg.c
 *
 *	DESCRIPTION:
 *		Random dataset generator for SKYLINE operator evaluation.
 *
 *	AUTHOR:
 *		based on the work by the authors of based [Borzsonyi2001]
 *		modified by Hannes Eder <Hannes@HannesEder.net>
 *		thanks to Donald Kossmann for providing the source code
 *		of the original implementation
 *
 *	REFERENCES:
 *		[Borzsonyi2001] B�rzs�nyi, S.; Kossmann, D. & Stocker, K.: 
 *		The Skyline Operator, ICDE 2001, 421--432
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string.h>

#include "port.h"

/*
 * some macros for warnings/errors
 */
const char *progname = "sldg";

#define warning(FMT, ...) fprintf(stderr, "%s: warning: " FMT "\n", progname, ##__VA_ARGS__)
#define invalidargs(FMT, ...) do { fprintf(stderr, "%s: error: " FMT "\n", progname, ##__VA_ARGS__); usage(); exit(1); } while (0)
#define fatal(FMT, ...) do { fprintf(stderr, "%s: error: " FMT "\n", progname, ##__VA_ARGS__); exit(1); } while (0)

/*
 * forward decl's
 */
static double sqr(double a);

static int stats_vector_count;
static double *stats_sum_x;
static double *stats_sum_x_sqr;
static double *stats_sum_x_prod;

static void stats_init(int dim);
static void stats_enter(int dim, double *x);
static void stats_output(int dim);

static double random_equal(double min, double max);
static double random_peak(double min, double max, int dim);
static double random_normal(double med, double var);

static void output_vector(int dim, double *x);
static int is_vector_ok(int dim, double *x);

static void generate_indep(int count, int dim);
static void generate_corr(int count, int dim);
static void generate_anti(int count, int dim);

static void usage();

/*
 * main
 */
int
main(int argc, char **argv)
{
	int c;
	int dim = 0;
	int count = 0;
	int dist = 0;
	int header = 0;
	int stats = 0;

	/*
	 * process command line arguments 
	 */
	while ((c = getopt(argc, argv, "icad:n:s:pSh?")) != -1)
	{
		switch (c)
		{
		case 'i':
		case 'c':
		case 'a':
			if (dist != 0 && dist != c)
				invalidargs("distribution already selected, hint: use only one of -i | -c | -a");
			dist = c;
			break;

		case 'd':
			dim = atoi(optarg);
			break;

		case 'n':
			count = atoi(optarg);
			break;

		case 'p':
			header = 1;
			break;

		case 'S':
			stats = 1;
			break;

		case 's':
			srand(atoi(optarg));
			break;

		case 'h':
		case '?':
			usage();
			return 0;

		default:
			usage();
			return 1;
		}
	}

	/*
	 * check for additional arguments
	 */
	while (optind < argc)
	{
		warning("ignoring additional argument `%s'", argv[optind]);
		optind++;
	}

	/*
	 * perform sanity checks on arguments
	 */
	if (dist == 0)
		invalidargs("no distribution selected");

	if (dim < 1)
		invalidargs("dimension less than 1");

	if (dim < 2 && dist != 'i')
	{
		dist = 'i';
		warning("for 1 dimensional data, independent distribution is used");
	}

	if (count < 0)
		invalidargs("the number of vectors must be non negative");

	/*
	 * here we go
	 */
	stats_init(dim);

	if (header)
		fprintf(stdout, "%d %d\n", count, dim);

	switch (dist)
	{
	case 'i':
		generate_indep(count, dim);
		break;
	case 'c':
		generate_corr(count, dim);
		break;
	case 'a':
		generate_anti(count, dim);
		break;
	default:
		fatal("distribution `%c' is not supported", dist);
		break;
	}

	if (stats)
		stats_output(dim);

	return 0;
}

/*
 * sqr
 */
static double
sqr(double a)
{
	return a*a;
}

/*
 * stats_init
 *
 *	inits the statistics, what else ;) 
 */
void
stats_init(int dim)
{
	int		d;

	/*
	 * We don't explicitly free this buffers, since they are freed when
	 * the program terminates anyway.
	 */
	stats_sum_x = (double *) malloc(dim * sizeof(double));
	stats_sum_x_sqr = (double *) malloc(dim * sizeof(double));
	stats_sum_x_prod = (double *) malloc(dim * dim * sizeof(double));

	stats_vector_count = 0;

	for (d = 0; d < dim; d++)
	{
		int		dd;

		stats_sum_x[d] = 0.0;
		stats_sum_x_sqr[d] = 0.0;
		for (dd = 0; dd < dim; dd++)
			stats_sum_x_prod[d * dim + dd] = 0.0;
	}
}


/*
 * stats_enter
 *
 *	adds the vector x[0..dim-1] into the stats
 */
void
stats_enter(int dim, double *x)
{
	int		d;
	stats_vector_count++;
	for (d = 0; d < dim; d++)
	{
		int		dd;

		stats_sum_x[d] += x[d];
		stats_sum_x_sqr[d] += x[d] * x[d];
		for (dd = 0; dd < dim; dd++)
			stats_sum_x_prod[d * dim + dd] += x[d] * x[dd];
	}
}

/*
 * stats_output
 *
 *	write the statistics to STDERR
 */
void
stats_output(int dim)
{
	int		d;

	/*
	 *	mean, var, sd
	 */
	for (d = 0; d < dim; d++)
	{
		double E = stats_sum_x[d] / stats_vector_count;
		double V = stats_sum_x_sqr[d] / stats_vector_count - E * E;
		double s = sqrt(V);
		fprintf(stderr, "# E[X%d]=%5.2f Var[X%d]=%5.2f s[X%d]=%5.2f\n", d + 1, E, d + 1, V,
			d + 1, s);
	}

	/*
	 * correlation factor matrix
	 */
	fprintf(stderr, "#\n# correlation factor matrix:\n");
	for (d = 0; d < dim; d++) {
		int		dd;

		fprintf(stderr, "#");
		for (dd = 0; dd < dim; dd++) {
			double cov =
				(stats_sum_x_prod[d * dim + dd] / stats_vector_count) -
				(stats_sum_x[d] / stats_vector_count) * (stats_sum_x[dd] /
				stats_vector_count);
			double cor =
				cov / sqrt(stats_sum_x_sqr[d] / stats_vector_count -
				sqr(stats_sum_x[d] / stats_vector_count)) /
				sqrt(stats_sum_x_sqr[dd] / stats_vector_count -
				sqr(stats_sum_x[dd] / stats_vector_count));
			fprintf(stderr, " %5.2f", cor);
		}
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "#\n# %d vector(s) generated\n", stats_vector_count);
}

/*
 * random_equal
 *
 *	returns a random value x \in [min,max]	
 */
static double
random_equal(double min, double max)
{
	double x = (double) rand() / RAND_MAX;
	return x * (max - min) + min;
}

/*
 * random_peak
 *
 *	Returns a random value x \in [min,max) as sum of dim equally 
 *	distributed random values.
 */
static double
random_peak(double min, double max, int dim)
{
	int		d;
	double	sum = 0.0;

	for (d = 0; d < dim; d++)
		sum += random_equal(0, 1);
	sum /= dim;
	return sum * (max - min) + min;
}

/*
 * random_normal
 *
 *	Returns a normally distributed random value x \in (med-var,med+var)
 *	with E[x] = med.
 *
 *	NOTE: This implementation works well if the random values returned by
 *	the underlaying random_equal are sufficiently independent.
 */
static double
random_normal(double med, double var)
{
	return random_peak(med - var, med + var, 12);
}

/*
 * output_vector
 *
 *	Writes the vector x[0..dim-1] to stdout, separated by spaces and with a
 *	trailing linefeed.
 */
static void
output_vector(int dim, double *x)
{
	while (dim--)
          fprintf(stdout, "%.*e%s", DBL_DIG, *(x++), dim ? " " : "\n");
}

/*
 * is_vector_ok
 *
 *	returns 1 iif all x_i \in [0,1]
 */
static int
is_vector_ok(int dim, double *x)
{
	while (dim--)
	{
		if (*x < 0.0 || *x > 1.0)
			return 0;
		x++;
	}

	return 1;
}

/*
 * generate_indep
 *
 *	Generate count vectors x[0..dim-1] with x_i \in [0,1] independently
 *	and equally distributed and outputs them to STDOUT.
 */
static void
generate_indep(int count, int dim)
{
	double *x = (double *) malloc(sizeof(double) * dim);

	while (count--)
	{
		int		d;

		for (d = 0; d < dim; d++)
			x[d] = random_equal(0, 1);

		output_vector(dim, x);
		stats_enter(dim, x);
	}

	free(x);
}

/*
 * generate_corr
 *
 *	Generates count vectors x[0..dim-1] with x_i \in [0,1].
 *	The x_i are correlated, i.e. if x is high in one dimension
 *	it is likely that x is high in another.
 */
static void
generate_corr(int count, int dim)
{
	double *x = (double *) malloc(sizeof(double) * dim);

	while (count--)
	{
		do
		{
			int		d;
			double	v = random_peak(0, 1, dim);
			double	l = v <= 0.5 ? v : 1.0 - v;

			for (d = 0; d < dim; d++)
				x[d] = v;

			for (d = 0; d < dim; d++)
			{
				double h = random_normal(0, l);
				x[d] += h;
				x[(d + 1) % dim] -= h;
			}
		} while (!is_vector_ok(dim, x));

		output_vector(dim, x);
		stats_enter(dim, x);
	}

	free(x);
}

/*
 * generate_anti
 *
 *	Generates count vectors x[0..dim-1] with x_i in [0,1], such that
 *	if x is high in one dimension it is likely that x is low in another.
 */
static void
generate_anti(int count, int dim)
{
	double *x = (double *) malloc(sizeof(double) * dim);

	while (count--)
	{
		do
		{
			int		d;
			double	v = random_normal(0.5, 0.25);
			double	l = v <= 0.5 ? v : 1.0 - v;
			
			for (d = 0; d < dim; d++)
				x[d] = v;
		
			for (d = 0; d < dim; d++)
			{
				double h = random_equal(-l, l);
				x[d] += h;
				x[(d + 1) % dim] -= h;
			}
		} while (!is_vector_ok(dim, x));

		output_vector(dim, x);
		stats_enter(dim, x);
	}
}

/*
 * usage
 */
static void
usage()
{
	fprintf(stderr, 
"\
Test data generator for Skyline Operator Evaluation\n\
usage: %s (-i|-c|-a) -d DIM -n COUNT [-s SEED] [-p] [-S] [-h|-?]\n\
\n\
Options:\n\
       -i       independent (dim >= 1)\n\
       -c       correlated (dim >= 2)\n\
       -a       anti-correlated (dim >= 2)\n\
\n\
       -d DIM   dimensions >=1\n\
       -n COUNT number of vectors\n\
\n\
       -s SEED  set random generator seed to SEED\n\
\n\
       -p       print dimension and number of vectors\n\
\n\
       -S       output stats to stderr\n\
\n\
       -h -?    display this help message and exit\n\
\n\
Examples:\n\
       %s -e -d 3 -n 10\n\
       %s -a -d 2 -n 100 -S -p\n\
\n\
", progname, progname, progname);
}
