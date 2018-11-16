struct DATA_SEND
{
	int port;
	char ipaddr[16];
	int filter;
	int protocol;
};
#define MAJOR_NUM 101
#define IOCTL_SET_MSG _IOR(MAJOR_NUM, 0, struct DATA_SEND *)
#define IOCTL_GET_MSG _IOR(MAJOR_NUM, 1, struct DATA_SEND *)
#define IOCTL_GET_MSG_COL _IOR(MAJOR_NUM, 2, struct int *)

