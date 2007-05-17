/*
 * Kossmann's SkyLine Data Generator
 * based on kossmann-main.cpp
 * modified by Hannes Eder <Hannes@HannesEder.net>
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

FILE *fout = NULL;
FILE *flog = NULL;

extern char * __progname;

#define invalidargs(FMT, ...) fprintf(stderr, "%s: error: " FMT "\n", __progname, ##__VA_ARGS__), usage(), exit(1)
#define fatal(FMT, ...) fprintf(stderr, "%s: error: " FMT "\n", __progname, ##__VA_ARGS__), exit(1)

static double sqr(double a) { return a*a; }

int Statistics_Count;
double *Statistics_SumX;
double *Statistics_SumXsquared;
double *Statistics_SumProduct;

void InitStatistics(int Dimensions);
void EnterStatistics(int Dimensions, double *x);
void OutputStatistics(int Dimensions);

double RandomEqual(double min, double max);
double RandomPeak(double min, double max, int dim);
double RandomNormal(double med, double var);

void OutputVector(int Dimensions, double *x);
int IsVectorOk(int Dimensions, double *x);

void GenerateDataEqually(int Count, int Dimensions);
void GenerateDataCorrelated(int Count, int Dimensions);
void GenerateDataAnticorrelated(int Count, int Dimensions);

static void usage();

int
main(int argc, char **argv)
{
  int c;
  int dim = 0;
  int count = 0;
  int dist = 0;
  int header = 0;
  int stats = 0;
  char * filename = "-";

  while ((c = getopt(argc, argv, "specad:n:o:h?")) != -1) {
    switch (c) {
    case 'e':
    case 'c':
    case 'a':
      if (dist != 0 && dist != c)
        invalidargs("distribution already selected, hint: use only one of -e | -c | -a");
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

    case 's':
      stats = 1;
      break;

    case 'o':
      filename = optarg;
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

  if (dist == 0)
    invalidargs("no distribution selected");

  if (dim < 2)
    invalidargs("dimension less than 2");

  if (count <= 0)
    invalidargs("invalid number of vectors");

  for (int index = optind; index < argc; index++)
    filename = argv[index];

  if (strcmp(filename, "-") == 0) {
    fout = stdout;
    filename = "<stdout>";
  }  
  else
    fout = fopen(filename, "wt");

  if (fout == NULL)
    fatal("can't open `%s' for output: %s", filename, strerror(errno));

  flog = stderr;

  InitStatistics(dim);

  if (header)
    fprintf(fout, "%d %d\n", count, dim);

  switch (dist) {
  case 'e':
    GenerateDataEqually(count, dim);
    break;
  case 'c':
    GenerateDataCorrelated(count, dim);
    break;
  case 'a':
    GenerateDataAnticorrelated(count, dim);
    break;
  }

  if ((fout != stdout) && (fout != NULL))
    fclose(fout);
  
  if (stats) {
    OutputStatistics(dim);
    fprintf(stderr, "# %d vector(s) generated\n", count);
  }
  
  return 0;
}


/* initialisiert Zählvariablen der Statistik 
 */
void
InitStatistics(int Dimensions)
{
  /* We don't explicitly free this buffers, since they are freed when
   * the program terminates
   */
  Statistics_SumX = (double *) malloc(Dimensions * sizeof(double));
  Statistics_SumXsquared = (double *) malloc(Dimensions * sizeof(double));
  Statistics_SumProduct =
    (double *) malloc(Dimensions * Dimensions * sizeof(double));

  Statistics_Count = 0;
  for (int d = 0; d < Dimensions; d++) {
    Statistics_SumX[d] = 0.0;
    Statistics_SumXsquared[d] = 0.0;
    for (int dd = 0; dd < Dimensions; dd++)
      Statistics_SumProduct[d * Dimensions + dd] = 0.0;
  }
}


/* registiriert den Vektor "x" für die Statistik 
 */
void
EnterStatistics(int Dimensions, double *x)
{
  Statistics_Count++;
  for (int d = 0; d < Dimensions; d++) {
    Statistics_SumX[d] += x[d];
    Statistics_SumXsquared[d] += x[d] * x[d];
    for (int dd = 0; dd < Dimensions; dd++)
      Statistics_SumProduct[d * Dimensions + dd] += x[d] * x[dd];
  }
}

/* gibt die Statistik aus 
 */
