#!/usr/bin/env python

from threading import Thread, Lock
from lolopener.pyircibot import PyIrciBot
from lolopener.lolopenstatus import LOLOpenStatus
from lolopener.lolmail import LOLOpenerMail
from lolopener.lolirc import LOLOpenerIrcBot


def set_led(status):
    with open("/sys/class/lol_gpio/gpio11", "w") as f: #red
        if status:
            f.write("0")
        else:
            f.write("1")
    with open("/sys/class/lol_gpio/gpio8", "w") as f: #green
        if status:
            f.write("1")
        else:
            f.write("0")


def launch_ircbot(lolstatus):
    server = "irc.lyonopenlab.net"
    channel = "#testlol"
    botnick = "lolopener"
    bot = PyIrciBot(server, channel, botnick)
    bot.connect(timeout_use_class=True)
    bot.use_parser_class(LOLOpenerIrcBot, open_status=lolstatus)
    bot.run()


def launch_mailer(lolstatus):
    global end_of_running
    mailer = LOLOpenerMail(lolstatus)
    mailer.run()
    end_of_running = True


def launch_leds(lolstatus):
    running = True
    status = lolstatus.is_open()
    set_led(status)
    while running:
        status = lolstatus.wait_change()
        set_led(status)
        if end_of_running:
            running = False

## MAIN FUNCTION

end_of_running = False

apps = [{'launch': launch_leds, 'thread': None},
        {'launch': launch_ircbot, 'thread': None},
        {'launch': launch_mailer, 'thread': None},
       ]

#lolstatus = LOLOpenStatus(lock=Lock())
for app in apps:
    lolstatus = LOLOpenStatus()
    app['thread'] = Thread(target=app['launch'], args=(lolstatus,))

for app in apps:
    if not app['thread']:
        break
    app['thread'].start()

for app in apps:
    if not app['thread']:
        break
    app['thread'].join()

