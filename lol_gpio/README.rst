lol_gpio
########

This Linux kernel module allow non-root users to use the Rapsberry Pi GPIOs (in
input/output mode).

It maps the BCM2835 GPIOs registers and sets some specific GPIOs in input and
others in output. Then it provide a SYSFS interface files in ``/sys/class/lol_gpio/``
with a formated file name ``gpioX`` where ``X`` is the GPIO number.

Those files are accesible in read/write mode for all users (unlike
``/sys/class/gpio/`` files that are accessible in write mode only for root
users).

A file ``gpioX`` is a text file and should contains ``0`` or ``1`` representing
the GPIO status. The user can read it or write it, in the second case the GPIO
will be set.
