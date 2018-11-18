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
int index_filter_get = 0;
int flag_table = 0;

MODULE_AUTHOR("spear_soul <v.merkel778@gmail.com>");
MODULE_DESCRIPTION("tufilter");
MODULE_LICENSE("GPL");

#define SUCCESS 0
#define DEVICE_NAME "tufilter_dev"
#define BUF_LEN 80
#define MAJOR_NUM 101

static struct nf_hook_ops nfin;
static struct nf_hook_ops nfout;

//функция перехвата приходящих пакетов
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
					if((ip_header->saddr == filter_table[i].ipaddr || filter_table[i].ipaddr == -1) && (ntohs(tcp_header->source) == filter_table[i].port|| filter_table[i].port == -1))
					{
						filter_table[i].col_packet+=1;
						filter_table[i].size_packet += ntohs(ip_header->tot_len) - (tcp_header->doff * 4) - (ip_header->ihl * 4);
						return NF_DROP;
					}
				}
			}
			else if(ip_header->protocol == filter_table[i].protocol)
			{
				udp_header = (struct udphdr *)(skb_transport_header(skb));
				if (udp_header)
				{
					if((ip_header->saddr == filter_table[i].ipaddr || filter_table[i].ipaddr == -1) && (ntohs(udp_header->source) == filter_table[i].port|| filter_table[i].port == -1))
					{
						filter_table[i].col_packet+=1;
						filter_table[i].size_packet += udp_header->len;
						return NF_DROP;
					}
				}
			}
		}
	}
	return NF_ACCEPT;
}

//функция для перехвата исходящих пакетов
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
					if((ip_header->daddr == filter_table[i].ipaddr || filter_table[i].ipaddr == -1) && (ntohs(tcp_header->dest) == filter_table[i].port|| filter_table[i].port == -1))
					{
						filter_table[i].col_packet += 1;
						filter_table[i].size_packet += ntohs(ip_header->tot_len) - (tcp_header->doff * 4) - (ip_header->ihl * 4);
						return NF_DROP;
					}
				}
			}
			else if(ip_header->protocol == IPPROTO_UDP)
			{
				udp_header = (struct udphdr *)(skb_transport_header(skb));
				if (udp_header)
				{
					if((ip_header->daddr == filter_table[i].ipaddr || filter_table[i].ipaddr == -1) && (ntohs(udp_header->dest) == filter_table[i].port|| filter_table[i].port == -1))
					{
						filter_table[i].col_packet+=1;
						filter_table[i].size_packet += udp_header->len;
						return NF_DROP;
					}
				}
			}
		}
	}
	return NF_ACCEPT;
}

//функция для удаления правила
void del_filter(int cmp_index)
{
	int i;
	for(i = cmp_index; i < col_filter - 1; i++)
	{
		filter_table[i] = filter_table[i + 1];
	}
}
//функция для сравнения правил
int cmp_filter(struct DATA_FILTER filter_str, struct DATA_SEND data, int index)
{
	if(filter_str.ipaddr == data.ipaddr && filter_str.port == data.port && filter_str.protocol == data.protocol)
	{
		return index;
	}
	return 0;
}

long device_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
	int i = 0;
	int cmp_index = 0; //для получения индекса совпадающих фильтров
	struct DATA_SEND data;
	//Реакция на различные команды ioctl
	switch (ioctl_num) {
	case IOCTL_SET_MSG:
		//принять новое правило
		flag_table = 0;
		copy_from_user(&data, (struct DATA_SEND *)ioctl_param, sizeof(struct DATA_SEND));

		if(data.filter == 1 && col_filter < MAX_COL_FILTER)
		{
			for(i = 0; i < col_filter || cmp_index; i++)
			{
				cmp_index = cmp_filter(filter_table[i], data, i);//в случае если такое правило уже существует пользователь об этом узнает
			}
			if(!cmp_index)
			{
				filter_table[col_filter].col_packet = 0;
				filter_table[col_filter].size_packet = 0;
				filter_table[col_filter].ipaddr = data.ipaddr;
				filter_table[col_filter].port = data.port;
				filter_table[col_filter].protocol = data.protocol;
				col_filter++;
			}
			else
			{
				flag_table = 2; //в случае если добовляемый фильтр уже
			}
		}
		else if(data.filter == 0)
		{
			for(i = 0; i < col_filter || cmp_index ; i++)
			{
				cmp_index = cmp_filter(filter_table[i], data, i);
			}
			del_filter(cmp_index);
			col_filter--;
		}
		else
		{
			flag_table = 1; //в случае если правил больше максимального числа, то пользователь об этом узнает
		}
		cmp_index = 0;
		break;
	case IOCTL_GET_MSG_COL:
		index_filter_get = 0; //обнуления счётчика пересланых сообщений
		copy_to_user((int *)ioctl_param, &col_filter, sizeof(int)); //передаём кол записей в таблице
		break;
	case IOCTL_GET_MSG:
		copy_to_user((struct DATA_FILTER *)ioctl_param, &filter_table[index_filter_get],  sizeof(struct DATA_FILTER));
		index_filter_get++; //после считывания увеличиваем счётчик, за количеством переданных запесей следит приложение
		break;
	case IOCTL_GET_FLAG_FILTER:
		copy_to_user((int *)ioctl_param, &flag_table, sizeof(int)); //возврат флага для пользователя
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

//Инициализация модуля - Регистрация символьного устройства и инциализация функций перехвата

int init_module()
{
	int ret_val;
	//Попытка регистрации символьного устройства
	ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);
	if (ret_val < 0) {
		printk("%s failed with %d\n",
		"Sorry, registering the character device ", ret_val);
		return ret_val;
	}
	//стурктура для регистрации функции перехвата сообщений
	nfin.hook     = hook_func_in;
	nfin.hooknum  = NF_INET_PRE_ROUTING;
	nfin.pf       = PF_INET;
	nfin.priority = NF_IP_PRI_FIRST;
	//регистрация этой функции
	nf_register_net_hook(&init_net, &nfin);
	nfout.hook     = hook_func_out;
	nfout.hooknum  = NF_INET_POST_ROUTING;
	nfout.pf       = PF_INET;
	nfout.priority = NF_IP_PRI_FIRST;
	nf_register_net_hook(&init_net, &nfout);

	//Небольшая подсказка для передачи данных, можно увидеть с помощью dmesg
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

//Завершение работы модуля - дерегистрация файла в /proc
void cleanup_module()
{
	//Дерегистрация устройства
	unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
	//Дерегистрация функций перехвата пакетов
	nf_unregister_net_hook(&init_net, &nfin);
	nf_unregister_net_hook(&init_net, &nfout);
}
