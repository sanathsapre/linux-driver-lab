/***************************************************************************//**
*  \file       07_LEB_GPIO_Ctrl.c
*
*  \details    Simple GPIO driver explanation
*
*  \author     Sanath Sapre
*
*  \Tested with Linux BBB rev C
*
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>  //copy_to/from_user()
#include <linux/gpio.h>     //GPIO
#include <linux/err.h>
#include <linux/version.h>

//LED is connected to this GPIO
#define GPIO_53 (53) //USR0 connects to GPIO1_21. GPIO number =  bank * 32 + pin => GPIO1 21 is (1*32)+21 = 53
 
dev_t dev = 0;
static struct class *dev_class;
static struct cdev sanath_cdev;
 
static int __init sanath_driver_init(void);
static void __exit sanath_driver_exit(void);
 
 
/*************** Driver functions **********************/
static int sanath_open(struct inode *inode, struct file *file);
static int sanath_release(struct inode *inode, struct file *file);
static ssize_t sanath_read(struct file *filp, 
                char __user *buf, size_t len,loff_t * off);
static ssize_t sanath_write(struct file *filp, 
                const char *buf, size_t len, loff_t * off);
/******************************************************/
 
//File operation structure 
static struct file_operations fops =
{
  .owner          = THIS_MODULE,
  .read           = sanath_read,
  .write          = sanath_write,
  .open           = sanath_open,
  .release        = sanath_release,
};

/*
** This function is to configure permissions for the dev node
*/

/* Kernel 6.x (const struct device *) */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)

static char *sanath_devnode(const struct device *dev, umode_t *mode)
{
    if (mode)
        *mode = 0666;
    return NULL;
}

#else   /* Kernel 5.x and below */

/* Kernel 5.x (non-const struct device *) */
static char *sanath_devnode(struct device *dev, umode_t *mode)
{
    if (mode)
        *mode = 0666;
    return NULL;
}

#endif

/*
** This function will be called when we open the Device file
*/ 
static int sanath_open(struct inode *inode, struct file *file)
{
  pr_info("Device File Opened...!!!\n");
  return 0;
}

/*
** This function will be called when we close the Device file
*/
static int sanath_release(struct inode *inode, struct file *file)
{
  pr_info("Device File Closed...!!!\n");
  return 0;
}

/*
** This function will be called when we read the Device file
*/ 
static ssize_t sanath_read(struct file *filp, 
                char __user *buf, size_t len, loff_t *off)
{
  uint8_t gpio_state = 0;
  
  //reading GPIO value
  gpio_state = gpio_get_value(GPIO_53);
  
  //write to user
  len = 1;
  if( copy_to_user(buf, &gpio_state, len) > 0) {
    pr_err("ERROR: Not all the bytes have been copied to user\n");
  }
  
  pr_info("Read function : GPIO_53 = %d \n", gpio_state);
  
  return 0;
}

/*
** This function will be called when we write the Device file
*/ 
static ssize_t sanath_write(struct file *filp, 
                const char __user *buf, size_t len, loff_t *off)
{
  uint8_t rec_buf[10] = {0};
  
  if( copy_from_user( rec_buf, buf, len ) > 0) {
    pr_err("ERROR: Not all the bytes have been copied from user\n");
  }
  
  pr_info("Write Function : GPIO_53 Set = %c\n", rec_buf[0]);
  
  if (rec_buf[0]=='1') {
    //set the GPIO value to HIGH
    gpio_set_value(GPIO_53, 1);
  } else if (rec_buf[0]=='0') {
    //set the GPIO value to LOW
    gpio_set_value(GPIO_53, 0);
  } else {
    pr_err("Unknown command : Please provide either 1 or 0 \n");
  }
  
  return len;
}

/*
** Module Init function
*/ 
static int __init sanath_driver_init(void)
{
  /*Allocating Major number*/
  if((alloc_chrdev_region(&dev, 0, 1, "sanath_Dev")) <0){
    pr_err("Cannot allocate major number\n");
    goto r_unreg;
  }
  pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));
 
  /*Creating cdev structure*/
  cdev_init(&sanath_cdev,&fops);
 
  /*Adding character device to the system*/
  if((cdev_add(&sanath_cdev,dev,1)) < 0){
    pr_err("Cannot add the device to the system\n");
    goto r_del;
  }
 
  /*Creating struct class*/
  if(IS_ERR(dev_class = class_create(THIS_MODULE,"sanath_class"))){
    pr_err("Cannot create the struct class\n");
    goto r_class;
  }
  dev_class->devnode = sanath_devnode;
  /*Creating device*/
  if(IS_ERR(device_create(dev_class,NULL,dev,NULL,"sanath_device"))){
    pr_err( "Cannot create the Device \n");
    goto r_device;
  }
  
  //Checking the GPIO is valid or not
  if(gpio_is_valid(GPIO_53) == false){
    pr_err("GPIO %d is not valid\n", GPIO_53);
    goto r_device;
  }
  
  //Requesting the GPIO
  if(gpio_request(GPIO_53,"GPIO_53") < 0){
    pr_err("ERROR: GPIO %d request\n", GPIO_53);
    goto r_gpio;
  }
  
  //configure the GPIO as output
  gpio_direction_output(GPIO_53, 0);
  
  /* Using this call the GPIO 53 will be visible in /sys/class/gpio/
  ** Now you can change the gpio values by using below commands also.
  ** echo 1 > /sys/class/gpio/gpio53/value  (turn ON the LED)
  ** echo 0 > /sys/class/gpio/gpio53/value  (turn OFF the LED)
  ** cat /sys/class/gpio/gpio53/value  (read the value LED)
  ** 
  ** the second argument prevents the direction from being changed.
  */
  gpio_export(GPIO_53, false);
  
  pr_info("Device Driver Insert...Done!!!\n");
  return 0;
 
r_gpio:
  gpio_free(GPIO_53);
r_device:
  device_destroy(dev_class,dev);
r_class:
  class_destroy(dev_class);
r_del:
  cdev_del(&sanath_cdev);
r_unreg:
  unregister_chrdev_region(dev,1);
  
  return -1;
}

/*
** Module exit function
*/ 
static void __exit sanath_driver_exit(void)
{
  gpio_unexport(GPIO_53);
  gpio_free(GPIO_53);
  device_destroy(dev_class,dev);
  class_destroy(dev_class);
  cdev_del(&sanath_cdev);
  unregister_chrdev_region(dev, 1);
  pr_info("Device Driver Remove...Done!!\n");
}
 
module_init(sanath_driver_init);
module_exit(sanath_driver_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sanath Sapre");
MODULE_DESCRIPTION("A simple device driver - GPIO Driver");
MODULE_VERSION("1.32");
