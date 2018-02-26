# sllin_app
This is a LIN (Local Interconnect Network) with some presentation Layer suitable with Linux LIN kernel driver available at:

https://github.com/trainman419/linux-lin/tree/master/sllin

in order to be able to build the application, please follow these steps:
- mkdir build && cd build
- cmake ..
- make

Before running the application, follow steps available in the SLLIN driver README file:
- make driver
- load it at runtime (sudo insmod sllin.ko)
- slcan_attach (modified executable based on can-utils patch available at the driver link):
  sudo slcan_attach -w /dev/ttyS0
- Open a new Terminal and run:
sudo ip link set sllin0 up
sudo  ip link set sllin0 txqueuelen 1000 (To enable 1000 frames in TX queue)

- open a new terminal and run your application:
sudo sllin_app

- In order to be able to see periodically sent LIN frames, open a new terminal and run:
candump -t d sllin0

- In order to be able to see received frames, open a new Terminal and run:
cangen sllin0 -I 9 i -n 300 -3 -g 300 -v

Enjoy!!!
