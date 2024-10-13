#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/err.h>
#include <linux/gpio.h> 
#include <linux/hrtimer.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/interrupt.h>


// To find the correct gpio label, check /sys/kernel/debug/gpio
#define GPIO_21 (533) // Clock pin
#define GPIO_20 (532) // Latch pin
#define GPIO_16 (528) // Data pin

int size_buttons = 9;
int data_sent_counter = 0;

bool data[9] = {0};
u64 start_t;
u64 start2_t;

static struct hrtimer clk;
static struct hrtimer latch;

static int __init snes_controller_init(void);
static void __exit snes_controller_exit(void);


static enum hrtimer_restart clk_handler(struct hrtimer *timer) {

  hrtimer_forward(timer,hrtimer_cb_get_time(timer),ktime_set(0, ns_to_ktime(1.6*1000*1000)));
  gpio_set_value(GPIO_21, 1);
  udelay(20);
  gpio_set_value(GPIO_21, 0);
  
  if(data_sent_counter < size_buttons){
    int rin = gpio_get_value(GPIO_16);
    // pr_info("Read value[%d]: %d\n", data_sent_counter, rin);
    data[data_sent_counter] = rin;
    //pr_info("data[%d] = %d\n", data_sent_counter, data[data_sent_counter]);
    data_sent_counter++;    
  }
  else{
    if(data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 0 && data[4] == 0 && data[5] == 0 && data[6] == 0 && data[7] == 0 && data[8] == 0){
    }
    else{
      pr_info("Data received: %d %d %d %d %d %d %d %d %d\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8]);
      //input_report_key(button_dev, BTN_0, 1);
      //input_sync(button_dev);
    }
  }     

  return HRTIMER_RESTART;

}

static enum hrtimer_restart latch_handler(struct hrtimer *timer) {

  pr_info("Activate latch pulse\n");
  hrtimer_forward(timer,hrtimer_cb_get_time(timer),ktime_set(0,ms_to_ktime(16)));
  gpio_set_value(GPIO_20, 1);
  udelay(100);
  gpio_set_value(GPIO_20, 0);
  data_sent_counter = 0;
  return HRTIMER_RESTART;
}
static int __init snes_controller_init(void)
{
  
  if(gpio_request(GPIO_21, "LED_LOL") < 0)
  {
    pr_err("ERROR: GPIO %d request\n", GPIO_21);
    gpio_free(GPIO_21);
    return -1;
  }

  if(gpio_request(GPIO_20, "SNES_LATCH") < 0)
  {
    pr_err("ERROR: GPIO %d request\n", GPIO_20);
    gpio_free(GPIO_20);
    return -1;
  }
  
  if(gpio_request(GPIO_16, "SNES_DATA") < 0)
  {
    pr_err("ERROR: GPIO %d request\n", GPIO_16);
    gpio_free(GPIO_16);
    return -1;
  }
  
  gpio_direction_output(GPIO_21, 0);
  gpio_direction_output(GPIO_20, 0);
  gpio_direction_input(GPIO_16);

  gpiod_export(gpio_to_desc(GPIO_16), false);
  gpiod_export(gpio_to_desc(GPIO_21), false);
  gpiod_export(gpio_to_desc(GPIO_20), false);

  pr_info("Device Driver Initialized\n");

  hrtimer_init(&clk, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  clk.function = &clk_handler;
  start_t = jiffies;
  hrtimer_start(&clk, ms_to_ktime(100), HRTIMER_MODE_REL);
 
  hrtimer_init(&latch, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  latch.function = &latch_handler;
  start2_t = jiffies;
  hrtimer_start(&latch, ms_to_ktime(50), HRTIMER_MODE_REL); 
   
  pr_info("Timer Initialized");
  return 0;
}

static void __exit snes_controller_exit(void)
{
  gpiod_unexport(gpio_to_desc(GPIO_21));
  gpiod_unexport(gpio_to_desc(GPIO_20));
  gpiod_unexport(gpio_to_desc(GPIO_16));
  gpio_free(GPIO_21);
  gpio_free(GPIO_20);
  gpio_free(GPIO_16);
  hrtimer_cancel(&clk);
  hrtimer_cancel(&latch);
  pr_info("Driver Removed\n");
  
}


module_init(snes_controller_init);
module_exit(snes_controller_exit);


MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver to manage snes controller");
MODULE_LICENSE("GPL");
