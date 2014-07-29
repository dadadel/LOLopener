class LOLOpenStatus(object):
    """LOLOpenStatus provides methods to know if the open status has
    changed, wait for a change, get the open status:

        -`has_changed`: to know if the status changed (bool)
        -`wait_change`: wait until the status changes and return the status (bool)
        -`get_open_status`: get the status open/close (bool) (won't update the 
            internal status)
        -`update`: update the internal status with the open status and inform
            if changed (bool)
        -`is_open`: get the internal open status (bool) (call update before)

    """
    def __init__(self, gpio_file="/sys/class/lol_gpio/gpio4", lock=None):
        self.lock = lock
        self._gpio_status_file = gpio_file
        self._is_open = self.get_open_status()

    def _lock(self):
        if self.lock:
            self.lock.acquire()

    def _unlock(self):
        if self.lock:
            self.lock.release()

    def get_open_status(self):
        """Get the open status.

        :return: True if it is open (gpio == 0), False if closed,
        None if failed to read the GPIO
        """
        is_open = None
        self._lock()
        with open(self._gpio_status_file, "r") as f:
            gpio = int(f.read())
            # Note that the status is inverted: 0 open/1 close
            if gpio == 1:
                is_open = False
            elif gpio == 0:
                is_open = True
        self._unlock()
        return is_open

    def has_changed(self):
        status = self.get_open_status()
        if status is None:
            return None
        if not status and self._is_open or status and not self._is_open:
            changed = True
        else:
            changed = False

    def wait_change(self):
        """Wait until the open status change and return it.

        :return: return the new open status

        """
        while not self.update():
            pass
        return self._is_open

    def is_open(self):
        """Return the open status"""
        return self._is_open

    def update(self):
        """Update the open status if changed.

        :return: True if the new status is different than the previous,
        False if the status has not changed, else None if failed to get status.

        """
        changed = False
        status = self.get_open_status()
        if status is None:
            return None
        self._lock()
        if not status and self._is_open:
            changed = True
            self._is_open = False
        elif status and not self._is_open:
            changed = True
            self._is_open = True
        self._unlock()
        return changed

