//#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
//#include <strings.h>
//#include <sys/time.h>
#include "distribute.h"

//#define DEBUG_TIME
#ifdef DEBUG_TIME
#include <time.h>
extern clock_t total_time;
#endif

void Distribute::init()//int n_servers, int k_servers, int alphabet_size)
{
  //this->n_servers = n_servers;
  //this->alphabet_size = alphabet_size;
  //this->k_servers = k_servers;
  rs = new ReedSolomon((n_servers - k_servers) * alphabet_size);
}

Distribute::~Distribute()
{
  delete rs;
}

void Distribute::reset(int n_servers, int k_servers, int alphabet_size)
{
  this->n_servers = n_servers;
  this->k_servers = k_servers;
  this->alphabet_size = alphabet_size;
  rs->reset_npar((n_servers - k_servers) * alphabet_size);
}

// 'encoded_length' returns the byte length of each array
unsigned char **Distribute::encode_split(unsigned char *msg, int message_length_in_bytes, int &encoded_length)
{
  //initialize_rs((n_servers - k_servers) * alphabet_size);

  int msg_unit = k_servers * alphabet_size;
  int unit_count = (message_length_in_bytes - 1) / msg_unit + 1;
  unsigned char codeword[256];
  int i,j;

  unsigned char **encoded = new unsigned char *[n_servers];
  for(i = 0; i < n_servers; i++)
  {
    encoded[i] = new unsigned char[unit_count * alphabet_size];
  }

  for(i = 0; i < unit_count; i++)
  {
    if((i < (unit_count - 1)) || ( message_length_in_bytes % msg_unit == 0))
    {
      rs->encode_data( msg + i * msg_unit, k_servers * alphabet_size, codeword);
    }
    else // padding
    {
      unsigned char padded_msg[msg_unit];
      bzero(padded_msg, msg_unit *(sizeof(unsigned char)));
      memcpy(padded_msg, msg + i * msg_unit, message_length_in_bytes - i * msg_unit);
      rs->encode_data( padded_msg, k_servers * alphabet_size, codeword);
    }

    for(j = 0; j < n_servers; j++)
    {
      memcpy(encoded[j] + i * alphabet_size, codeword + j * alphabet_size,
              alphabet_size * sizeof(unsigned char));
    }
  }

  encoded_length = unit_count * alphabet_size;
  return encoded;
}


// returns message length
unsigned char *Distribute::decode_merge(unsigned char **encoded, int encoded_length, int &message_length)
{
  message_length = encoded_length * k_servers;
  unsigned char codeword[256];
  int i,j,k;
  
  //initialize_rs((n_servers - k_servers) * alphabet_size);
  unsigned char *decoded = new unsigned char[message_length];

  for(i = 0; i < encoded_length / alphabet_size; i++)
  {
    int er[128];
    int num_er = 0;
    bzero(er, sizeof(er));
    for(j=0; j < n_servers; j++)
    {
      if(encoded[j] == NULL)
      {
        // erased
        //memset(codeword + j * alphabet_size, 0xFF, alphabet_size); // TEMP for DEBUG
        for(k =0; k<alphabet_size; k++)
          er[num_er++] = (n_servers * alphabet_size) - (j * alphabet_size + k) - 1;
      }
      else
      {
        memcpy(codeword + j * alphabet_size, encoded[j] + i * alphabet_size, alphabet_size);
      }
    }
    //print_hex("EN", codeword, n_servers * alphabet_size);
    rs->decode_data(codeword, n_servers * alphabet_size);

#ifdef DEBUG_TIME
	clock_t start=clock();
#endif
    rs->get_berlekamp()->correct_errors_erasures (codeword, n_servers * alphabet_size, num_er, er);
#ifdef DEBUG_TIME
	total_time += clock() - start;
#endif
    //print_hex("DE", codeword, n_servers * alphabet_size);
    memcpy(decoded + k_servers * alphabet_size * i, codeword, k_servers * alphabet_size);
  }

  return decoded;
}

