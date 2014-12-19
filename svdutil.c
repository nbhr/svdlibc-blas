/*
Copyright © 2002, University of Tennessee Research Foundation.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

  Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the University of Tennessee nor the names of its
  contributors may be used to endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include "svdlib.h"
#include "svdutil.h"

#define BUNZIP2  "bzip2 -d"
#define BZIP2    "bzip2 -1"
#define UNZIP    "gzip -d"
#define ZIP      "gzip -1"
#define COMPRESS "compress"

#define MAX_FILENAME 512
#define MAX_PIPES    64
static FILE *Pipe[MAX_PIPES];
static int numPipes = 0;

long *svd_longArray(long size, char empty, char *name) {
  long *a;
  if (empty) a = (long *) calloc(size, sizeof(long));
  else a = (long *) malloc(size * sizeof(long));
  if (!a) {
    perror(name);
    /* exit(errno); */
  }
  return a;
}

double *svd_doubleArray(long size, char empty, char *name) {
  double *a;
  if (empty) a = (double *) calloc(size, sizeof(double));
  else a = (double *) malloc(size * sizeof(double));
  if (!a) {
    perror(name);
    /* exit(errno); */
  }
  return a;
}

void svd_beep(void) {
  fputc('\a', stderr);
  fflush(stderr);
}

void svd_debug(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}

void svd_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  svd_beep();
  fprintf(stderr, "ERROR: ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
}

void svd_fatalError(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  svd_beep();
  fprintf(stderr, "ERROR: ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\a\n");
  va_end(args);
  exit(1);
}

static void registerPipe(FILE *p) {
  if (numPipes >= MAX_PIPES) svd_error("Too many pipes open");
  Pipe[numPipes++] = p;
}

static char isPipe(FILE *p) {
  int i;
  for (i = 0; i < numPipes && Pipe[i] != p; i++);
  if (i == numPipes) return FALSE;
  Pipe[i] = Pipe[--numPipes];
  return TRUE;
}

static FILE *openPipe(const char *pipeName, const char *mode) {
  FILE *pipe;
  fflush(stdout);
  if ((pipe = popen(pipeName, mode))) registerPipe(pipe);
  return pipe;
}

static FILE *readZippedFile(const char *command, const char *fileName) {
  char buf[MAX_FILENAME];
  sprintf(buf, "%s < %s 2>/dev/null", command, fileName);
  return openPipe(buf, "r");
}

FILE *svd_fatalReadFile(const char *filename) {
  FILE *file;
  if (!(file = svd_readFile(filename)))
    svd_fatalError("couldn't read the file %s", filename);
  return file;
}

static int stringEndsIn(const char *s, const char *t) {
  int ls = strlen(s);
  int lt = strlen(t);
  if (ls < lt) return FALSE;
  return (strcmp(s + ls - lt, t)) ? FALSE : TRUE;
}

/* Will silently return NULL if file couldn't be opened */
FILE *svd_readFile(const char *fileName) {
  char fileBuf[MAX_FILENAME];
  struct stat statbuf;

  /* Special file name */
  if (!strcmp(fileName, "-"))
    return stdin;
  
  /* If it is a pipe */
  if (fileName[0] == '|')
    return openPipe(fileName + 1, "r");

  /* Check if already ends in .gz or .Z and assume compressed */
  if (stringEndsIn(fileName, ".gz") || stringEndsIn(fileName, ".Z")) {
    if (!stat(fileName, &statbuf))
      return readZippedFile(UNZIP, fileName);
    return NULL;
  }
  /* Check if already ends in .bz or .bz2 and assume compressed */
  if (stringEndsIn(fileName, ".bz") || stringEndsIn(fileName, ".bz2")) {
    if (!stat(fileName, &statbuf))
      return readZippedFile(BUNZIP2, fileName);
    return NULL;
  }
  /* Try just opening normally */
  if (!stat(fileName, &statbuf))
    return fopen(fileName, "r");
  /* Try adding .gz */
  sprintf(fileBuf, "%s.gz", fileName);
  if (!stat(fileBuf, &statbuf))
    return readZippedFile(UNZIP, fileBuf);
  /* Try adding .Z */
  sprintf(fileBuf, "%s.Z", fileName);
  if (!stat(fileBuf, &statbuf))
    return readZippedFile(UNZIP, fileBuf);
  /* Try adding .bz2 */
  sprintf(fileBuf, "%s.bz2", fileName);
  if (!stat(fileBuf, &statbuf))
    return readZippedFile(BUNZIP2, fileBuf);
  /* Try adding .bz */
  sprintf(fileBuf, "%s.bz", fileName);
  if (!stat(fileBuf, &statbuf))
    return readZippedFile(BUNZIP2, fileBuf);

  return NULL;
}

