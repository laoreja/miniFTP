# miniFTP
computer networks' lab. To implement the ftp protocol.

#How to compile
gcc server.c ../ftp_utils.c -o server
gcc client.c ../ftp_utils.c -o client

#How to use
./server
./client localhost
