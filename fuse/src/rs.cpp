/* 
 * Reed Solomon Encoder/Decoder 
 *
 * Copyright Henry Minsky (hqm@alum.mit.edu) 1991-2009
 *
 * This software library is licensed under terms of the GNU GENERAL
 * PUBLIC LICENSE
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

 * Commercial licensing is available under a separate license, please
 * contact author for details.
 *
 * Source code is available at http://rscode.sourceforge.net
 */

#include "rs.h"
#include <stdio.h>

ReedSolomon::ReedSolomon(int npar)
{
  this->npar = npar;
  //printf("npar = %d  ",npar); fflush(stdout);
  pBytes = new int[npar * 2]; clear<int>(pBytes, npar * 2);
  synBytes = new int[npar * 2]; clear<int>(synBytes, npar * 2);
  genPoly = new int[npar * 4]; clear<int>(genPoly, npar * 4);

  berlekamp = new Berlekamp(npar, synBytes);
  initialize_ecc();
}

ReedSolomon::~ReedSolomon()
{
  delete[] pBytes;
  delete[] synBytes;
  delete[] genPoly;
  delete berlekamp;
}

void ReedSolomon::reset_npar(int npar)
{
  if(this->npar == npar) return;
  this->npar = npar;
  //printf("npar = %d   ",npar);
  delete[] pBytes;
  delete[] synBytes;
  delete[] genPoly;
  delete berlekamp;
  pBytes = new int[npar * 2]; clear<int>(pBytes, npar * 2);
  synBytes = new int[npar * 2]; clear<int>(synBytes, npar * 2);
  genPoly = new int[npar * 4]; clear<int>(genPoly, npar * 4);
  berlekamp = new Berlekamp(npar, synBytes);
  initialize_ecc();
}

/* Initialize lookup tables, polynomials, etc. */
void ReedSolomon::initialize_ecc()
{
  /* Initialize the galois field arithmetic tables */
	Galois::init_galois_tables();

    /* Compute the encoder generator polynomial */
    compute_genpoly(npar, genPoly);
}

void ReedSolomon::zero_fill_from (unsigned char buf[], int from, int to)
{
  int i;
  for (i = from; i < to; i++) buf[i] = 0;
}

/* Append the parity bytes onto the end of the message */
void ReedSolomon::build_codeword (unsigned char msg[], int nbytes, unsigned char dst[])
{
  int i;
	
  for (i = 0; i < nbytes; i++) dst[i] = msg[i];
	
  for (i = 0; i < npar; i++) {
    dst[i+nbytes] = pBytes[npar-1-i];
  }
}
	
/**********************************************************
 * Reed Solomon Decoder 
 *
 * Computes the syndrome of a codeword. Puts the results
 * into the synBytes[] array.
 */
 
void ReedSolomon::decode_data(unsigned char data[], int nbytes)
{
  int i, j, sum;
  for (j = 0; j < npar;  j++) {
    sum	= 0;
    for (i = 0; i < nbytes; i++) {
      sum = data[i] ^ Galois::gmult(Galois::gexp(j+1), sum);
    }
    synBytes[j]  = sum;
  }
}


/* Check if the syndrome is zero */
int ReedSolomon::check_syndrome (void)
{
 int i, nz = 0;
 for (i =0 ; i < npar; i++) {
  if (synBytes[i] != 0) {
      nz = 1;
      break;
  }
 }
 return nz;
}



/* Create a generator polynomial for an n byte RS code. 
 * The coefficients are returned in the genPoly arg.
 * Make sure that the genPoly array which is passed in is 
 * at least n+1 bytes long.
 */

void ReedSolomon::compute_genpoly (int nbytes, int genpoly[])
{
  int i, tp[256], tp1[256];
	
  /* multiply (x + a^n) for n = 1 to nbytes */

  berlekamp->zero_poly(tp1);
  tp1[0] = 1;

  for (i = 1; i <= nbytes; i++) {
    berlekamp->zero_poly(tp);
    tp[0] = Galois::gexp(i);		/* set up x+a^n */
    tp[1] = 1;
	  
    berlekamp->mult_polys(genpoly, tp, tp1);
    berlekamp->copy_poly(tp1, genpoly);
  }
}

/* Simulate a LFSR with generator polynomial for n byte RS code. 
 * Pass in a pointer to the data array, and amount of data. 
 *
 * The parity bytes are deposited into pBytes[], and the whole message
 * and parity are copied to dest to make a codeword.
 * 
 */

void ReedSolomon::encode_data (unsigned char msg[], int nbytes, unsigned char dst[])
{
  int i, LFSR[npar+1],dbyte, j;
	
  for(i=0; i < npar+1; i++) LFSR[i]=0;

  for (i = 0; i < nbytes; i++) {
    dbyte = msg[i] ^ LFSR[npar-1];
    for (j = npar-1; j > 0; j--) {
      LFSR[j] = LFSR[j-1] ^ Galois::gmult(genPoly[j], dbyte);
    }
    LFSR[0] = Galois::gmult(genPoly[0], dbyte);
  }

  for (i = 0; i < npar; i++) 
    pBytes[i] = LFSR[i];
	
  build_codeword(msg, nbytes, dst);
}

