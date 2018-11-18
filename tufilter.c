#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include "Header.h"

#define DEVICE_FILE_NAME "tufilter_dev"
#define FLAG_TRANSPORT "transport"
#define FLAG_FILTER "--filter"
#define FLAG_IP "--ip"
#define FLAG_PORT "--port"
#define FLAG_SHOW "show"
#define FLAG_HELP "help"
#define TCP_CONST_PROTOCOL 6
#define UDP_CONST_PROTOCOL 17 //В модуле ядра 0 - tcp, 17 - udp;

//функция для отправки одного сообещния
void ioctl_set_msg(int file_desc, struct DATA_SEND *messag)
{
	int ret_val = ioctl(file_desc, IOCTL_SET_MSG, messag);
	if(ret_val < 0)
	{
		printf("Ошибка при вызове ioctl_set_msg: %d\n", ret_val);
		exit(-1);
	}
}

//функция для получения одного сообщения
void ioctl_get_msg(int file_desc, struct DATA_FILTER *messag)
{
	int ret_val = ioctl(file_desc, IOCTL_GET_MSG, messag);
	if(ret_val < 0)
	{
		printf("Ошибка при вызове ioctl_set_msg: %d\n", ret_val);
		exit(-1);
	}
}

//функция для просмотра активных правил и статистики по ним
void ioctl_show_filter(int file_desc)
{
	struct DATA_FILTER *messag = malloc(sizeof(struct DATA_FILTER));
	int col_row;
	//получение количество активных правил
	int ret_val = ioctl(file_desc, IOCTL_GET_MSG_COL, &col_row);
	if(ret_val < 0)
	{
		printf("Ошибка при вызове ioctl_show_filter: %d\n", ret_val);
		exit(-1);
	}
	char protocol_str[4];
	char ipaddr_get[16];
	struct in_addr in_addr_get;
	//основной вывод
	printf("num\tpkts\tbytes\t\ttarget\tprot\tsourse\n");
	if(col_row < 1)
	{
		printf("-- empty -- \n");
	}
	for(int i = 0; i < col_row; i++)
	{
		ioctl_get_msg(file_desc, messag);
		messag->protocol == TCP_CONST_PROTOCOL ? strcpy(protocol_str, "tcp") : strcpy(protocol_str, "udp");
		messag->ipaddr == -1 ? strcpy(ipaddr_get, "0.0.0.0") : (in_addr_get.s_addr = messag->ipaddr, strcpy(ipaddr_get, inet_ntoa(in_addr_get)));
		printf("%d\t%u\t%d\t\tDROP\t%s\t%s", i, messag->col_packet, messag->size_packet, protocol_str, ipaddr_get);
		if(messag->port != -1)
		{
			printf(":%d", messag->port);
		}
		printf("\n");
	}
	free(messag);

}

//функция для добавления/удаления правила/фильтра
void ioctl_change_filter(int argc, char *argv[], int file_desc)
{
	struct DATA_SEND *data = malloc(sizeof(struct DATA_SEND));
	int filter_flag = -1, port_flag = -1, ipaddr_flag = -1;
	//проверка на наличия нужных флагов и их значений
	for(int i = 0; i < argc; i++)
	{
		if(!strcmp(argv[i], FLAG_FILTER)) filter_flag = i;
		if(!strcmp(argv[i], FLAG_IP)) ipaddr_flag = i;
		if(!strcmp(argv[i], FLAG_PORT)) port_flag = i;
	}
	if(filter_flag == -1 || (ipaddr_flag == -1 && port_flag == -1))
	{
		printf("Try 'tufilter --help' for more information\n");
		exit(-1);
	}

	if(strcasecmp(argv[filter_flag + 1], "enable") && strcmp(argv[filter_flag + 1], "disable"))
	{
		printf("Try 'tufilter --help' for more information\n");
		exit(-1);
	}
	int port_check = atoi(argv[port_flag + 1]);
	if((port_check > 65536 || 0 > port_check) && port_flag > 0)
	{
		printf("Try 'tufilter --help' for more information\n");
		exit(-1);
	}
	struct in_addr in_addr_send;
	if((inet_aton(argv[ipaddr_flag + 1], &in_addr_send) == 0) && ipaddr_flag > 0)
	{
		printf("Try 'tufilter --help' for more information\n");
		exit(-1);
	}
	//запись в структуру для передачи
	port_flag == -1 ? (data->port = -1) : (data->port = atoi(argv[port_flag + 1]));
	ipaddr_flag == -1 ? (data->ipaddr = -1) : (data->ipaddr = in_addr_send.s_addr);
	strcasecmp(argv[filter_flag + 1], "enable") == 0 ? (data->filter = 1) : (data->filter = 0); //strcasecmp - функция для сравнения строк без учёта регистра
	strcasecmp(argv[2], "tcp") == 0 ? (data->protocol = TCP_CONST_PROTOCOL) : (data->protocol = UDP_CONST_PROTOCOL);
	ioctl_set_msg(file_desc, data);
	free(data);
}

void show_help()
{
	printf("./tufilter --show - показывает активные правила, если они есть, и статистику по ним(кол пакетов и размер)\n");
	printf("./tufilter --transport proto - добавить или удалить фильтр, protocol имя протокола (tcp или udp)\n\n");
	printf("Опции(для --transport):\n");
	printf("--ip addres указать ip адрес(ipv4) для блокировки, может отсутствовать\n");
	printf("--port port указать порт для блокировки, может отсутствовать\n");
	printf("--filter filter указать отключить(disable) или включить(enable) данный фильтр\n\n");
	printf("Пример:\n");
	printf("./tufilter --transport tcp --ip 8.8.8.8 --port 443 --filter enable\n");
	printf("./tufilter --transport tcp --port 80 --filter Enable\n");
	printf("./tufilter --transport udp --ip 8.8.8.8 --filter Enable\n");
	printf("./tufilter --transport udp --ip 8.8.8.8 --filter disable\n");
	printf("./tufilter --show\n");
}

int main(int argc, char *argv[])
{
	int flag_case;
	const char* short_options = "h";
	int option_index = -1;
	//структура для "длинных" входных параметров
	const struct option long_options[] =
	{
	{FLAG_TRANSPORT, 1, &flag_case , 1},
	{"port", 1, NULL, 1},
	{"ip", 1, NULL , 1},
	{"filter", 1, NULL , 1},
	{FLAG_SHOW, 0, &flag_case, 2},
	{FLAG_HELP, 0, &flag_case, 3},
	{NULL, 0, NULL, 0}
};
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
	while (getopt_long (argc, argv, short_options, long_options, &option_index) != -1);
	switch (flag_case) {
	case 1:
		ioctl_change_filter(argc, argv, file_desc);
		printf("--transport done \n");
		break;
	case 2:
		ioctl_show_filter(file_desc);
		printf("--show done \n");
		break;
	case 3:
		show_help();
		break;
	default:
		printf("Try 'tufilter --help' for more information\n");
		exit(1);
		break;
	}

	close(file_desc);
	return 0;
}
