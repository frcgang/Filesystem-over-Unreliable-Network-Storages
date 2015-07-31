// modified by gang

/***********************************************************************
 * Copyright Henry Minsky (hqm@alum.mit.edu) 1991-2009
 *
 * This software library is licensed under terms of the GNU GENERAL
 * PUBLIC LICENSE
 * 
 *
 * RSCODE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RSCODE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Rscode.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Commercial licensing is available under a separate license, please
 * contact author for details.
 *
 * Source code is available at http://rscode.sourceforge.net
 * Berlekamp-Peterson and Berlekamp-Massey Algorithms for error-location
 *
 * From Cain, Clark, "Error-Correction Coding For Digital Communications", pp. 205.
 *
 * This finds the coefficients of the error locator polynomial.
 *
 * The roots are then found by looking for the values of a^n
 * where evaluating the polynomial yields zero.
 *
 * Error correction is done using the error-evaluator equation  on pp 207.
 *
 */

#include <stdio.h>
//#include <stdlib.h>

#include "berlekamp.h"

Berlekamp::Berlekamp(int npar, int *synbytes)
{
  this->maxdeg = npar * 2;
  this->npar = npar;
  this->synbytes = synbytes;
  Lambda = new int[maxdeg]; clear<int>(Lambda,maxdeg);
  Omega = new int[maxdeg]; clear<int>(Omega, maxdeg);

  NErasures = 0;
  NErrors = 0;
}

Berlekamp::~Berlekamp()
{
   delete[] Lambda;
   delete[] Omega;
}

/* From  Cain, Clark, "Error-Correction Coding For Digital Communications", pp. 216. */
void Berlekamp::Modified_Berlekamp_Massey (void)
{	
  int n, L, L2, k, d, i;
  int psi[maxdeg], psi2[maxdeg], D[maxdeg];
  int gamma[maxdeg];
	
  /* initialize Gamma, the erasure locator polynomial */
  init_gamma(gamma);

  /* initialize to z */
  copy_poly(D, gamma);
  mul_z_poly(D);
	
  copy_poly(psi, gamma);	
  k = -1; L = NErasures;
	
  for (n = NErasures; n < npar; n++) {
	
    d = compute_discrepancy(psi, synbytes, L, n);
		
    if (d != 0) {
		
      /* psi2 = psi - d*D */
      for (i = 0; i < maxdeg; i++) psi2[i] = psi[i] ^ Galois::gmult(d, D[i]);
		
		
      if (L < (n-k)) {
	L2 = n-k;
	k = n-L;
	/* D = scale_poly(ginv(d), psi); */
	for (i = 0; i < maxdeg; i++) D[i] = Galois::gmult(psi[i], Galois::ginv(d));
	L = L2;
      }
			
      /* psi = psi2 */
      for (i = 0; i < maxdeg; i++) psi[i] = psi2[i];
    }
		
    mul_z_poly(D);
  }
	
  //for(i = 0; i < maxdeg; i++){printf(" %d %d, %d\n",maxdeg,i,psi[i]); Lambda[i] = psi[i];}
  for(i = 0; i < maxdeg; i++)
	  Lambda[i] = psi[i];
  compute_modified_omega();

	
}

/* given Psi (called Lambda in Modified_Berlekamp_Massey) and synBytes,
   compute the combined erasure/error evaluator polynomial as 
   Psi*S mod z^4
  */
void Berlekamp::compute_modified_omega ()
{
  int i;
  int product[maxdeg*2];
	
  mult_polys(product, Lambda, synbytes);	
  zero_poly(Omega);
  for(i = 0; i < npar; i++) Omega[i] = product[i];

}

/* polynomial multiplication */
void Berlekamp::mult_polys (int dst[], int p1[], int p2[])
{
  int i, j;
  int tmp1[maxdeg*2];
	
  for (i=0; i < (maxdeg*2); i++) dst[i] = 0;
	
  for (i = 0; i < maxdeg; i++) {
    for(j=maxdeg; j<(maxdeg*2); j++) tmp1[j]=0;
		
    /* scale tmp1 by p1[i] */
    for(j=0; j<maxdeg; j++) tmp1[j]=Galois::gmult(p2[j], p1[i]);
    /* and mult (shift) tmp1 right by i */
    for (j = (maxdeg*2)-1; j >= i; j--) tmp1[j] = tmp1[j-i];
    for (j = 0; j < i; j++) tmp1[j] = 0;
		
    /* add into partial product */
    for(j=0; j < (maxdeg*2); j++) dst[j] ^= tmp1[j];
  }
}

