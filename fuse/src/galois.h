//modified by gang

/*****************************
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
 * 
 *
 * Multiplication and Arithmetic on Galois Field GF(256)
 *
 * From Mee, Daniel, "Magnetic Recording, Volume III", Ch. 5 by Patel.
 * 
 *
 ******************************/
 
#ifndef GALOIS_H
#define GALOIS_H

/* This is one of 14 irreducible polynomials
 * of degree 8 and cycle length 255. (Ch 5, pp. 275, Magnetic Recording)
 * The high order 1 bit is implicit */
/* x^8 + x^4 + x^3 + x^2 + 1 */
//#define PPOLY 0x1D 

class Galois
{
  private:
	static const int exp_size;
	static const int log_size;

	static int exp[];
	static int log[];
	
	//static void init_exp_table ();
	
  public:
	static void init_galois_tables () { }
	//static void init_galois_tables () { init_exp_table(); }

	static int gexp(int index)
	{
		if((index >= 0) && (index < exp_size)) return exp[index]; 
		else throw "range error in gexp";
	}

	static int glog(int index)
	{
		if((index >= 0) && (index < 256)) return log[index]; 
		else throw "range error in glog";
	}
	
	/* multiplication using logarithms */
	static int gmult(int a, int b)
	{
	  int i,j;
	  if (a==0 || b == 0) return (0);
	  i = log[a];
	  j = log[b];
	  return (exp[i+j]);
	}
			
	
	static int ginv (int elt) 
	{ 
	  return (exp[255-log[elt]]);
	}
};

template<class T> void clear(T a[], int size)
{
	for(int i=0; i<size; i++)
		a[i] = 0;
}

#endif
