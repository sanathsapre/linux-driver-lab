/***************************************************************************//**
*  \file       01_msg_queue_driver.c
*
*  \details    Simple Linux device driver kernel timer driver with WorkQueue
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
#include <linux/stddef.h>
#include <linux/poll.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/err.h>
#include <linux/workqueue.h>            // Required for workqueues

//Timer Variable
#define TIMEOUT 1000    //milliseconds

#define ROW_SIZE        16           //Row Size
#define MEM_SIZE        128           //Memory Size
#define DONE            1
 
/* IOCTL's */

/* magic number — pick a unique one for your driver */
#define ETX_MAGIC   'g'

/* CLEAR_QUEUE command — no data transfer, just a trigger to start the timer*/
#define START_TIMER        _IO(ETX_MAGIC, 1)

/* RESET_DEVICE command — no data transfer, just a trigger to stop the timer */
#define STOP_TIMER        _IO(ETX_MAGIC, 2)

static struct timer_list etx_timer;
dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;

static DECLARE_WAIT_QUEUE_HEAD(etx_wait_queue);

typedef struct kernel_logger
{
    uint8_t kernel_buffer[ROW_SIZE][MEM_SIZE];
    uint8_t read_indexer;
    uint8_t write_indexer;
    uint8_t count;
    spinlock_t lock;
    unsigned long timer_count;
    int timer_active;    /* ← add this */
} kernel_logger_t;

static kernel_logger_t kernel_logger = 
{
        .read_indexer = 0,
        .write_indexer = 0,
        .count = 0,
        .timer_count = 0,
        .timer_active = 0,
};

/*
** Function Prototypes
*/
static int      __init etx_driver_init(void);
static void     __exit etx_driver_exit(void);
static int      etx_open(struct inode *inode, struct file *file);
static int      etx_release(struct inode *inode, struct file *file);
static ssize_t  etx_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static long     etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
void timer_callback(struct timer_list * data);

void workqueue_fn(struct work_struct *work); 
 
/*Creating work by Static Method */
DECLARE_WORK(workqueue,workqueue_fn);
 
/*Workqueue Function*/
void workqueue_fn(struct work_struct *work)
{
        /* declare flags variable at the top of each function */
        unsigned long flags;
        
        printk(KERN_INFO "Executing Workqueue Function\n");

        spin_lock_irqsave(&kernel_logger.lock, flags);
        snprintf(kernel_logger.kernel_buffer[kernel_logger.write_indexer], MEM_SIZE, "worker_event_%ld\n",kernel_logger.timer_count++);
        pr_info("%s\n", kernel_logger.kernel_buffer[kernel_logger.write_indexer]);
        kernel_logger.write_indexer = (kernel_logger.write_indexer + 1) % ROW_SIZE;  /* Increment index to point to the next slot */
        /* If buffer is not full, increment count.
        * If full, overwrite oldest entry by bumping read_indexer forward.
        */
        if (kernel_logger.count < ROW_SIZE)

                kernel_logger.count++;

        else
                /* full — oldest overwritten, bump read to stay valid */
                kernel_logger.read_indexer = (kernel_logger.read_indexer + 1) % ROW_SIZE;
        
        spin_unlock_irqrestore(&kernel_logger.lock, flags);
        wake_up_interruptible(&etx_wait_queue);
}

/*
** File Operations structure
*/
static struct file_operations fops =
{
        .owner          = THIS_MODULE,
        .read           = etx_read,
        .open           = etx_open,
        .release        = etx_release,
        .unlocked_ioctl = etx_ioctl,
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
** This function will be called when we write IOCTL on the Device file
*/
static long etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
        unsigned long flags;

