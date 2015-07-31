// modified by gang

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

#ifndef RS_H
#define RS_H

#include "berlekamp.h"

class ReedSolomon
{
	int npar;

	/* Encoder parity bytes */
	int *pBytes;
	
	/* Decoder syndrome bytes */
	int *synBytes;
	
	/* generator polynomial */
	int *genPoly;
	
	
	Berlekamp *berlekamp;

  public:
	ReedSolomon(int npar);
	~ReedSolomon();

	int num_parity_server() { return npar; }
	Berlekamp *get_berlekamp() { return berlekamp; }
	void reset_npar(int npar);
  private:
	/* Initialize lookup tables, polynomials, etc. */
	void initialize_ecc();
	
	void zero_fill_from (unsigned char buf[], int from, int to);
	
	/* debugging routines */
	/*
	void print_parity (void)
	{ 
	  int i;
	  printf("Parity Bytes: ");
	  for (i = 0; i < npar; i++) 
	    printf("[%d]:%x, ",i,pBytes[i]);
	  printf("\n");
	}
	
	
	void print_syndrome (void)
	{ 
	  int i;
	  printf("Syndrome Bytes: ");
	  for (i = 0; i < npar; i++) 
	    printf("[%d]:%x, ",i,synBytes[i]);
	  printf("\n");
	}
	*/
	
	/* Append the parity bytes onto the end of the message */
	void build_codeword (unsigned char msg[], int nbytes, unsigned char dst[]);
	
	//void debug_check_syndrome (void);
	
	/* Create a generator polynomial for an n byte RS code. 
	 * The coefficients are returned in the genPoly arg.
	 * Make sure that the genPoly array which is passed in is 
	 * at least n+1 bytes long.
	 */
	
	void compute_genpoly (int nbytes, int genpoly[]);
	
			
  public:	 
	/* Check if the syndrome is zero */
	int check_syndrome (void);
		
	/**********************************************************
	 * Reed Solomon Decoder 
	 *
	 * Computes the syndrome of a codeword. Puts the results
	 * into the synBytes[] array.
	 */
	void decode_data(unsigned char data[], int nbytes);
	
	/* Simulate a LFSR with generator polynomial for n byte RS code. 
	 * Pass in a pointer to the data array, and amount of data. 
	 *
	 * The parity bytes are deposited into pBytes[], and the whole message
	 * and parity are copied to dest to make a codeword.
	 * 
	 */
	
	void encode_data (unsigned char msg[], int nbytes, unsigned char dst[]);

};

#endif
