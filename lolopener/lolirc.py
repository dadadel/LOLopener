from pyircibot import PyIrciBot
from lolopenstatus import LOLOpenStatus


class LolOpenerIrcBot(object):
    '''LOLOpener IRC bot. This class should be provided
    to PyIrciBot (https://github.com/dadadel/pyircibot)
    '''

    def __init__(self, nick=None, channel=None, open_status=LOLOpenStatus()):
        '''Init the data

        '''
        self.channel = channel
        self.nick = nick
        self.open_status = open_status
        self.is_open = open_status.is_open()

    def set_nick(self, nick):
        '''Sets the nick. This will be called by PyIrciBot to update the nickname.

        @param nick: the nick name
        
        '''
        self.nick = nick

    def set_channel(self, channel):
        '''Sets the channel. This will be called by PyIrciBot to update the channel.

        @param channel: the channel name
        
        '''
        self.channel = channel

    def asked_if_open(self, message):
        return 'ouvert ?' in message or 'ouvert?' in message or message.strip() == '?'

    def open_message(self):
        return 'Le LOLcal {}'.format('est ouvert !' if self.is_open else "est fermé !")

    def timeout_function(self):
        '''Check the GPIO status to detect Opening change, so inform the chan.
        '''
        if self.open_status.update():
            self.is_open = self.open_status.is_open()
            return {'cmd': {'message': self.open_message()}}
        return None

    def parse_message(self, message, source, target):
        '''Proceeds a message received with PRIVMSG.
        Will react if the following are found in the message:
            -'!bot: arrete toi stp': will send stop request to the bot
            -'?' || 'ouvert ?' || 'ouvert?': send to the channel a message saying if the LOLcal is open or not

        @param message: the message
        @param source: the sender of the message
        @param target: the target of the message (might be the channel or the bot)
        @return: None or dictionary

        '''
        if not target.startswith('#'):
            # Private message
            if 'bot: arrete toi stp' in message:
                print ("je m'arrete")
                return {'cmd': {'end': '', 'message': "On me demande de m'arreter. Au revoir..."}}
            if self.asked_if_open(message):
                return {'cmd': {'message': self.open_message()}}
        else:
            # Channel message
            if message.startswith(self.nick) and not message.replace(self.nick, '')[0].isalnum():
                if self.asked_if_open(message.replace(self.nick, '')[1:]):
                    return {'cmd': {'message': self.open_message()}}
            else:
                pass
 
if __name__ == '__main__':
    server = "irc.lyonopenlab.net"
    channel = "#testlol"
    botnick = "lolopener"
    bot = PyIrciBot(server, channel, botnick)
    bot.connect(timeout_use_class=True)
    bot.use_parser_class(LolOpenerIrcBot)
    bot.run()#parse_message=parse_message)
