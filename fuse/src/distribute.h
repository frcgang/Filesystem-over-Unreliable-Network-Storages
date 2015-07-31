#ifndef DISTRIBUTE_H
#define DISTRIBUTE_H

#include "rs.h"

class Distribute
{
	ReedSolomon *rs;
	int k_servers;

	void init();

  public:
	int n_servers;
	int alphabet_size;
	
	/**
	  @param n_servers The number of stroages; the block length n
	  @param k_servers The number of minimum storages to decode messages; the message lengath k
	  @param alphabet_size RS code unit size in bytes
	 */
	Distribute(int n_servers, int k_servers, int alphabet_size = 1)
		//: n_servers(n_servers), k_servers(k_servers), alphabet_size(alphabet_size)
	{
		this->n_servers = n_servers;
		this->k_servers = k_servers;
		this->alphabet_size = alphabet_size;
		init();
	}

	Distribute(int n_servers)
		//: n_servers(n_servers), k_servers(n_servers / 2 + 1), alphabet_size(1)
	{
		this->n_servers = n_servers;
		this->k_servers = n_servers / 2 + 1;
		this->alphabet_size = 1;
		init();
	}

	~Distribute();

	/** Change the value of basic parameters
	 */
	void reset(int n_servers, int k_servers, int alphabet_size = 1);

	/**
	  @param msg The file content to encode
	  @param message_length_in_bytes The lenght of 'msg' in bytes
	  @param encoded_length The return value of the byte length of  each encoded message array
	  @return The array of encoded messages
	 */
	unsigned char **encode_split(unsigned char *msg, int message_length_in_bytes, int &encoded_length);

	/**
	  @param encoded The encoded message array
	  @param encoded_length The byte lenth of an encoded message
	  @param The return value of the decoded message
	  @return The decoded message
	 */
	unsigned char *decode_merge(unsigned char **encoded, int encoded_length, int &message_length);

	/** deallocate memory
	 */
	void deleteEncoded(unsigned char **encoded)
	{
	  for(int i = 0; i < n_servers; i++)
		  delete[] encoded[i];
	  delete[] encoded;
	}
};

#endif
