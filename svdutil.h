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

#ifndef SVDUTIL_H
#define SVDUTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include "svdlib.h"

#define SAFE_FREE(a) {if (a) {free(a); a = NULL;}}

/* Allocates an array of longs. */
extern long *svd_longArray(long size, char empty, char *name);
/* Allocates an array of doubles. */
extern double *svd_doubleArray(long size, char empty, char *name);

extern void svd_debug(const char *fmt, ...);
extern void svd_error(const char *fmt, ...);
extern void svd_fatalError(const char *fmt, ...);
extern FILE *svd_fatalReadFile(const char *filename);
extern FILE *svd_readFile(const char *fileName);
extern FILE *svd_writeFile(const char *fileName, char append);
extern void svd_closeFile(FILE *file);

extern char svd_readBinInt(FILE *file, int *val);
extern char svd_readBinFloat(FILE *file, float *val);
extern char svd_writeBinInt(FILE *file, int x);
extern char svd_writeBinFloat(FILE *file, float r);

/************************************************************** 
 * returns |a| if b is positive; else fsign returns -|a|      *
 **************************************************************/ 
static inline double svd_fsign(double a, double b) {
  if ((a>=0.0 && b>=0.0) || (a<0.0 && b<0.0))return(a);
  else return -a;
}

/************************************************************** 
 * returns the larger of two double precision numbers         *
 **************************************************************/ 
static inline double svd_dmax(double a, double b) {
   return (a > b) ? a : b;
}

/************************************************************** 
 * returns the smaller of two double precision numbers        *
 **************************************************************/ 
static inline double svd_dmin(double a, double b) {
  return (a < b) ? a : b;
}

/************************************************************** 
 * returns the larger of two integers                         *
 **************************************************************/ 
static inline long svd_imax(long a, long b) {
  return (a > b) ? a : b;
}

/************************************************************** 
 * returns the smaller of two integers                        *
 **************************************************************/ 
static inline long svd_imin(long a, long b) {
  return (a < b) ? a : b;
}

/************************************************************** 
 * Function scales a vector by a constant.     		      *
 * Based on Fortran-77 routine from Linpack by J. Dongarra    *
 **************************************************************/ 
#define svd_dscal cblas_dscal
extern void svd_dscal(long n, double da, double *dx, long incx);

/************************************************************** 
 * function scales a vector by a constant.	     	      *
 * Based on Fortran-77 routine from Linpack by J. Dongarra    *
 **************************************************************/ 
extern void svd_datx(long n, double da, double *dx, long incx, double *dy, long incy);

/************************************************************** 
 * Function copies a vector x to a vector y	     	      *
 * Based on Fortran-77 routine from Linpack by J. Dongarra    *
 **************************************************************/ 
#define svd_dcopy cblas_dcopy
extern void svd_dcopy(long n, double *dx, long incx, double *dy, long incy);

/************************************************************** 
 * Function forms the dot product of two vectors.      	      *
 * Based on Fortran-77 routine from Linpack by J. Dongarra    *
 **************************************************************/ 
#define svd_ddot cblas_ddot
extern double svd_ddot(long n, double *dx, long incx, double *dy, long incy);

/************************************************************** 
 * Constant times a vector plus a vector     		      *
 * Based on Fortran-77 routine from Linpack by J. Dongarra    *
 **************************************************************/ 
#define svd_daxpy cblas_daxpy
extern void svd_daxpy (long n, double da, double *dx, long incx, double *dy, long incy);

/********************************************************************* 
 * Function sorts array1 and array2 into increasing order for array1 *
 *********************************************************************/
extern void svd_dsort2(long igap, long n, double *array1, double *array2);

/************************************************************** 
 * Function interchanges two vectors		     	      *
 * Based on Fortran-77 routine from Linpack by J. Dongarra    *
 **************************************************************/ 
#define svd_dswap cblas_dswap
extern void svd_dswap(long n, double *dx, long incx, double *dy, long incy);

/***************************************************************** 
 * Function finds the index of element having max. absolute value*
 * based on FORTRAN 77 routine from Linpack by J. Dongarra       *
 *****************************************************************/ 
#define svd_idamax cblas_idamax
extern long svd_idamax(long n, double *dx, long incx);

/**************************************************************
 * multiplication of matrix B by vector x, where B = A'A,     *
 * and A is nrow by ncol (nrow >> ncol). Hence, B is of order *
 * n = ncol (y stores product vector).		              *
 **************************************************************/
extern void svd_opb(SMat A, double *x, double *y, double *temp);

/***********************************************************
 * multiplication of matrix A by vector x, where A is 	   *
 * nrow by ncol (nrow >> ncol).  y stores product vector.  *
 ***********************************************************/
extern void svd_opa(SMat A, double *x, double *y);

/***********************************************************************
 *                                                                     *
 *				random2()                              *
 *                        (double precision)                           *
 ***********************************************************************/
extern double svd_random2(long *iy);

/************************************************************** 
 *							      *
 * Function finds sqrt(a^2 + b^2) without overflow or         *
 * destructive underflow.				      *
 *							      *
 **************************************************************/ 
static inline double svd_pythag(double a, double b) {
    //
    // http://blogs.mathworks.com/cleve/2012/07/30/pythagorean-addition/
    // use of sqrt() should be safe and fast for Intel CPUs
    //
    if(a==0 && b==0) {
        return 0;
    }
    a = fabs(a);
    b = fabs(b);
    if(a > b) {
        b /= a;
        b *= b;
        return a * sqrt(b+1);
    } else {
        a /= b;
        a *= a;
        return b * sqrt(a+1);
    }
}

#ifdef __cplusplus
}
#endif

#endif /* SVDUTIL_H */
