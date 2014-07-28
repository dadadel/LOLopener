import smtplib
import email

from lolopenstatus import LOLOpenStatus


class LOLOpenerMail(object):

    def __init__(self, open_status=LOLOpenStatus()):
        self.lolmsg = {True: """Le LOLcal est ouvert !
        Vous êtes donc les bienvenus

        Votre LOLOpener
        """,
                       False: """Le LOLcal est maintenant fermé !
        Désolé mais il faudra attendre qu'il réouvre avant de venir !

        Votre LOLOpener
        """}
        self.smtp_server = "localhost"
        self.from_addr = "lolopener@lolcal.lyonopenlab.net"
        self.to_addr =  ["lyon-hackerspace@lists.hackerspaces.org"]
        self.subject = {True: "Le LOLcal est ouvert",
                        False: "Le LOLcal est fermé"}
        self.open_status = open_status

    def run(self):
        running = True
        while running:
            if self.open_status.update():
                lol_open_status = self.open_status.is_open()
                msg = email.mime.text.MIMEText(self.lolmsg[lol_open_status])
                msg['From'] = self.from_addr
                msg['To'] = self.to_addr
                msg['Subject'] = self.subject[lol_open_status]
                s = smtplib.SMTP(self.smtp_server)
                s.sendmail(self.from_addr, self.to_addr, msg.as_string())
                s.quit()

