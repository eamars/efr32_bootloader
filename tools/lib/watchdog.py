from threading import Timer

class WatchdogCallbackObject:
    def __init__(self):
        pass

    def watchdog_handler(self, arg):
        raise Exception("Not implemented")

class Watchdog:
    def __init__(self, timeout, callbackObject):
        self.timeout = timeout
        self.callbackObject = callbackObject
        self.timer = None

    def start(self):
        self.timer = Timer(self.timeout, self.handler)
        self.timer.setDaemon(True)
        self.timer.start()

    def stop(self):
        if self.timer:
            self.timer.cancel()

    def reset(self):
        if self.timer:
            self.timer.cancel()
        self.timer = Timer(self.timeout, self.handler)
        self.timer.setDaemon(True)
        self.timer.start()

    def handler(self):
        self.timer.cancel()
        self.callbackObject.watchdog_handler(self)



if __name__ == "__main__":
    class ExampleClass(WatchdogCallbackObject):
        def __init__(self):
            self.watchdog = Watchdog(1, self)
            self.watchdog.start()

        def watchdog_handler(self, arg):
            print("Timeout")
            arg.reset()

    example = ExampleClass()

    while 1:
        continue
