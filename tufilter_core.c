#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include "Header.h"
#include <asm/uaccess.h>        /* определения функций get_user и put_user */

#define SUCCESS 0
#define DEVICE_NAME "tufilter_dev"
#define BUF_LEN 80
#define MAJOR_NUM 101

static int Device_Open = 0;
static char Message[BUF_LEN];
static char *Message_Ptr;

static int device_open(struct inode *inode, struct file *file)
{
#ifdef DEBUG
  printk("device_open(%p)\n", file);
#endif

  /*
   * В каждый конкретный момент времени только один процесс может открыть файл устройства
   */
  if (Device_Open)
	  return -EBUSY;

  Device_Open++;
  /*
   * Инициализация сообщения
   */
  Message_Ptr = Message;
  try_module_get(THIS_MODULE);
  return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
#ifdef DEBUG
  printk("device_release(%p,%p)\n", inode, file);
#endif

  /*
   * Теперь мы готовы принять запрос от другого процесса
   */
  Device_Open--;

  module_put(THIS_MODULE);
  return SUCCESS;
}

long device_ioctl(/*struct inode *inode,*/ /* см. include/linux/fs.h */
struct file *file,              /* то же самое */
unsigned int ioctl_num,         /* номер и аргументы ioctl */
unsigned long ioctl_param)
{
  int i;
  char *temp;
  char ch;

  /*
   * Реакция на различные команды ioctl
   */
  switch (ioctl_num) {
  case IOCTL_SET_MSG:
	  /*
	 * Принять указатель на сообщение (в пространстве пользователя)
	 * и переписать в буфер. Адрес которого задан в дополнительно аргументе.
	 */
	  temp = (char *)ioctl_param;

	/*
	 * Найти длину сообщения
	 */
	get_user(ch, temp);
	for (i = 0; ch && i < BUF_LEN; i++, temp++)
		get_user(ch, temp);

	//device_write(file, (char *)ioctl_param, i, 0);
	  break;

  case IOCTL_GET_MSG:
	  /*
	 * Передать текущее сообщение вызывающему процессу -
	 * записать по указанному адресу.
	 */
	  //i = device_read(file, (char *)ioctl_param, 99, 0);

	/*
	 * Вставить в буфер завершающий символ \0
	 */
	put_user('\0', (char *)ioctl_param + i);
	  break;

  }

  return SUCCESS;
}

struct file_operations Fops = {
  .read = NULL,
  .write = NULL,
  .unlocked_ioctl = device_ioctl,
  .open = device_open,
  .release = device_release,    /* оно же close */
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
}
