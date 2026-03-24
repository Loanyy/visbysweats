import socket
import threading
import json

HOST = '0.0.0.0'
PORT = 5050

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind((HOST, PORT))
server.listen()

clients = [None, None]
lock = threading.Lock()

def recv_line(conn):
    data = b''
    while True:
        ch = conn.recv(1)
        if not ch:
            raise ConnectionError("disconnected")
        if ch == b'\n':
            return data.decode('utf-8')
        data += ch

def send_line(conn, msg):
    conn.sendall((msg + '\n').encode('utf-8'))

def handle_client(conn, player_id):
    print(f"[Player {player_id}] connected")
    while True:
        try:
            msg = recv_line(conn)
            other_id = 1 - player_id
            with lock:
                other = clients[other_id]
            if other:
                try:
                    send_line(other, msg)
                except:
                    pass
        except Exception as e:
            print(f"[Player {player_id}] disconnected")
            with lock:
                clients[player_id] = None
            conn.close()
            break

def start():
    print("Server is open and waiting players to join...")

    conn0, addr0 = server.accept()
    with lock:
        clients[0] = conn0
    print(f"[HOST] conectat: {addr0}")
    send_line(conn0, json.dumps({"player_id": 0}))

    conn1, addr1 = server.accept()
    with lock:
        clients[1] = conn1
    print(f"[JOINER] conectat: {addr1}")
    send_line(conn1, json.dumps({"player_id": 1}))

    print("Ambii conectati - START!")
    send_line(conn0, json.dumps({"status": "START"}))
    send_line(conn1, json.dumps({"status": "START"}))

    t0 = threading.Thread(target=handle_client, args=(conn0, 0), daemon=True)
    t1 = threading.Thread(target=handle_client, args=(conn1, 1), daemon=True)
    t0.start()
    t1.start()
    t0.join()
    t1.join()

start()