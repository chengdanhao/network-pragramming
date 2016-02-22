#! /usr/bin/python3
# -*- coding: utf-8 -*-

import socket
import select
import queue

HOST = ''
PORT = 10001
TIMEOUT = 20

inputs = []
outputs = []
msg_queue = {}

def main():
    master_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    master_socket.setblocking(False)
    master_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    master_socket.bind((HOST, PORT))
    master_socket.listen(10)

    inputs.append(master_socket)

    while inputs:
        print('-' * 50)
        print('waiting for next event')
        readable, writable, exceptional = select.select(inputs, outputs, inputs, TIMEOUT)

        # when timeout reached, select return 3 empty lists.
        if not (readable or writable or exceptional):
            print('timeout.')
            break

        for s in readable:
            if s is master_socket:
                conn, client_addr = s.accept()
                print('connection from', client_addr)
                conn.setblocking(False)
                inputs.append(conn)
                msg_queue[conn] = queue.Queue()
            else:
                data = s.recv(1024).decode()
                if data:
                    print('received \'' + data + '\' from ' + str(s.getpeername()))
                    msg_queue[s].put(data)

                    # add output channel for response
                    if s not in outputs:
                        outputs.append(s)
                else:
                    # interpret empty result as close connection
                    # 逗号后面会自动补空格
                    print("closing", s.getpeername())
                    if s in outputs:
                        outputs.remove(s)
                    inputs.remove(s)
                    s.close()
                    del msg_queue[s]

        for s in writable:
            try:
                msg = msg_queue[s].get_nowait()
            except queue.Empty:
                print(s.getpeername(), 'queue is empty.')
                outputs.remove(s)
            else:
                msg = msg.upper()
                print('sending \'' + msg + '\' to ' + str(s.getpeername()))
                s.send(msg.encode())

        for s in exceptional:
            print('exception condition on', s.getpeername())

            # stop listening inputs on the connection
            inputs.remove(s)
            if s in outputs:
                outputs.remove(s)
            s.close()
            del msg_queue[s]

if __name__ == '__main__':
    main()