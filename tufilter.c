#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include "Header.h"


#define DEVICE_FILE_NAME "tufilter_dev"
#define FLAG_TRANSPORT "--transport"
#define FLAG_FILTER "--filter"
#define FLAG_IP "--ip"
#define FLAG_PORT "--port"
#define FLAG_SHOW "--show"
#define FLAG_HELP "--help"

void ioctl_set_msg(int file_desc, struct DATA_SEND *messag)
{
	int ret_val = ioctl(file_desc, IOCTL_SET_MSG, messag);
	if(ret_val < 0)
	{
		printf("Ошибка при вызове ioctl_set_msg: %d\n", ret_val);
		exit(-1);
	}
}
void ioctl_show_filter(int file_desc)
{
	struct DATA_SEND *messag = malloc(sizeof(struct DATA_SEND));
	int ret_val = ioctl(file_desc, IOCTL_GET_MSG, messag);
	if(ret_val < 0)
	{
		printf("Ошибка при вызове ioctl_show_filter: %d\n", ret_val);
		exit(-1);
	}
}

void ioctl_change_filter(int argc, char *argv[], int file_desc)
{
	struct DATA_SEND *data = malloc(sizeof(struct DATA_SEND));
	int filter_flag = -1, port_flag = -1, ipaddr_flag = -1;
	for(int i = 0; i < argc; i++)
	{
		if(!strcmp(argv[i], FLAG_FILTER)) filter_flag = i;
		if(!strcmp(argv[i], FLAG_IP)) ipaddr_flag = i;
		if(!strcmp(argv[i], FLAG_FILTER)) port_flag = i;
	}
	if(filter_flag == -1 || (ipaddr_flag == -1 && port_flag == -1))
	{
		printf("Try 'tufilter --help' for more information\n");
		exit(-1);
	}
	if(strcmp(argv[filter_flag + 1], "enable") && strcmp(argv[filter_flag + 1], "disable"))
	{
		printf("Try 'tufilter --help' for more information\n");
		exit(-1);
	}
	int port_check = atoi(argv[port_flag + 1]);
	if((port_check > 65536 || 0 < port_check) && port_flag > 0)
	{
		printf("Try 'tufilter --help' for more information\n");
		exit(-1);
	}
	struct in_addr in_addr_test;
	if((inet_aton(argv[ipaddr_flag + 1], &in_addr_test) == 0) && ipaddr_flag > 0)
	{
		printf("Try 'tufilter --help' for more information\n");
		exit(-1);
	}
	data->port = atoi(argv[port_flag + 1]);
	ipaddr_flag == -1 ? strcmp(data->ipaddr, "*\0") : strcmp(data->ipaddr, argv[ipaddr_flag + 1]);
	strcmp(argv[filter_flag + 1], "enable") == 0 ? (data->filter = 1) : (data->filter = 0);
	strcmp(data->protocol, argv[port_flag + 1]);
	ioctl_set_msg(file_desc, data);
}


int main(int argc, char *argv[])
{
	int file_desc;
	if((file_desc = open(DEVICE_FILE_NAME, 0)) < 0)
	{
		printf("Невозможно открыть файл устройства: %s\n", DEVICE_FILE_NAME);
		exit(-1);
	}
	if(argc < 2)
	{
		printf("Try 'tufilter --help' for more information\n");
		exit(-1);
	}
	else if(!strcmp(argv[1], FLAG_TRANSPORT))
	{
		ioctl_change_filter(argc, argv, file_desc);
		printf("--transport done \n");
	}
	else if(!strcmp(argv[1], FLAG_SHOW) && argc == 2)
	{
		ioctl_show_filter(file_desc);
		printf("--show done \n");
	}
	else if(!strcmp(argv[1], FLAG_HELP))
	{
		printf("--help done \n");
	}
	else
	{
		printf("Try 'tufilter --help' for more information\n");
		exit(-1);
	}
	for(int i = 0; i < argc; i++)
	{
		printf("%s ", argv[i]);
	}
	printf("\n");
	close(file_desc);
	return 0;
}