void
OutputStatistics(int Dimensions)
{
  for (int d = 0; d < Dimensions; d++) {
    double E = Statistics_SumX[d] / Statistics_Count;
    double V = Statistics_SumXsquared[d] / Statistics_Count - E * E;
    double s = sqrt(V);
    fprintf(flog, "# E[X%d]=%5.2f Var[X%d]=%5.2f s[X%d]=%5.2f\n", d + 1, E, d + 1, V,
           d + 1, s);
  }
  fprintf(flog, "#\n# Korrelationsmatrix:\n");
  for (int d = 0; d < Dimensions; d++) {
    fprintf(flog, "#");
    for (int dd = 0; dd < Dimensions; dd++) {
      double Kov =
        (Statistics_SumProduct[d * Dimensions + dd] / Statistics_Count) -
        (Statistics_SumX[d] / Statistics_Count) * (Statistics_SumX[dd] /
                                                   Statistics_Count);
      double Cor =
        Kov / sqrt(Statistics_SumXsquared[d] / Statistics_Count -
                   sqr(Statistics_SumX[d] / Statistics_Count)) /
        sqrt(Statistics_SumXsquared[dd] / Statistics_Count -
             sqr(Statistics_SumX[dd] / Statistics_Count));
      fprintf(flog, " %5.2f", Cor);
    }
    fprintf(flog, "\n");
  }
  fprintf(flog, "#\n");
}

double
RandomEqual(double min, double max)
{
  double x = (double) rand() / RAND_MAX;
  return x * (max - min) + min;
}


/* liefert eine Zufallsvariable im Intervall [min,max[ 
 * als Summe von "dim" gleichverteilten Zufallszahlen 
 */
double
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
double
RandomNormal(double med, double var)
{
  return RandomPeak(med - var, med + var, 12);
}

void
OutputVector(int Dimensions, double *x)
{
  while (Dimensions--)
    fprintf(fout, "%8.6f%s", *(x++), Dimensions ? " " : "\n");
}

int
IsVectorOk(int Dimensions, double *x)
{
  while (Dimensions--) {
    if (*x < 0.0 || *x > 1.0)
      return 0;
    x++;
  }

  return 1;
}

/* generiert "Count" gleichverteilte Datensätze 
 */
void
GenerateDataEqually(int Count, int Dimensions)
{
  while (Count--) {
    double x[Dimensions];
    for (int d = 0; d < Dimensions; d++)
      x[d] = RandomEqual(0, 1);

    OutputVector(Dimensions, x);
    EnterStatistics(Dimensions, x);
  }
}

/* generiert "Count" korrelierte Datensätze 
 */
void
GenerateDataCorrelated(int Count, int Dimensions)
{
  while (Count--) {
    double x[Dimensions];
    do {
      double v = RandomPeak(0, 1, Dimensions);
      for (int d = 0; d < Dimensions; d++)
        x[d] = v;
      double l = v <= 0.5 ? v : 1.0 - v;
      for (int d = 0; d < Dimensions; d++) {
        double h = RandomNormal(0, l);
        x[d] += h;
        x[(d + 1) % Dimensions] -= h;
      }
    } while (!IsVectorOk(Dimensions, x));

    OutputVector(Dimensions, x);
    EnterStatistics(Dimensions, x);
  }
}

/* generiert "Count" antikorrelierte Datensätze 
 */
void
GenerateDataAnticorrelated(int Count, int Dimensions)
{
  while (Count--) {
    double x[Dimensions];    
    do {
      double v = RandomNormal(0.5, 0.25);
      for (int d = 0; d < Dimensions; d++)
        x[d] = v;
      double l = v <= 0.5 ? v : 1.0 - v;
      for (int d = 0; d < Dimensions; d++) {
        double h = RandomEqual(-l, l);
        x[d] += h;
        x[(d + 1) % Dimensions] -= h;
      }
    } while (!IsVectorOk(Dimensions, x));

    OutputVector(Dimensions, x);
    EnterStatistics(Dimensions, x);
  }
}

static void
usage()
{
  printf(
"\
Test data generator for Skyline Operator\n\
usage: %s [OPTIONS] [FILE]\n\
\n\
Options:\n\
       -e       equally distributed\n\
       -c       correlated\n\
       -a       anti-correlated\n\
\n\
       -d DIM   dimensions >=2\n\
\n\
       -n COUNT number of vectors\n\
\n\
       -p       print dimension and number of vectors\n\
       -o FILE  output filename, use `-' for stdout\n\
\n\
       -s       output stats to stderr\n\
\n\
Examples:\n\
       %s -e -d 3 -n 10\n\
       %s -a -d 2 -n 100 -s -p -o anticorr-2d-100.txt\n\
\n\
", __progname, __progname, __progname);
}

