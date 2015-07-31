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

#ifndef BERLEKAMP_H
#define BERLEKAMP_H
#include "galois.h"

class Berlekamp
{
	/* The Error Locator Polynomianit_exp_table();
	    46 }
	    l, also known as Lambda or Sigma. Lambda[0] == 1 */
	int *Lambda;

	/* The Error Evaluator Polynomial */
	int *Omega;

	/* error locations found using Chien's search*/
	int ErrorLocs[256];
	int NErrors;

	/* erasure flags */
	int ErasureLocs[256];
	int NErasures;

	int maxdeg;
	int npar;
	int *synbytes;

  public:
	Berlekamp(int npar, int *synbytes);
	~Berlekamp();

  private:

	/* local ANSI declarations */
	int compute_discrepancy(int lambda[], int S[], int L, int n);
	void init_gamma(int gamma[]);
	void compute_modified_omega (void);


	/* From  Cain, Clark, "Error-Correction Coding For Digital Communications", pp. 216. */
	void Modified_Berlekamp_Massey (void);


	/* given Psi (called Lambda in Modified_Berlekamp_Massey) and synBytes,
	   compute the combined erasure/error evaluator polynomial as 
	   Psi*S mod z^4
	  */

  public:	
	void zero_poly (int poly[]);
	
	/* polynomial multiplication */
	void mult_polys (int dst[], int p1[], int p2[]);

	void copy_poly (int dst[], int src[]);

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
	
	int correct_errors_erasures (unsigned char codeword[], 
				 int csize, int nerasures, int erasures[]);

  private:
	void compute_next_omega (int d, int A[], int dst[], int src[]);

	/********** polynomial arithmetic *******************/

	void add_polys (int dst[], int src[]) 
	{
	  int i;
	  for (i = 0; i < maxdeg; i++) dst[i] ^= src[i];
	}
	
	void scale_poly (int k, int poly[]) 
	{	
	  int i;
	  for (i = 0; i < maxdeg; i++) poly[i] = Galois::gmult(k, poly[i]);
	}
	
	/* multiply by z, i.e., shift right by 1 */
	void mul_z_poly (int src[])
	{
	  int i;
	  for (i = maxdeg-1; i > 0; i--) src[i] = src[i-1];
	  src[0] = 0;
	}
	
	
	/* Finds all the roots of an error-locator polynomial with coefficients
	 * Lambda[j] by evaluating Lambda at successive values of alpha. 
	 * 
	 * This can be tested with the decoder's equations case.
	 */
	
	
	void Find_Roots (void);
		
};

#endif
