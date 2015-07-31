#include <stdio.h>
#include <string.h>
#include "updown_command.h"

const char *UpDownCommand::openssl_up_option = "enc -aes-128-cbc";
const char *UpDownCommand::openssl_down_option = "enc -d -aes-128-cbc";
const char *UpDownCommand::curl_up_option = "-s -k --connect-timeout 10 -T -";
const char *UpDownCommand::curl_down_option = "-s -k --connect-timeout 10";

UpDownCommand::UpDownCommand(const char *file_path,
		const char *openssl, const char *key,
		const char *curl, const char **server, int n_server)
{
	int openssl_size = strlen(openssl) + strlen(file_path) + strlen(key);
	int curl_size = strlen(curl);
	n = n_server;
	
	openssl_up_cmd = new char[openssl_size + 200];
	openssl_down_cmd = new char[openssl_size + 200];
	sprintf(openssl_up_cmd,
			"%s %s -k %s -in %s",
			openssl, openssl_up_option, key, file_path);
	sprintf(openssl_down_cmd,
			"%s %s -k %s -out %s",
			openssl, openssl_down_option, key, file_path);

	curl_up_cmd = new char *[n];
	curl_down_cmd = new char *[n];
	//const char *filename = basename(file_path);
	const char *filename = file_path + strlen("cache/");
	for(int i = 0; i < n; i++)
	{
		curl_up_cmd[i] = new char [curl_size + 200];
		curl_down_cmd[i] = new char [curl_size + 200];
		sprintf(curl_up_cmd[i],
				"%s %s %s%s-%03d",
				curl, curl_up_option, server[i], filename, i);
		sprintf(curl_down_cmd[i],
				"%s %s %s%s-%03d",
				curl, curl_down_option, server[i], filename, i);
	}
}


UpDownCommand::~UpDownCommand()
{
	delete[] openssl_up_cmd;
	delete[] openssl_down_cmd;
	for(int i = 0; i < n; i++)
	{
		delete[] curl_up_cmd[i];
		delete[] curl_down_cmd[i];
	}
	delete[] curl_up_cmd;
	delete[] curl_down_cmd;
}
