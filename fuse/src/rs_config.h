#ifndef RS_CONFIG
#define RS_CONFIG

class RsServerInfo
{
  private:
	int capacity;
	int start_block_index;

  public:
	void set(int index, int capacity)
	{ this->capacity = capacity; start_block_index = index; }

	// getter
	int get_capacity(){ return capacity; }
	int get_start_block_index() { return start_block_index; }

};

class RsConfig
{
  private:
	RsServerInfo *server;

	int mesg_count;
	int parity_count;
	int k;
	int n;
	const char *filename_prefix;

	char tmp_filename[256];

  public:
	RsConfig(int *mesg_capacity, int mesg_server_count,
			int *parity_capacity, int parity_server_count,
			const char *prefix);
	~RsConfig();

	int get_k() { return k; }
	int get_p() { return n-k; }
	int get_n() { return n; }

	char *get_filename(int index);
	void set_filename_prefix(const char *prefix) { filename_prefix = prefix; }

	int get_mesg_count() { return mesg_count; }
	int get_parity_count() { return parity_count; }
	int get_server_count() { return mesg_count + parity_count; }

	int get_start_block(int index) { return server[index].get_start_block_index(); }
	int get_capacity(int index) { return server[index].get_capacity(); }
};



#endif
