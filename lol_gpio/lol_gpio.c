/**
 * This Linux kernel module is developed for the LOLOpener project.
 * It provides a way from userland to manage the Raspberry PI BCM2835
 * GPIOs used for the project.
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
#include <linux/ioport.h>
#include <asm/io.h>

MODULE_AUTHOR ("A. Daouzli");
MODULE_LICENSE("GPL");

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

/* GLOBAL VARIABLES */

struct reg_gpio *mgpio = NULL;

/* FUNCTIONS PROTOTYPES */

int lol_setup_gpio(int gpio, int mode);
int lol_set_gpio(int gpio, int value);
int lol_get_gpio(int gpio);

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
 * Init module function
 **/
int lol_gpio_init(void)
{
    lol_alert("%s - version %s\n", LOL_DRIVER_NAME, LOL_DRIVER_VERSION);
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
    lol_setup_gpio(4, INPUT_MODE);
    lol_setup_gpio(7, OUTPUT_MODE);
    lol_setup_gpio(8, OUTPUT_MODE);
    lol_setup_gpio(9, OUTPUT_MODE);
    lol_setup_gpio(11, OUTPUT_MODE);
    lol_set_gpio(7, 1);
    lol_set_gpio(9, 1);
    lol_alert("work is done!\n");
    return 0;
}

void lol_gpio_exit(void)
{
    lol_set_gpio(7, 0);
    lol_set_gpio(9, 0);
    iounmap(mgpio);
    release_mem_region(GPIO_BASE_ADDRESS, sizeof(struct reg_gpio));
    lol_alert("Module released!\n");
}

module_init(lol_gpio_init);
module_exit(lol_gpio_exit);

