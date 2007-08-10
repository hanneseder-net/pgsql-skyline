#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <float.h>

extern char * __progname;

#define invalidargs(FMT, ...) fprintf(stderr, "%s: error: " FMT "\n", __progname, ##__VA_ARGS__), usage(), exit(1)
#define fatal(FMT, ...) fprintf(stderr, "%s: error: " FMT "\n", __progname, ##__VA_ARGS__), exit(1)

static double RandomNormal(double med, double var);

static void usage();

int
main(int argc, char **argv)
{
  int n = 0;
  int c;

  while ((c = getopt(argc, argv, "n:h?")) != -1) {
    switch (c) {
    case 'n':
      n = atoi(optarg);
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

  if (n < 0)
    invalidargs("invalid number of vectors");

  while (n--) {
    double x = RandomNormal(0, 1);
    printf("%.*e\n", DBL_DIG, x);
  }

  return 0;
}


static double
RandomEqual(double min, double max)
{
  double x = (double) rand() / RAND_MAX;
  return x * (max - min) + min;
}


/* liefert eine Zufallsvariable im Intervall [min,max[ 
 * als Summe von "dim" gleichverteilten Zufallszahlen 
 */
static double
RandomPeak(double min, double max, int dim)
{
  double sum = 0.0;
  for (int d = 0; d < dim; d++)
    sum += RandomEqual(0, 1);
  sum /= dim;
  return sum * (max - min) + min;
}

/* liefert eine normalverteilte Zufallsvariable mit Erwartungswert med 
 * im Intervall ]med-var,med+var[ 
 */
static double
RandomNormal(double med, double var)
{
  return RandomPeak(med - var, med + var, 12);
}


static void
usage()
{
  printf(
"\
Generate n number distributed N(0,1)\n\
usage: %s [OPTIONS]\n\
\n\
Options:\n\
       -n COUNT number of vectors\n\
\n\
", __progname);
}

