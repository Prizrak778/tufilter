struct DATA_SEND
{
	int port;
	uint32_t ipaddr;
	int filter;
	int protocol;
};
struct DATA_FILTER
{
	int port;
	uint32_t ipaddr;
	int protocol;
	int size_packet;
	int col_packet;
};
#define MAX_COL_FILTER 10
#define MAJOR_NUM 101
#define IOCTL_SET_MSG _IOR(MAJOR_NUM, 0, struct DATA_SEND *)
#define IOCTL_GET_MSG _IOR(MAJOR_NUM, 1, struct DATA_FILTER *)
#define IOCTL_GET_MSG_COL _IOR(MAJOR_NUM, 2, int *)

