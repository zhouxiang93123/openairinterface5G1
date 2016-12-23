import os, subprocess, time, fcntl, termios, tty

from utils import log

class connection:
    def __init__(self, description, host, user, password):
        self.description = description
        self.host = host
        self.user = user
        self.password = password

        try:
            (pid, fd) = os.forkpty()
        except BaseException, e:
            log("ERROR: forkpty for '" + description + "': " + e)
            (pid, fd) = (-1, -1)

        if pid == -1:
            log("ERROR: creating connection for '" + description + "'")
            os._exit(1)

        # child process, run ssh
        if pid == 0:
            try:
                os.execvp('sshpass', ['sshpass', '-p', password,
                          'ssh', user + '@' + host])
            except BaseException, e:
                log("ERROR: execvp for '" + description + "': " + e)
            log("ERROR: execvp failed for '" + description + "'")
            os._exit(1)

        # parent process
        # make the TTY raw to avoid getting input printed and no ^M
        try:
            tty.setraw(fd, termios.TCSANOW)
        except BaseException, e:
            log("ERROR: failed configuring TTY: " + str(e))
            os._exit(1)

#        try:
#            fcntl.fcntl(fd, fcntl.F_SETFL,
#                        fcntl.fcntl(fd, fcntl.F_GETFL) | os.O_NONBLOCK)
#        except:
#            log("ERROR: fcntl failed for '" + description + "'")
#            os._exit(1)

        self.pid = pid
        self.fd = fd
        self.active = True

    def send(self, string):
        if self.active == False:
            log("ERROR: send: child is dead for '" + self.description + "'")

        try:
            (pid, out) = os.waitpid(self.pid, os.WNOHANG)
        except BaseException, e:
            log("ERROR: waitpid failed for '" + self.description + "'")
            log(str(e))
            (pid, out) = (self.pid, 0)
        if pid != 0:
            log("ERROR: child process dead for '" + self.description + "'")
            try:
                os.close(self.fd)
            except BaseException, e:
                log("ERROR: close failed for '" + self.description + "'")
                log(str(e))
            self.active = False
            return -1

        length = len(string)
        while length != 0:
            try:
                ret = os.write(self.fd, string)
            except BaseException, e:
                log("ERROR: send fails for '" + self.description + "'")
                os._exit(1)

            if ret == 0:
                log("ERROR: write returns 0 for '" + self.description + "'")
                os._exit(1)

            length = length - ret
            string = string[ret:]