         switch(cmd) {

                case START_TIMER:
                        pr_info("ioctl START_TIMER\n");

                        spin_lock_irqsave(&kernel_logger.lock, flags);
                        kernel_logger.timer_active = 1;    /* ← set before starting */
                        spin_unlock_irqrestore(&kernel_logger.lock, flags);

                        /* setup timer interval to based on TIMEOUT Macro */
                        mod_timer(&etx_timer, jiffies + msecs_to_jiffies(TIMEOUT));
                        break;

                case STOP_TIMER:
                        spin_lock_irqsave(&kernel_logger.lock, flags);
                        kernel_logger.timer_active = 0;    /* ← clear before stopping */
                        spin_unlock_irqrestore(&kernel_logger.lock, flags);    

                        pr_info("ioctl STOP_TIMER\n");
                        /* remove kernel timer when unloading module */
                        del_timer_sync(&etx_timer);
                        break;

                default:
                        pr_info("IOCTL Default case\n");
                        return -EINVAL;
        }
        return 0;
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
        /* declare flags variable at the top of each function */
        unsigned long flags;
        uint8_t tmp[MEM_SIZE];      /* local kernel buffer */
        size_t copy_len = 0;
        size_t msg_len = 0;

        int *done = filp->private_data;

        if(*done)
                return 0;

        spin_lock_irqsave(&kernel_logger.lock, flags);

        while (kernel_logger.count == 0) {
            spin_unlock_irqrestore(&kernel_logger.lock, flags);
            pr_info("Data Read : Buffer empty\n");

            if (wait_event_interruptible_exclusive(etx_wait_queue, kernel_logger.count > 0))
                return -ERESTARTSYS;
            spin_lock_irqsave(&kernel_logger.lock, flags);
        }
        
        /* compute lengths here — after lock acquired and count > 0 guaranteed */
        msg_len  = strnlen(kernel_logger.kernel_buffer[kernel_logger.read_indexer], MEM_SIZE);
        copy_len = min(len, msg_len);
        
        memcpy(tmp, kernel_logger.kernel_buffer[kernel_logger.read_indexer], copy_len);
        memset(kernel_logger.kernel_buffer[kernel_logger.read_indexer], 0, MEM_SIZE);
        kernel_logger.read_indexer = (kernel_logger.read_indexer + 1) % ROW_SIZE;
        kernel_logger.count--;
        
        spin_unlock_irqrestore(&kernel_logger.lock, flags);

        if (copy_to_user(buf, tmp, copy_len)) {
                pr_err("Data Read : Err!\n");
                return -EFAULT;
        }
        *done = DONE;

        return copy_len;
}

/*
** This function will be called when we write the Device file
*/
void timer_callback(struct timer_list * data)
{
        static unsigned long tim = 0;

        schedule_work(&workqueue);

        pr_info("timer callback called %ld\n", tim++);       
        /*
        Re-enable timer. Because this function will be called only first time. 
        If we re-enable this will work like periodic timer. 
        */
        /* only re-arm if timer is still active */
        if (kernel_logger.timer_active)
                mod_timer(&etx_timer, jiffies + msecs_to_jiffies(TIMEOUT));
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
        if(IS_ERR(device_create(dev_class,NULL,dev,NULL,"sanath_worker"))){
            pr_info("Cannot create the Device 1\n");
            goto r_device;
        }

        spin_lock_init(&kernel_logger.lock);
        
        /* setup your timer to call my_timer_callback */
        timer_setup(&etx_timer, timer_callback, 0);       //If you face some issues and using older kernel version, then you can try setup_timer API(Change Callback function's argument to unsingned long instead of struct timer_list *.
        
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
        /* remove kernel timer when unloading module */
        del_timer_sync(&etx_timer);
        flush_work(&workqueue);
        device_destroy(dev_class,dev);
        class_destroy(dev_class);
        cdev_del(&etx_cdev);
        unregister_chrdev_region(dev, 1);
        pr_info("Device Driver Remove...Done!!!\n");
}
 
module_init(etx_driver_init);
module_exit(etx_driver_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sanath Sapre");
MODULE_DESCRIPTION("Kernel Logger");
MODULE_VERSION("1.4");