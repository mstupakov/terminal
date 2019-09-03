#   RS232:
1. List all serial interfaces with «ttyO*» prefix:
```
# ls -la /dev/ttyO*
crw-rw----    1 root     dialout   247,   0 Jan  1 00:00 /dev/ttyO0
crw-rw----    1 root     dialout   247,   9 Jan  1 00:00 /dev/ttyO9
```

2. Socket mapping;
```
 /dev/ttyO0 - X601;
 /dev/ttyO9 - X602;
```

3. Changing serial port speed and open the terminal:
```
# ./terminal
Usage:
 ./terminal -B 115200 /dev/ttyS0

# terminal -B 115200 /dev/ttyO0
```