void Berlekamp::zero_poly (int poly[])
{
  int i;
  for (i = 0; i < maxdeg; i++) poly[i] = 0;
}

void Berlekamp::copy_poly (int dst[], int src[])
{
  int i;
  for (i = 0; i < maxdeg; i++) dst[i] = src[i];
}

/* gamma = product (1-z*a^Ij) for erasure locs Ij */
void Berlekamp::init_gamma (int gamma[])
{
  int e, tmp[maxdeg];
	
  zero_poly(gamma);
  zero_poly(tmp);
  gamma[0] = 1;
	
  for (e = 0; e < NErasures; e++) {
    copy_poly(tmp, gamma);
    scale_poly(Galois::gexp(ErasureLocs[e]), tmp);
    mul_z_poly(tmp);
    add_polys(gamma, tmp);
  }
}
	
	
	
void  Berlekamp::compute_next_omega (int d, int A[], int dst[], int src[])
{
  int i;
  for ( i = 0; i < maxdeg;  i++) {
    dst[i] = src[i] ^ Galois::gmult(d, A[i]);
  }
}
	


int Berlekamp::compute_discrepancy (int lambda[], int S[], int L, int n)
{
  int i, sum=0;
	
  for (i = 0; i <= L; i++) 
    sum ^= Galois::gmult(lambda[i], S[n-i]);
  return (sum);
}

/* Finds all the roots of an error-locator polynomial with coefficients
 * Lambda[j] by evaluating Lambda at successive values of alpha. 
 * 
 * This can be tested with the decoder's equations case.
 */


void  Berlekamp::Find_Roots (void)
{
  int sum, r, k;	
  NErrors = 0;
  
  for (r = 1; r < 256; r++) {
    sum = 0;
    /* evaluate lambda at r */
    for (k = 0; k < npar+1; k++) {
      sum ^= Galois::gmult(Galois::gexp((k*r)%255), Lambda[k]);
    }
    if (sum == 0) 
      { 
	ErrorLocs[NErrors] = (255-r); NErrors++; 
#ifdef RS_DEBUG
	fprintf(stderr, "Root found at r = %d, (255-r) = %d\n", r, (255-r));
#endif
      }
  }
}

/* Combined Erasure And Error Magnitude Computation 
 * 
 * Pass in the codeword, its size in bytes, as well as
 * an array of any known erasure locations, along the number
 * of these erasures.
 * 
 * Evaluate Omega(actually Psi)/Lambda' at the roots
 * alpha^(-i) for error locs i. 
 *
 * Returns 1 if everything ok, or 0 if an out-of-bounds error is found
 *
 */

int Berlekamp::correct_errors_erasures (unsigned char codeword[], 
			 int csize,
			 int nerasures,
			 int erasures[])
{
  int r, i, j, err;

  /* If you want to take advantage of erasure correction, be sure to
     set NErasures and ErasureLocs[] with the locations of erasures. 
     */
  NErasures = nerasures;
  for (i = 0; i < NErasures; i++) ErasureLocs[i] = erasures[i];

  Modified_Berlekamp_Massey();
  Find_Roots();
  

  if ((NErrors <= npar) && NErrors > 0) { 

    /* first check for illegal error locs */
    for (r = 0; r < NErrors; r++) {
      if (ErrorLocs[r] >= csize) {
#ifdef RS_DEBUG
	  fprintf(stderr, "Error loc i=%d outside of codeword length %d\n", i, csize);
#endif
	return(0);
      }
    }

    for (r = 0; r < NErrors; r++) {
      int num, denom;
      i = ErrorLocs[r];
      /* evaluate Omega at alpha^(-i) */

      num = 0;
      for (j = 0; j < maxdeg; j++) 
	num ^= Galois::gmult(Omega[j], Galois::gexp(((255-i)*j)%255));
      
      /* evaluate Lambda' (derivative) at alpha^(-i) ; all odd powers disappear */
      denom = 0;
      for (j = 1; j < maxdeg; j += 2) {
	denom ^= Galois::gmult(Lambda[j], Galois::gexp(((255-i)*(j-1)) % 255));
      }
      
      err = Galois::gmult(num, Galois::ginv(denom));
#ifdef RS_DEBUG
      fprintf(stderr, "Error magnitude %#x at loc %d\n", err, csize-i);
#endif
      
      codeword[csize-i-1] ^= err;
    }
    return(1);
  }
  else {
#ifdef RS_DEBUG
    if (NErrors) fprintf(stderr, "Uncorrectable codeword\n");
#endif
    return(0);
  }
}



