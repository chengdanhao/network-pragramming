#! /usr/bin/python3
# -*- coding: utf-8 -*-

import socket

HOST = ''
PORT = 10001
MAX_CONNECT = 10

messages = ["This is", "a message, it ", "will be send", "in parts."]

socks = []

def main():
    print('connect to the server...')

    for i in range(MAX_CONNECT):
        socks.append(socket.socket(socket.AF_INET, socket.SOCK_STREAM))

    for s in socks:
        s.connect((HOST, PORT))

    count = 0
    for msg in messages:
        for s in socks:
            count += 1
            data = '{0} from connection {1}'.format(msg, str(count))
            print('%s sending \'%s\'' % (s.getpeername(), data))
            s.send(data.encode())

        for s in socks:
            data = s.recv(1024).decode()
            # WRONG : print(s.getpeername() + " received " + data)
            # can only concatenate tuple (not "str") to tuple
            print('%s received \'%s\'' % (s.getpeername(), data))
            if not data:
                print('closing socket ', s.getpeername())
                s.close()

if __name__ == '__main__':
    main()