static FILE *writeZippedFile(const char *fileName, char append) {
  char buf[MAX_FILENAME];
  const char *op = (append) ? ">>" : ">";
  if (stringEndsIn(fileName, ".bz2") || stringEndsIn(fileName, ".bz"))
    sprintf(buf, "%s %s \"%s\"", BZIP2, op, fileName);
  else if (stringEndsIn(fileName, ".Z"))
    sprintf(buf, "%s %s \"%s\"", COMPRESS, op, fileName);
  else
    sprintf(buf, "%s %s \"%s\"", ZIP, op, fileName);
  return openPipe(buf, "w");
}

FILE *svd_writeFile(const char *fileName, char append) {
  /* Special file name */
  if (!strcmp(fileName, "-"))
    return stdout;
  
  /* If it is a pipe */
  if (fileName[0] == '|')
    return openPipe(fileName + 1, "w");

  /* Check if ends in .gz, .Z, .bz, .bz2 */
  if (stringEndsIn(fileName, ".gz") || stringEndsIn(fileName, ".Z") ||
      stringEndsIn(fileName, ".bz") || stringEndsIn(fileName, ".bz2"))
    return writeZippedFile(fileName, append);
  return (append) ? fopen(fileName, "a") : fopen(fileName, "w");
}

/* Could be a file or a stream. */
void svd_closeFile(FILE *file) {
  if (file == stdin || file == stdout) return;
  if (isPipe(file)) pclose(file);
  else fclose(file);
}


char svd_readBinInt(FILE *file, int *val) {
  int x;
  if (fread(&x, sizeof(int), 1, file) == 1) {
    *val = ntohl(x);
    return FALSE;
  }
  return TRUE;
}

/* This reads a float in network order and converts to a real in host order. */
char svd_readBinFloat(FILE *file, float *val) {
  int x;
  float y;
  char * z;
  if (fread(&x, sizeof(int), 1, file) == 1) {
    x = ntohl(x);
    z = (char*)&x;
    y = *((float *)z);
    *val = y;
    return FALSE;
  }
  return TRUE;
}

char svd_writeBinInt(FILE *file, int x) {
  int y = htonl(x);
  if (fwrite(&y, sizeof(int), 1, file) != 1) return TRUE;
  return FALSE;
}

/* This takes a real in host order and writes a float in network order. */
char svd_writeBinFloat(FILE *file, float r) {
  char * z = (char*)&r;
  int y = htonl(*((int *) z));
  if (fwrite(&y, sizeof(int), 1, file) != 1) return TRUE;
  return FALSE;
}

/************************************************************** 
 * function scales a vector by a constant.	     	      *
 * Based on Fortran-77 routine from Linpack by J. Dongarra    *
 **************************************************************/ 
void svd_datx(long n, double da, double *dx, long incx, double *dy, long incy) {
  long i;
  
  if (n <= 0 || incx == 0 || incy == 0 || da == 0.0) return;
  if (incx == 1 && incy == 1) 
    for (i=0; i < n; i++) *dy++ = da * (*dx++);
  
  else {
    if (incx < 0) dx += (-n+1) * incx;
    if (incy < 0) dy += (-n+1) * incy;
    for (i=0; i < n; i++) {
      *dy = da * (*dx);
      dx += incx;
      dy += incy;
    }
  }
  return;
}

/********************************************************************* 
 * Function sorts array1 and array2 into increasing order for array1 *
 *********************************************************************/
