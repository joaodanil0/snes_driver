#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/err.h>
#include <linux/gpio.h> 


#define GPIO_21 (533)

dev_t dev = 0;
static struct cdev snes_cdev;
static struct class *dev_class;
static ssize_t snes_write(struct file *filp, const char *buf, size_t len, loff_t * off);


static struct file_operations fops =
{
  .owner          = THIS_MODULE,
//  .read           = etx_read,
  .write          = snes_write,
//  .open           = etx_open,
//  .release        = etx_release,
};

static int __init snes_controller_init(void);
static void __exit snes_controller_exit(void);

static ssize_t snes_write(struct file *filp, const char *buf, size_t len, loff_t * off)
{
  uint8_t rec_buf[10] = {0};
  
  if( copy_from_user( rec_buf, buf, len ) > 0) {
    pr_err("ERROR: Not all the bytes have been copied from user\n");
  }
  
  pr_info("Write Function : GPIO_21 Set = %c\n", rec_buf[0]);
  
  if (rec_buf[0]=='1') {
    //set the GPIO value to HIGH
    gpio_set_value(GPIO_21, 1);
  } else if (rec_buf[0]=='0') {
    //set the GPIO value to LOW
    gpio_set_value(GPIO_21, 0);
  } else {
    pr_err("Unknown command : Please provide either 1 or 0 \n");
  }
  
  return len;
  
}

static int __init snes_controller_init(void)
{
  if((alloc_chrdev_region(&dev, 0, 1, "snes_Dev")) < 0 )
  {
    pr_err("Unable to allocate major number\n");
    unregister_chrdev_region(dev,1);
    return -1;
  }
  pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));
  
  cdev_init(&snes_cdev, &fops);

  if((cdev_add(&snes_cdev, dev, 1)) < 0)
  {
    pr_err("Unable to add device to the system\n");
    cdev_del(&snes_cdev);
    unregister_chrdev_region(dev,1);
    return -1;
  }  

  dev_class = class_create("snes_class"); 
  if(IS_ERR(dev_class))
  {
    pr_err("Unable to create struct class\n");
    class_destroy(dev_class);
    cdev_del(&snes_cdev);
    unregister_chrdev_region(dev,1);

    return -1;
  }
  
  if(IS_ERR(device_create(dev_class, NULL, dev, NULL, "snes_device")))
  {
    pr_err("Unable to create device\n");
    device_destroy(dev_class,dev);
    class_destroy(dev_class);
    cdev_del(&snes_cdev);
    unregister_chrdev_region(dev,1);

    return -1;
  }

  if(gpio_request(GPIO_21, "LED_LOL") < 0)
  {
    pr_err("ERROR: GPIO %d request\n", GPIO_21);
    gpio_free(GPIO_21);
    device_destroy(dev_class,dev);
    class_destroy(dev_class);
    cdev_del(&snes_cdev);
    unregister_chrdev_region(dev,1);
    return -1;
  }
  
  gpio_direction_output(GPIO_21, 0);
  
  gpiod_export(gpio_to_desc(GPIO_21), false);

  pr_info("Device Driver Initialized\n");


 
  return 0;
}

static void __exit snes_controller_exit(void)
{
  gpiod_unexport(gpio_to_desc(GPIO_21));
  gpio_free(GPIO_21);
  device_destroy(dev_class, dev);
  class_destroy(dev_class);
  cdev_del(&snes_cdev);
  unregister_chrdev_region(dev, 1);
  pr_info("Driver Removed\n");
}


module_init(snes_controller_init);
module_exit(snes_controller_exit);


MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver to manage snes controller");
MODULE_LICENSE("GPL");
