#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>        /* определения функций get_user и put_user */

#define SUCCESS 0
#define DEVICE_NAME "tufilter_dev"
#define BUF_LEN 80
#define MAJOR_NUM 101

struct file_operations Fops = {
  .read = NULL,
  .write = NULL,
  .unlocked_ioctl = NULL,
  .open = NULL,
  .release = NULL,    /* оно же close */
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
