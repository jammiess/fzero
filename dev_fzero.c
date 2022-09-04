#include <linux/init.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/uaccess.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("James Johnson");
MODULE_DESCRIPTION("Like /dev/zero but for arbitrary data");
MODULE_VERSION("0.1");

static const char *DEVICE_NAME = "fzero";
static const char *CLASS_NAME = "fzero";

static int major_number;
static char file_data[256];
static unsigned int data_size;
static struct class *fzero_class = NULL;
static struct device *fzero_device = NULL;

static int fzero_open(struct inode *, struct file *);
static int fzero_release(struct inode *, struct file *);
static ssize_t fzero_read(struct file *, char *, size_t, loff_t *);
static ssize_t fzero_write(struct file *, const char *, size_t, loff_t *);

static const struct file_operations fzero_fops = {
  .owner = THIS_MODULE,
  .open = fzero_open,
  .read = fzero_read,
  .write = fzero_write,
  .release = fzero_release,
};

static int __init fzero_init(void) {
  printk(KERN_INFO "fzero: Initializing module\n");

  major_number = register_chrdev(0, DEVICE_NAME, &fzero_fops);
  if (major_number < 0) {
    printk(KERN_ALERT "fzero: Failed to register a major number\n");
    return major_number;
  }
  printk(KERN_INFO "fzero: registered major number %d\n", major_number);

  fzero_class = class_create(THIS_MODULE, CLASS_NAME);
  if (IS_ERR(fzero_class)) {
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_ALERT "fzero: Failed to register device class\n");
    return PTR_ERR(fzero_class);
  }
  printk(KERN_INFO "fzero: Registered device class\n");

  fzero_device = device_create(fzero_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
  if (IS_ERR(fzero_device)) {
    class_destroy(fzero_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_ALERT "fzero: Failed to create device\n");
    return PTR_ERR(fzero_device);
  }

  memset(file_data, 0xff, 256);
  data_size = 1;
  printk(KERN_INFO "fzero: Module successfully initialized\n");
  return 0;
}

static void __exit fzero_exit(void) {
  device_destroy(fzero_class, MKDEV(major_number, 0));
  class_unregister(fzero_class);
  class_destroy(fzero_class);
  unregister_chrdev(major_number, DEVICE_NAME);
  printk(KERN_INFO "fzero: Unregistered module\n");
}

static int fzero_open(struct inode *node, struct file *filp) {
  return 0;
}

static int fzero_release(struct inode *node, struct file *filp) {
  return 0;
}

static ssize_t fzero_read(struct file *filp, char *buffer, size_t len, loff_t *offset) {
  if (! access_ok(buffer, len)) {
    return -EFAULT;
  }
  if (data_size == 1) {
    char val = file_data[0];
    for (size_t i = 0; i < len; i++) {
      put_user(val, buffer + i);
    }
    *offset += len;
    return len;
  } else {
    loff_t off = *offset;
    for (size_t i = 0; i < len; i++) {
      loff_t curr_off = (off + i) % data_size;
      char val = file_data[curr_off];
      put_user(val, buffer + i);
    }
    *offset += len;
    return len;
  }
}

static ssize_t fzero_write(struct file *filp, const char *buffer, size_t len, loff_t *offset) {
  char val;
  if (len > 256) {
    return -EINVAL;
  }
  if (! access_ok(buffer, len)) {
    return -EFAULT;
  }
  if (len == 0) {
    return -EINVAL;
  }
  *offset = 0;
  get_user(val, buffer);
  if (len == 1 && val == 0) {
    memset(file_data, 0xff, 256);
    data_size = 1;
    return 1;
  } else {
    data_size = len;
    for (int i = 0; i < len; i++) {
      get_user(val, buffer + i);
      file_data[i] = val;
    }
    return len;
  }
}

module_init(fzero_init);
module_exit(fzero_exit);
