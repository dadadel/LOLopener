/**
 * This Linux kernel module is developed for the LOLOpener project.
 * It provides a way from userland to manage the Raspberry PI BCM2835
 * GPIOs used for the project.
 * It will export GPIOs 8/11 in output mode to light a red/green LED.
 * It will export GPIO4 in input mode to manage the open/close swith.
 *
 * This software is distributed under the GPLv3 (http://www.gnu.org/licenses/gpl.txt)
 * and comes without any warranty.
 *
 * A. Daouzli - LyonOpenLab - (c) 2014
 *
 **/

/* INCLUDES */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <asm/io.h>

/* CONSTANTS */

#define LOL_DRIVER_VERSION "0.0.1"
#define LOL_DRIVER_NAME "LOLOpener GPIO"

#define GPIO_BASE_ADDRESS  0x20200000 

#define MASK_MODE   0x7
#define INPUT_MODE  0x0
#define OUTPUT_MODE 0x1

/* MACROS DEFINITIONS */

#define GPF_IDX(g)  (g / 10)      /* get the GPFSEL register number for the GPIO */
#define GPF_OFFS(g) (3 * (g - (10 * GPF_IDX(g)))) /* get the bit offset of the GPIO */

#define GPSCL_IDX(g)  (g / 32)
#define GPSCL_OFFS(g) (g % 32)

#define lol_alert(__MSG, ...)  pr_alert("%s: " __MSG, LOL_DRIVER_NAME, ## __VA_ARGS__) 
#define lol_err(__MSG, ...)  pr_err("%s - l.%d: " __MSG, LOL_DRIVER_NAME, __LINE__, ## __VA_ARGS__) 

/* FUNCTIONS PROTOTYPES */

int lol_setup_gpio(int gpio, int mode);
int lol_set_gpio(int gpio, int value);
int lol_get_gpio(int gpio);

static ssize_t lol_sysfs_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count);
static ssize_t lol_sysfs_show(struct class *cls, struct class_attribute *attr, char *buf);

/* TYPES DEFINITIONS */

struct reg_gpio 
{
    unsigned int gpfsel[6]; /* GPIO function select (set the operation mode) */
    unsigned int reserved1;
    unsigned int gpset[2]; /* GPIO pin output set (set the value to 1) */
    unsigned int reserved2;
    unsigned int gpclr[2]; /* GPIO pin output clear (set the value to 0) */
    unsigned int reserved3;
    unsigned int gplev[2]; /* GPIO pin output level (get the value)*/
    unsigned int reserved4;
    unsigned int gpeds[2]; /* GPIO pin event detect status */
    unsigned int reserved5;
    unsigned int gpren[2]; /* GPIO pin rising edge detect enable */
    unsigned int reserved6;
    unsigned int gpfen[2]; /* GPIO pin falling edge detect enable */
    unsigned int reserved7;
}; 

static struct class_attribute lol_sysfs_class_attrs[] = {
    __ATTR(gpio4, 0666, lol_sysfs_show, lol_sysfs_store),
    __ATTR(gpio8, 0666, lol_sysfs_show, lol_sysfs_store),
    __ATTR(gpio11, 0666, lol_sysfs_show, lol_sysfs_store),
    __ATTR_NULL
};

struct class lol_class = {
    .name        = "lol_gpio",
    .owner       = THIS_MODULE,
    .class_attrs = lol_sysfs_class_attrs,
};

/* GLOBAL VARIABLES */

struct reg_gpio *mgpio = NULL;

/* MODULE CODE */

/**
 * Setup the GPIO in mode input or output
 *
 * \param[in] gpio the GPIO number
 * \param[in] mode the mode to set (0 for input, 1 for output)
 * \retval 0 success
 * \retval -1 the mapping is not set
 * \retval -2 bad mode
 **/
int lol_setup_gpio(int gpio, int mode)
{
    int ret = 0;
    unsigned int tmp;
    if (mgpio == NULL)
    {
        ret = -1;
    }
    else if (mode != INPUT_MODE && mode != OUTPUT_MODE)
    {
        ret = -2;
    }
    else
    {
        tmp = mgpio->gpfsel[GPF_IDX(gpio)];
        tmp &= ~(unsigned int)(MASK_MODE << GPF_OFFS(gpio)); /* reset the 3 bits for the gpio */
        tmp |= (unsigned int)(mode << GPF_OFFS(gpio)); /* set the mode */
        mgpio->gpfsel[GPF_IDX(gpio)] = tmp;
    }
    return ret;
}

