target remote tcp:localhost:2331

set breakpoint pending on

monitor reset halt

jump Reset_Handler
