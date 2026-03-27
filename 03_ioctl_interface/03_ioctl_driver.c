/***************************************************************************//**
*  \file       01_msg_queue_driver.c
*
*  \details    Simple Linux device driver kernel buffer with ioctl support
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
 #include <linux/poll.h>
 #include <linux/version.h>

#define ROW_SIZE        16           //Row Size
#define MEM_SIZE        128           //Memory Size
#define DONE    1
 
/* IOCTL's */

/* magic number — pick a unique one for your driver */
#define ETX_MAGIC   'e'

/* GET_QUEUE_SIZE — driver returns size to user — direction is IOR */
#define GET_QUEUE_SIZE  _IOR(ETX_MAGIC, 1, int32_t)

/* GET_MAX_CAPACITY — driver returns max capacity to user — direction is IOR */
#define GET_MAX_CAPACITY  _IOR(ETX_MAGIC, 2, int32_t)

/* CLEAR_QUEUE command — no data transfer, just a trigger to clear the queue */
#define CLEAR_QUEUE        _IO(ETX_MAGIC, 3)

/* RESET_DEVICE command — no data transfer, just a trigger to clear 
* the queue and reset the device internal states */
#define RESET_DEVICE        _IO(ETX_MAGIC, 4)

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

typedef struct etx_priv {
    int read_done;    /* used by read path  */
    int write_done;   /* used by write path */
} etx_priv_t;

kernel_logger_t kernel_logger;
static DECLARE_WAIT_QUEUE_HEAD(etx_wait_queue);
int size_of_message = 0;

/*
** Function Prototypes
*/
static int      __init etx_driver_init(void);
static void     __exit etx_driver_exit(void);
static int      etx_open(struct inode *inode, struct file *file);
static int      etx_release(struct inode *inode, struct file *file);
static ssize_t  etx_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t  etx_write(struct file *filp, const char *buf, size_t len, loff_t * off);
static unsigned int etx_poll(struct file *filp, poll_table *wait);
static long     etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg);


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
        .poll           = etx_poll,
        .unlocked_ioctl = etx_ioctl,
};
 
/*
** This function is to configure permissions for the dev node
*/

/*
** ===== DEVNODE COMPATIBILITY (STRICT MATCHING) =====
*/

/* Kernel 6.x (const struct device *) */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)

static char *etx_devnode(const struct device *dev, umode_t *mode)
{
    if (mode)
        *mode = 0666;
    return NULL;
}

#else   /* Kernel 5.x and below */

/* Kernel 5.x (non-const struct device *) */
static char *etx_devnode(struct device *dev, umode_t *mode)
{
    if (mode)
        *mode = 0666;
    return NULL;
}

#endif

/*
** This function will be called when we write IOCTL on the Device file
*/
static long etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
         int32_t val = 0;
         int32_t loop_index = 0;

         switch(cmd) {
                case GET_QUEUE_SIZE:
                        
                        mutex_lock(&kernel_logger.etx_mutex);
                        val = kernel_logger.count;
                        mutex_unlock(&kernel_logger.etx_mutex);
                        if (copy_to_user((int32_t __user *)arg, &val, sizeof(val)))
                                return -EFAULT;
                        pr_info("ioctl GET_QUEUE_SIZE: %d\n", val);
                        break;

                case GET_MAX_CAPACITY:
                        
                        val = ROW_SIZE;
                        if (copy_to_user((int32_t __user *)arg, &val, sizeof(val)))
                                return -EFAULT;
                        pr_info("ioctl GET_MAX_CAPACITY: %d\n", val);
                        break;

                case CLEAR_QUEUE:

                        mutex_lock(&kernel_logger.etx_mutex);
                        for(loop_index=0; loop_index < ROW_SIZE; loop_index++)
                        {
                                memset(kernel_logger.kernel_buffer[loop_index], 0, MEM_SIZE);
                        }
                        kernel_logger.read_indexer = 0;
                        kernel_logger.write_indexer = 0;
                        kernel_logger.count = 0;
                        mutex_unlock(&kernel_logger.etx_mutex);
                        pr_info("ioctl CLEAR_QUEUE\n");
                        break;

                case RESET_DEVICE:

                        mutex_lock(&kernel_logger.etx_mutex);
                        for(loop_index=0; loop_index < ROW_SIZE; loop_index++)
                        {
                                memset(kernel_logger.kernel_buffer[loop_index], 0, MEM_SIZE);
                        }
                        kernel_logger.read_indexer = 0;
                        kernel_logger.write_indexer = 0;
                        kernel_logger.count = 0;
                        mutex_unlock(&kernel_logger.etx_mutex);
                        pr_info("ioctl RESET_DEVICE\n");
                        break;

                default:
                        pr_info("IOCTL Default case\n");
                        return -EINVAL;
        }
        return 0;
}

static __poll_t etx_poll(struct file *filp, poll_table *wait)
{
    static int dev_poll_called_count = 0 ;
    etx_priv_t *priv = filp->private_data;
    dev_poll_called_count ++;
    poll_wait(filp,&etx_wait_queue,wait);
    pr_info("poll_wait...!!! dev_poll_called_count = %d\n",dev_poll_called_count);

    if(kernel_logger.count > 0)
        return POLLIN | POLLRDNORM;

    if (priv->write_done == DONE ){
        pr_info("size_of_message > 0 returning POLLIN | POLLRDNORM\n");
        priv->write_done = !DONE;
        return POLLIN | POLLRDNORM;
    }
    else {
        pr_info("dev_poll return 0\n");
        return 0;
    }
}

/*
** This function will be called when we open the Device file
*/
static int etx_open(struct inode *inode, struct file *file)
{
        etx_priv_t *priv = kmalloc(sizeof(etx_priv_t), GFP_KERNEL);
        if (!priv)
            return -ENOMEM;

        priv->read_done  = 0;
        priv->write_done = 0;
        file->private_data = priv;

        pr_info("Device File Opened...!!!\n");
        return 0;
}

/*
** This function will be called when we close the Device file
*/
static int etx_release(struct inode *inode, struct file *file)
{
        kfree(file->private_data);
        pr_info("Device File Closed...!!!\n");
        return 0;
}

/*
** This function will be called when we read the Device file
*/
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
        ssize_t ret = 0;
        etx_priv_t *priv = filp->private_data;

        if(priv->read_done)
                return 0;

        //Copy the data from the kernel space to the user-space
        mutex_lock(&kernel_logger.etx_mutex);

        /* empty check now uses count, not index comparison */
        while (kernel_logger.count == 0)
        {
              pr_info("Data Read : Buffer empty\n");
              
              mutex_unlock(&kernel_logger.etx_mutex);
             
              /* Add non blocking support */
              if(filp->f_flags & O_NONBLOCK)
                   return -EAGAIN;

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
        priv->read_done = DONE;
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
        etx_priv_t *priv = filp->private_data;

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
        priv->write_done = DONE;
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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
        dev_class = class_create("sanath_class");
#else
        dev_class = class_create(THIS_MODULE, "sanath_class");
#endif
        if(IS_ERR(dev_class)){
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