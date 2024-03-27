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
prepare images and file list in same folder<br>
put EOF in the last line of list file.<br>
example:<br>
3000.jpg<br>
3001.jpg<br>
EOF<br>

<b>server usage: server PORT_NUM</b><br>
$ server 1812

<b>client usage: client SERVER_IP PORT_NUM SECTION_SIZE LIST_FILE</b><br>
$ client 192.160.1.100 1812 30 lists.txt

Patrick Lin @ Taipei, Taiwan
