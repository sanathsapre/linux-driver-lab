/***************************************************************************//**
*  \file       01_msg_queue_driver.c
*
*  \details    Simple Linux device driver kernel buffer
*
*  \author     Sanath Sapre
*
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include<linux/slab.h>                 //kmalloc()
#include<linux/uaccess.h>              //copy_to/from_user()
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/stddef.h>
 
#define ROW_SIZE        3           //Row Size
#define MEM_SIZE        128           //Memory Size
#define DONE    1
 
dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;
typedef struct kernel_logger
{
    uint8_t kernel_buffer[ROW_SIZE][MEM_SIZE];
    uint8_t read_indexer;
    uint8_t write_indexer;
    struct mutex etx_mutex;
    uint8_t count;

} kernel_logger_t;

kernel_logger_t kernel_logger;
static DECLARE_WAIT_QUEUE_HEAD(etx_wait_queue);
/*
** Function Prototypes
*/
static int      __init etx_driver_init(void);
static void     __exit etx_driver_exit(void);
static int      etx_open(struct inode *inode, struct file *file);
static int      etx_release(struct inode *inode, struct file *file);
static ssize_t  etx_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t  etx_write(struct file *filp, const char *buf, size_t len, loff_t * off);


/*
** File Operations structure
*/
static struct file_operations fops =
{
        .owner          = THIS_MODULE,
        .read           = etx_read,
        .write          = etx_write,
        .open           = etx_open,
        .release        = etx_release,
};
 
/*
** This function is to configure permissions for the dev node
*/

static char *etx_devnode(const struct device *dev, umode_t *mode)
{
    if (mode)
        *mode = 0666;    /* rw for all */
    return NULL;
}

/*
** This function will be called when we open the Device file
*/
static int etx_open(struct inode *inode, struct file *file)
{
        int *done = 0;

        done = (int *)kmalloc(sizeof(int), GFP_KERNEL);

        if(!done)
        {
                pr_info("Malloc failed");
                return -ENOMEM;
        }
        
        *done = 0; //Initialize done to zero
        file->private_data = done;

        pr_info("Device File Opened...!!!\n");
        return 0;
}

/*
** This function will be called when we close the Device file
*/
static int etx_release(struct inode *inode, struct file *file)
{
        kfree(file->private_data);
        file->private_data = NULL;    /* safety — prevent dangling pointer */

        pr_info("Device File Closed...!!!\n");
        return 0;
}

/*
** This function will be called when we read the Device file
*/
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
        ssize_t ret = 0;
        int *done = filp->private_data;

        if(*done)
                return 0;

        //Copy the data from the kernel space to the user-space
        mutex_lock(&kernel_logger.etx_mutex);

        /* empty check now uses count, not index comparison */
        while (kernel_logger.count == 0)
        {
              pr_info("Data Read : Buffer empty\n");
              mutex_unlock(&kernel_logger.etx_mutex);
              if(wait_event_interruptible_exclusive(etx_wait_queue, kernel_logger.count > 0))
                return -ERESTARTSYS;

              mutex_lock(&kernel_logger.etx_mutex);

        }

        if( copy_to_user(buf, kernel_logger.kernel_buffer[kernel_logger.read_indexer], MEM_SIZE) )
        {
                pr_err("Data Read : Err!\n");
                ret = -EFAULT;
                goto out;
        }
        ret = MEM_SIZE;

        pr_info("Data Read : Done!\n");
        memset(kernel_logger.kernel_buffer[kernel_logger.read_indexer], 0, MEM_SIZE);
        kernel_logger.read_indexer = (kernel_logger.read_indexer + 1) % ROW_SIZE;
        kernel_logger.count--;
        *done = DONE;
out:
        mutex_unlock(&kernel_logger.etx_mutex);
        return ret;
}

/*
** This function will be called when we write the Device file
*/
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{

        //Copy the data to kernel space from the user-space
        size_t copy_len = min(len, (size_t)(MEM_SIZE - 1));
        ssize_t ret = 0;

        mutex_lock(&kernel_logger.etx_mutex);
        if( copy_from_user(kernel_logger.kernel_buffer[kernel_logger.write_indexer], buf, copy_len))
        {
                pr_err("Data Write : Err!\n");
                ret = -EFAULT;
                goto out;

        }
        kernel_logger.kernel_buffer[kernel_logger.write_indexer][copy_len] = '\0';
        ret = copy_len;
        pr_info("Data Write : Done!\n");
        kernel_logger.write_indexer = (kernel_logger.write_indexer + 1) % ROW_SIZE;

        /* If buffer is not full, increment count.
        * If full, overwrite oldest entry by bumping read_indexer forward.
        */
        if (kernel_logger.count < ROW_SIZE)

                kernel_logger.count++;

        else
                /* full — oldest overwritten, bump read to stay valid */
                kernel_logger.read_indexer = (kernel_logger.read_indexer + 1) % ROW_SIZE;
        wake_up_interruptible(&etx_wait_queue);
out:
        mutex_unlock(&kernel_logger.etx_mutex);
        return ret;
}

/*
** Module Init function
*/
static int __init etx_driver_init(void)
{
        /*Allocating Major number*/
        if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) <0){
                pr_info("Cannot allocate major number\n");
                return -1;
        }
        pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));
 
        /*Creating cdev structure*/
        cdev_init(&etx_cdev,&fops);
 
        /*Adding character device to the system*/
        if((cdev_add(&etx_cdev,dev,1)) < 0){
            pr_info("Cannot add the device to the system\n");
            goto r_class;
        }
 
        /*Creating struct class*/
        if(IS_ERR(dev_class = class_create("sanath_class"))){
            pr_info("Cannot create the struct class\n");
            goto r_class;
        }

        dev_class->devnode = etx_devnode;

        /*Creating device*/
        if(IS_ERR(device_create(dev_class,NULL,dev,NULL,"sanath_queue"))){
            pr_info("Cannot create the Device 1\n");
            goto r_device;
        }

        mutex_init(&kernel_logger.etx_mutex);

        //strcpy(kernel_buffer, "Hello_World");
        
        pr_info("Device Driver Insert...Done!!!\n");
        return 0;
 
r_device:
        class_destroy(dev_class);
r_class:
        unregister_chrdev_region(dev,1);
        return -1;
}

/*
** Module exit function
*/
static void __exit etx_driver_exit(void)
{       
        device_destroy(dev_class,dev);
        class_destroy(dev_class);
        cdev_del(&etx_cdev);
        mutex_lock(&kernel_logger.etx_mutex);
        mutex_unlock(&kernel_logger.etx_mutex);
        unregister_chrdev_region(dev, 1);
        pr_info("Device Driver Remove...Done!!!\n");
}
 
module_init(etx_driver_init);
module_exit(etx_driver_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sanath Sapre");
MODULE_DESCRIPTION("Kernel Logger");
MODULE_VERSION("1.4");