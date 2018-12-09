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
#define FLAG_ROUTE "--route"
#define FLAG_SHOW "show"
#define FLAG_HELP "help"
#define TCP_CONST_PROTOCOL 6
#define UDP_CONST_PROTOCOL 17 //В модуле ядра 0 - tcp, 17 - udp;
#define MAX_COL_PORT 65535
#define PROTOCOL_STR_LEN 4
#define IP_STR_LEN 16
#define ROUTE_STR_LEN 16

//функция для отправки одного сообещния
void ioctl_set_msg(int file_desc, struct DATA_SEND *messag)
{
	int ret_val = ioctl(file_desc, IOCTL_SET_MSG, messag);
	if(ret_val < 0)
	{
		printf("Ошибка при вызове ioctl_set_msg: %d\n", ret_val);
		free(messag);
		messag = NULL;
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
		free(messag);
		messag = NULL;
		exit(-1);
	}
}

void show_route_filter(char route_str[], int flag)
{
	if(flag == 1)
	{
		strcpy(route_str, "output");
	}
	else if(flag == -1)
	{
		strcpy(route_str, "input");
	}
	else
	{
		strcpy(route_str, "input&output");
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
		free(messag);
		messag = NULL;
		exit(-1);
	}
	char protocol_str[PROTOCOL_STR_LEN];
	char ipaddr_get[IP_STR_LEN];
	char route_str[ROUTE_STR_LEN];
	struct in_addr in_addr_get;
	//основной вывод
	printf("num\tpkts\tbytes\t\ttarget\tprot\tsourse\t\tChain\n");
	if(col_row < 1)
	{
		printf("-- empty -- \n");
	}
	for(int i = 0; i < col_row; i++)
	{
		ioctl_get_msg(file_desc, messag);
		show_route_filter(route_str, messag->flag_in_out);
		messag->protocol == TCP_CONST_PROTOCOL ? strcpy(protocol_str, "tcp") : strcpy(protocol_str, "udp");
		messag->ipaddr == -1 ? strcpy(ipaddr_get, "0.0.0.0") : (in_addr_get.s_addr = messag->ipaddr, strcpy(ipaddr_get, inet_ntoa(in_addr_get)));
		printf("%d\t%u\t%d\t\tDROP\t%s\t%s", i, messag->col_packet, messag->size_packet, protocol_str, ipaddr_get);
		if(messag->port != -1)
		{
			printf(":%d", messag->port);
		}
		printf("\t%s", route_str);
		printf("\n");
	}
	free(messag);
	messag = NULL;
}

void ioctl_err(struct DATA_SEND *data)
{
	printf("Try 'tufilter --help' for more information\n");
	free(data);
	data = NULL;
	exit(-1);
}

//функция для добавления/удаления правила/фильтра
void ioctl_change_filter(int argc, char *argv[], int file_desc)
{
	struct DATA_SEND *data = malloc(sizeof(struct DATA_SEND));
	int filter_flag = -1, port_flag = -1, ipaddr_flag = -1, route_flag = -1;
	//проверка на наличия нужных флагов и их значений
	for(int i = 0; i < argc; i++)
	{
		if(!strcmp(argv[i], FLAG_FILTER)) filter_flag = i;
		if(!strcmp(argv[i], FLAG_IP)) ipaddr_flag = i;
		if(!strcmp(argv[i], FLAG_PORT)) port_flag = i;
		if(!strcmp(argv[i], FLAG_ROUTE)) route_flag = i;
	}
	if(filter_flag == -1 || (ipaddr_flag == -1 && port_flag == -1))
	{
		ioctl_err(data);
	}
	if((strcasecmp(argv[route_flag + 1], "input") && strcasecmp(argv[route_flag + 1], "output")) && route_flag != -1)
	{
		ioctl_err(data);
	}
	if(strcasecmp(argv[filter_flag + 1], "enable") && strcasecmp(argv[filter_flag + 1], "disable"))
	{
		ioctl_err(data);
	}
	int port_check = atoi(argv[port_flag + 1]);
	if((port_check > MAX_COL_PORT || 0 > port_check) && port_flag > 0)
	{
		ioctl_err(data);
	}
	struct in_addr in_addr_send;
	if((inet_aton(argv[ipaddr_flag + 1], &in_addr_send) == 0) && ipaddr_flag > 0)
	{
		ioctl_err(data);
	}
	//запись в структуру для передачи
	//по хорошему следую строку надо написать по другом
	route_flag == -1 ? data->flag_in_out = 0: (strcasecmp(argv[route_flag + 1], "input") == 0 ? data->flag_in_out = -1 : (data->flag_in_out = 1));
	port_flag == -1 ? (data->port = -1) : (data->port = atoi(argv[port_flag + 1]));
	ipaddr_flag == -1 ? (data->ipaddr = -1) : (data->ipaddr = in_addr_send.s_addr);
	strcasecmp(argv[filter_flag + 1], "enable") == 0 ? (data->filter = 1) : (data->filter = 0); //strcasecmp - функция для сравнения строк без учёта регистра
	strcasecmp(argv[2], "tcp") == 0 ? (data->protocol = TCP_CONST_PROTOCOL) : (data->protocol = UDP_CONST_PROTOCOL);
	ioctl_set_msg(file_desc, data);
	free(data);
}
//функция для вывода результата добавления фильтра
void show_answer(int file_desc)
{
	int flag_filter;
	//получение количество активных правил
	int ret_val = ioctl(file_desc, IOCTL_GET_FLAG_FILTER, &flag_filter);
	if(ret_val < 0)
	{
		printf("Ошибка при вызове ioctl_show_filter: %d\n", ret_val);
		exit(-1);
	}
	if(flag_filter == 1)
		printf("Максимальное количество активных правил, новое правило не добавлено\n");
	else if(flag_filter == 2)
		printf("Такое правило уже существует\n");
	else
		printf("done\n");
}

void show_help()
{
	printf("./tufilter --show\t показывает активные правила, если они есть и статистику по ним(кол пакетов и размер)\n");
	printf("./tufilter --transport proto добавить или удалить фильтр, protocol имя протокола (tcp или udp)\n\n");
	printf("Опции(для --transport):\n");
	printf("--ip addres\t указать ip адрес(ipv4) для блокировки, может отсутствовать\n");
	printf("--port port\t указать порт для блокировки, может отсутствовать\n");
	printf("--route route\t указать какие пакеты фильтровать: входящие(input), исходящие(output)\n");
	printf("--filter filter\t указать отключить(disable) или включить(enable) данный фильтр\n\n");
	printf("Пример:\n");
	printf("./tufilter --transport tcp --ip 8.8.8.8 --port 443 --filter enable\n");
	printf("./tufilter --transport tcp --port 80 --filter Enable\n");
	printf("./tufilter --transport tcp --port 81 --filter Enable --route input\n");
	printf("./tufilter --transport tcp --port 81 --filter Enable --route output\n");
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
	{"port", 1, NULL, 4},
	{"ip", 1, NULL , 4},
	{"filter", 1, NULL , 4},
	{"filter", 1, NULL , 4},
	{"route", 1, NULL , 4},
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
		show_answer(file_desc);
		break;
	case 2:
		ioctl_show_filter(file_desc);
		printf("show done \n");
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
