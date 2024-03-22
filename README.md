# tcp_filetransfer
simple client-server to do file transfer

## Compile
$ clone https://github.com/s887432/tcp_filetransfer.git<br>
$ cd tcp_filetransfer<br>
$ mkdir build<br>
$ cd build<br>
$ cmake ..<br>
$ make<br>

modify CMakeLists.txt to identify compiler

## Usage
prepare 10 files from 3000.jpg to 3010.jpg in same folder

<b>server usage: server PORT_NUM</b><br>
$ server 1812

<b>client usage: client SERVER_IP PORT_NUM SECTION_SIZE</b><br>
$ client 192.160.1.100 1812 1

Patrick Lin @ Taipei, Taiwan