/**
 * Set the GPIO value
 *
 * \param[in] gpio the GPIO number
 * \param[in] the value to set (0 or 1) 
 * \retval 0 success
 * \retval -1 the mapping is not set
 * \retval -2 bad value to set
 **/
int lol_set_gpio(int gpio, int value)
{
    int ret = 0;
    if (mgpio == NULL)
    {
        ret = -1;
    }
    else if (value == 1)
    {
        mgpio->gpset[GPSCL_IDX(gpio)] = 1 << GPSCL_OFFS(gpio);
    }
    else if (value == 0)
    {
        mgpio->gpclr[GPSCL_IDX(gpio)] = 1 << GPSCL_OFFS(gpio);
    }
    else
    {
        ret = -2;
    }
    return ret;
}

/**
 * Get the GPIO value
 *
 * \retval 0 GPIO is cleared
 * \retval 1 GPIO is set
 * \retval -1 error
 **/
int lol_get_gpio(int gpio)
{
    int ret = 0;
    if (mgpio == NULL)
    {
        ret = -1;
    }
    else
    {
        ret = (mgpio->gplev[GPSCL_IDX(gpio)] & (1 << GPSCL_OFFS(gpio)))? 1 : 0;
    }
    return ret;
}

/**
 * User request to set a GPIO (write sysfs file).
 *
 * \param attr[in] corresponds to the attribute to set
 * \param buf[in] contains the value to set
 * \return GPIO value (0 or 1)
 * \retval -1 error
 **/
static ssize_t lol_sysfs_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
    long int value, gpio;
    int ret;
    ret = kstrtol(buf,10, &value);
    if (ret != 0)
    {
        lol_err("failed to extract number from '%s'\n", buf);
        return -1;
    }
    /* attr.name is expected to be format as "gpio%d" */
    ret = kstrtol(attr->attr.name + 4, 10, &gpio);
    if (ret != 0)
    {
        lol_err("failed to extract number from %s\n", attr->attr.name);
        return -1;
    }
    lol_alert("set GPIO %d to %d\n", (int)gpio, (int)value);
    lol_set_gpio((int)gpio, (int)value);
    return count;
}

/**
 * User request reading a GPIO value (read sysfs file).
 *
 * \param attr[in] corresponds to the attribute to get
 * \param buf[out] will contain the read value
 * \return GPIO value (0 or 1)
 * \retval -1 error
 **/
static ssize_t lol_sysfs_show(struct class *cls, struct class_attribute *attr, char *buf)
{
    int ret;
    long int gpio;
    /* attr.name is expected to be format as "gpio%d" */
    ret = kstrtol(attr->attr.name + 4, 10, &gpio);
    if (ret != 0)
    {   
        lol_err("failed to extract number from %s\n", attr->attr.name);
        return -1;
    }
    ret = lol_get_gpio((int)gpio);
    if (ret < 0)
    {
        lol_err("Failed to retrieve the GPIO value\n");
        return -1;
    }
    return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

/**
 * Init module function
 **/
int lol_gpio_init(void)
{
    lol_alert("%s - version %s\n", LOL_DRIVER_NAME, LOL_DRIVER_VERSION);

    /* Map the GPIOs */

    if (request_mem_region(GPIO_BASE_ADDRESS, sizeof(struct reg_gpio), LOL_DRIVER_NAME) == NULL)
    {
        lol_err("Failed to reserve the memory region\n");
        return -1;
    }
    mgpio = (struct reg_gpio*)ioremap(GPIO_BASE_ADDRESS, sizeof(struct reg_gpio));
    if (mgpio == NULL)
    {
        release_mem_region(GPIO_BASE_ADDRESS, sizeof(struct reg_gpio));
        lol_err("Failed to release the reserved memory region\n");
        return -1;
    }

    /* Configure the GPIOs directions */

    lol_setup_gpio(4, INPUT_MODE);
    lol_setup_gpio(8, OUTPUT_MODE);
    lol_setup_gpio(11, OUTPUT_MODE);

    /* initializes the SYSFS  */

    class_register(&lol_class);

    lol_alert("GPIOs ready to be used\n");

    return 0;
}

void lol_gpio_exit(void)
{
    class_unregister(&lol_class);
    iounmap(mgpio);
    release_mem_region(GPIO_BASE_ADDRESS, sizeof(struct reg_gpio));
    lol_alert("Module released!\n");
}

module_init(lol_gpio_init);
module_exit(lol_gpio_exit);

MODULE_AUTHOR ("A. Daouzli");
MODULE_LICENSE("GPL");

