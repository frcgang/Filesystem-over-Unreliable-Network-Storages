#ifndef _UPDOWN_COMMAND
#define _UPDOWN_COMMAND

class UpDownCommand
{
	private:
		static const char *openssl_up_option;
		static const char *openssl_down_option;
		static const char *curl_up_option;
		static const char *curl_down_option;

	public:
		char *openssl_up_cmd;
		char *openssl_down_cmd;
		char **curl_up_cmd;
		char **curl_down_cmd;
		int n;

		UpDownCommand(const char *file_path,
			const char *openssl, const char *key,
			const char *curl, const char **server, int n_server);
		~UpDownCommand();
};

#endif
