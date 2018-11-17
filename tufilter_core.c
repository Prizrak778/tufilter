#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <net/ip.h>
#include <linux/inet.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include "Header.h"

struct DATA_FILTER filter_table[MAX_COL_FILTER];
int col_filter = 0;

MODULE_AUTHOR("Double <v.merkel778@gmail.com>");
MODULE_DESCRIPTION("tufilter");
MODULE_LICENSE("GPL");

#define SUCCESS 0
#define DEVICE_NAME "tufilter_dev"
#define BUF_LEN 80
#define MAJOR_NUM 101

static struct nf_hook_ops nfin;
static struct nf_hook_ops nfout;

static unsigned int hook_func_in(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)

{
	struct iphdr *ip_header;
	struct tcphdr *tcp_header;
	struct udphdr *udp_header;
	int i = 0;
	if(skb->protocol == htons(ETH_P_IP))
	{
		ip_header = (struct iphdr *)skb_network_header(skb);
		for(i = 0; i < col_filter; i++)
		{
			// Проверяем что это внутри TCP или UDP пакет
			if (ip_header->protocol == filter_table[i].protocol)
			{
				tcp_header = (struct tcphdr *)(skb_transport_header(skb));
				if (tcp_header)
				{
					if((ip_header->saddr == filter_table[i].ipaddr || filter_table[i].ipaddr == -1) && (ntohs(tcp_header->source) == filter_table[i].port|| !(filter_table[i].port)))
					{
						printk("tcp_in DROP");
						return NF_DROP;
					}
				}
			}
			else if(ip_header->protocol == filter_table[i].protocol)
			{
				udp_header = (struct udphdr *)(skb_transport_header(skb));
				if (udp_header)
				{
					if((ip_header->saddr == filter_table[i].ipaddr || filter_table[i].ipaddr == -1) && (ntohs(udp_header->source) == filter_table[i].port|| !(filter_table[i].port)))
					{
						printk("udp_in DROP");
						return NF_DROP;
					}
				}
			}
		}
	}
	return NF_ACCEPT;
}
static unsigned int hook_func_out(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)

{
	struct iphdr *ip_header;
	struct tcphdr *tcp_header;
	struct udphdr *udp_header;
	int i = 0;
	if(skb->protocol == htons(ETH_P_IP))
	{
		ip_header = (struct iphdr *)skb_network_header(skb);
		// Проверяем что это внутри TCP или UDP пакет
		for(i = 0; i < col_filter; i++)
		{
			if (ip_header->protocol == IPPROTO_TCP)
			{
				tcp_header = (struct tcphdr *)(skb_transport_header(skb));
				if (tcp_header)
				{
					if((ip_header->daddr == filter_table[i].ipaddr || filter_table[i].ipaddr == -1) && (ntohs(tcp_header->dest) == filter_table[i].port|| !(filter_table[i].port)))
					{
						printk("tcp_in DROP");
						return NF_DROP;
					}
				}
			}
			else if(ip_header->protocol == IPPROTO_UDP)
			{
				udp_header = (struct udphdr *)(skb_transport_header(skb));
				if (udp_header)
				{
					if((ip_header->daddr == filter_table[i].ipaddr || filter_table[i].ipaddr == -1) && (ntohs(udp_header->dest) == filter_table[i].port|| !(filter_table[i].port)))
					{
						printk("udp_in DROP");
						return NF_DROP;
					}
				}
			}
		}
	}
	return NF_ACCEPT;
}

long device_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
	int i;
	struct DATA_SEND data;
	int flag_end_table;
	/*
	* Реакция на различные команды ioctl
	*/
	switch (ioctl_num) {
	case IOCTL_SET_MSG:
		//принять новое правило
		flag_end_table = 0;
		copy_from_user(&data, (struct DATA_SEND *)ioctl_param, sizeof(struct DATA_SEND));
		if(data.filter == 1 && col_filter < MAX_COL_FILTER)
		{
			printk("%d", data.ipaddr);
			filter_table[col_filter].col_packet = 0;
			filter_table[col_filter].size_packet = 0;
			filter_table[col_filter].ipaddr = data.ipaddr;
			filter_table[col_filter].port = data.port;
			filter_table[col_filter].protocol = data.protocol;
			col_filter++;
		}
		else if(data.filter == 0)
		{

		}
		else
		{
			flag_end_table = 1; //в случае если правил больше максимального числа, то пользователь об этом узнает
		}
		printk("read, port = %d, ip_addr = %d, filter = %d, protocol = %d", filter_table[col_filter - 1].port, filter_table[col_filter - 1].ipaddr, data.filter, filter_table[col_filter - 1].protocol);
		break;
	case IOCTL_GET_MSG_COL:
		i = 0; //обнуления счётчика пересланых сообщений
		copy_to_user(&col_filter, (int *)ioctl_param, sizeof(int)); //передаём кол записей в таблице
		printk("write %d\n", col_filter);
		break;
	case IOCTL_GET_MSG:
		copy_to_user(&filter_table[i], (struct DATA_SEND *)ioctl_param, sizeof(struct DATA_SEND));
		i++; //после считывания увеличиваем счётчик, за количеством переданных запесей следит приложение
		break;
	}
	return SUCCESS;
}

struct file_operations Fops = {
  .read = NULL,
  .write = NULL,
  .unlocked_ioctl = device_ioctl,
  .open = NULL,
  .release = NULL,
};

/*
 * Инициализация модуля - Регистрация символьного устройства
 */
int init_module()
{
	int ret_val;
  /*
   * Регистрация символьного устройства (по крайней мере - попытка регистрации)
   */
  ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);

  /*
   * Отрицательное значение означает ошибку
   */
  if (ret_val < 0) {
	  printk("%s failed with %d\n",
	"Sorry, registering the character device ", ret_val);
	return ret_val;
  }

  nfin.hook     = hook_func_in;
  nfin.hooknum  = NF_INET_PRE_ROUTING;
  nfin.pf       = PF_INET;
  nfin.priority = NF_IP_PRI_FIRST;
  nf_register_net_hook(&init_net, &nfin);
  nfout.hook     = hook_func_out;
  nfout.hooknum  = NF_INET_LOCAL_OUT;
  nfout.pf       = PF_INET;
  nfout.priority = NF_IP_PRI_FIRST;
  nf_register_net_hook(&init_net, &nfout);
  printk("%s The major device number is %d.\n",
  "Registeration is a success", MAJOR_NUM);
  printk("If you want to talk to the device driver,\n");
  printk("you'll have to create a device file. \n");
  printk("We suggest you use:\n");
  printk("mknod %s c %d 0\n", DEVICE_NAME, MAJOR_NUM);
  printk("The device file name is important, because\n");
  printk("the ioctl program assumes that's the\n");
  printk("file you'll use.\n");

  return 0;
}

/*
 * Завершение работы модуля - дерегистрация файла в /proc
 */
void cleanup_module()
{
  /*
   * Дерегистрация устройства
   */
  unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
  nf_unregister_net_hook(&init_net, &nfin);
  nf_unregister_net_hook(&init_net, &nfout);
}
