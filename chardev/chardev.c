#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#define SUCCESS 0
#define DEVICE_NAME "chardev"
#define BUF_LEN 80

int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t*);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static int major;
static int is_device_open = 0;
static char msg[BUF_LEN];
static char *msg_ptr;

static struct file_operations fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release,
};

int init_module(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0)
    {
        printk(KERN_INFO "[chardev] init_module error registering device.\n");
        return major;
    }

    printk(KERN_INFO "[chardev] i was assigned major number %d\n", major);

    return SUCCESS;
}

void cleanup_module(void)
{
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "[chardev] device driver unregistered.\n");
}

static int device_open(struct inode *inode, struct file *filp)
{
    static int counter = 0;
    if (is_device_open)
        return -EBUSY;
    
    is_device_open++;
    sprintf(msg, "[chardev] I already told you %d times\n\n", counter++);
    msg_ptr = msg;
    try_module_get(THIS_MODULE);

    return SUCCESS;
}

static int device_release(struct inode *inode, struct file *filp)
{
    is_device_open--;
    module_put(THIS_MODULE);

    printk(KERN_INFO "[chardev] device_release.\n");
    
    return 0;
}

static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
    int bytes_read = 0;

    printk(KERN_INFO "[chardev] device_read.\n");

    if (*msg_ptr == 0) return 0;

    while (length && *msg_ptr)
    {
        put_user(*(msg_ptr++), buffer++);
        length--;
        bytes_read++;
    }

    return bytes_read;
}

static ssize_t device_write(struct file *filp, const char *buffer, size_t length, loff_t *offset)
{
    printk(KERN_INFO "[chardev] device_write operation not permitted.\n");
    return -EINVAL;
}