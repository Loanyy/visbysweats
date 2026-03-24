import socket
import threading
import json

HOST = '0.0.0.0'
PORT = 5050

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

def handle_client(conn, player_id, clients, lock):
    print(f"[Player {player_id}] connected")
    try:
        while True:
            msg = recv_line(conn)
            other_id = 1 - player_id
            with lock:
                other = clients[other_id]
            if other:
                try:
                    send_line(other, msg)
                except:
                    pass
    except:
        print(f"[Player {player_id}] disconnected")
        with lock:
            clients[player_id] = None
        conn.close()

def run_match(server):
    import socket as sock_mod
    clients = [None, None]
    lock = threading.Lock()

    print("\n--- Waiting for players ---")
    conn0, addr0 = server.accept()
    conn0.setsockopt(sock_mod.IPPROTO_TCP, sock_mod.TCP_NODELAY, 1)
    with lock:
        clients[0] = conn0
    print(f"[HOST] connected: {addr0}")
    send_line(conn0, json.dumps({"player_id": 0}))

    conn1, addr1 = server.accept()
    conn1.setsockopt(sock_mod.IPPROTO_TCP, sock_mod.TCP_NODELAY, 1)
    with lock:
        clients[1] = conn1
    print(f"[JOINER] connected: {addr1}")
    send_line(conn1, json.dumps({"player_id": 1}))

    print("Both connected - START!")
    send_line(conn0, json.dumps({"status": "START"}))
    send_line(conn1, json.dumps({"status": "START"}))

    t0 = threading.Thread(target=handle_client, args=(conn0, 0, clients, lock), daemon=True)
    t1 = threading.Thread(target=handle_client, args=(conn1, 1, clients, lock), daemon=True)
    t0.start()
    t1.start()
    t0.join()
    t1.join()

def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen()
    print("ASTEROID 3D Server running on port 5050")
    while True:
        try:
            run_match(server)
            print("Match ended. Ready for next match...")
        except Exception as e:
            print(f"Error: {e}")

main()