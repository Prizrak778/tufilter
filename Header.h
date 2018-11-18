//Структура для передачи из программы в модуль ядра
struct DATA_SEND
{
	int port;
	uint32_t ipaddr;
	int filter;
	int protocol;
	int flag_in_out;
};
//Структура для хранения правил/фильтров и передачи их из модуля ядра в программу
struct DATA_FILTER
{
	int port;
	uint32_t ipaddr;
	int protocol;
	uint32_t size_packet;
	int col_packet;
	int flag_in_out;
};
#define MAJOR_NUM 101 //номер символьного устройства

#define MAX_COL_FILTER 10

//Функции для передачи данных через ioctl
#define IOCTL_SET_MSG _IOR(MAJOR_NUM, 0, struct DATA_SEND *)
#define IOCTL_GET_MSG _IOR(MAJOR_NUM, 1, struct DATA_FILTER *)
#define IOCTL_GET_MSG_COL _IOR(MAJOR_NUM, 2, int *)
#define IOCTL_GET_FLAG_FILTER _IOR(MAJOR_NUM, 3, int *)