void svd_dsort2(long igap, long n, double *array1, double *array2) {
  double temp;
  long i, j, index;
  
  if (!igap) return;
  else {
    for (i = igap; i < n; i++) {
      j = i - igap;
      index = i;
      while (j >= 0 && array1[j] > array1[index]) {
        temp = array1[j];
        array1[j] = array1[index];
        array1[index] = temp;
        temp = array2[j];
        array2[j] = array2[index];
        array2[index] = temp;
        j -= igap;
        index = j + igap;
      }
    } 
  }
  svd_dsort2(igap/2,n,array1,array2);
}

/**************************************************************
 * multiplication of matrix B by vector x, where B = A'A,     *
 * and A is nrow by ncol (nrow >> ncol). Hence, B is of order *
 * n = ncol (y stores product vector).		              *
 **************************************************************/
void svd_opb(SMat A, double *x, double *y, double *temp) {
  long i, j, end;
  long *pointr = A->pointr, *rowind = A->rowind;
  double *value = A->value;
  long n = A->cols;

  SVDCount[SVD_MXV] += 2;
  memset(y, 0, n * sizeof(double));
  memset(temp, 0, (A->rows) * sizeof(double));
  //for (i = 0; i < A->rows; i++) temp[i] = 0.0;

  for (i = 0; i < A->cols; i++) {
    end = pointr[i+1];
    for (j = pointr[i]; j < end; j++) 
      temp[rowind[j]] += value[j] * (*x); 
    x++;
  }

  for (i = 0; i < A->cols; i++) {
    end = pointr[i+1];
    for (j = pointr[i]; j < end; j++) 
      *y += value[j] * temp[rowind[j]];
    y++;
  }

  return;
}

/***********************************************************
 * multiplication of matrix A by vector x, where A is 	   *
 * nrow by ncol (nrow >> ncol).  y stores product vector.  *
 ***********************************************************/
void svd_opa(SMat A, double *x, double *y) {
  long end, i, j;
  long *pointr = A->pointr, *rowind = A->rowind;
  double *value = A->value;
   
  SVDCount[SVD_MXV]++;
  memset(y, 0, A->rows * sizeof(double));
  //for(i=0 ; i<A->rows ; i++) y[i]=0;
  
  for (i = 0; i < A->cols; i++) {
    end = pointr[i+1];
    for (j = pointr[i]; j < end; j++)
      y[rowind[j]] += value[j] * x[i]; 
  }
  return;
}


/***********************************************************************
 *                                                                     *
 *				random()                               *
 *                        (double precision)                           *
 ***********************************************************************/
/***********************************************************************

   Description
   -----------

   This is a translation of a Fortran-77 uniform random number
   generator.  The code is based  on  theory and suggestions  given in
   D. E. Knuth (1969),  vol  2.  The argument to the function should 
   be initialized to an arbitrary integer prior to the first call to 
   random.  The calling program should  not  alter  the value of the
   argument between subsequent calls to random.  Random returns values
   within the interval (0,1).


   Arguments 
   ---------

   (input)
   iy	   an integer seed whose value must not be altered by the caller
	   between subsequent calls

   (output)
   random  a double precision random number between (0,1)

 ***********************************************************************/
double svd_random2(long *iy) {
   static long m2 = 0;
   static long ia, ic, mic;
   static double halfm, s;

   /* If first entry, compute (max int) / 2 */
   if (!m2) {
      m2 = 1 << (8 * (int)sizeof(int) - 2); 
      halfm = m2;

      /* compute multiplier and increment for linear congruential 
       * method */
      ia = 8 * (long)(halfm * atan(1.0) / 8.0) + 5;
      ic = 2 * (long)(halfm * (0.5 - sqrt(3.0)/6.0)) + 1;
      mic = (m2-ic) + m2;

      /* s is the scale factor for converting to floating point */
      s = 0.5 / halfm;
   }

   /* compute next random number */
   *iy = *iy * ia;

   /* for computers which do not allow integer overflow on addition */
   if (*iy > mic) *iy = (*iy - m2) - m2;

   *iy = *iy + ic;

   /* for computers whose word length for addition is greater than
    * for multiplication */
   if (*iy / 2 > m2) *iy = (*iy - m2) - m2;
  
   /* for computers whose integer overflow affects the sign bit */
   if (*iy < 0) *iy = (*iy + m2) + m2;

   return((double)(*iy) * s);
}

