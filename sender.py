import socket
import time
import os

MCAST_GRP = '239.0.0.1'
MCAST_PORT_A = 5007
MCAST_PORT_B = 5008
MULTICAST_TTL = 2

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, MULTICAST_TTL)

try:
    with open('test_data.bin', 'rb') as f:
        data = f.read()
    print(f"Read {len(data)} bytes from test_data.bin.")
    print(f"Broadcasting to Port A ({
          MCAST_PORT_A}) and Port B ({MCAST_PORT_B})...")
except FileNotFoundError:
    print("Error: test_data.bin not found.")
    exit()

packet_counter = 0
while True:
    try:
        packet_counter += 1

        sock.sendto(data, (MCAST_GRP, MCAST_PORT_B))

        if packet_counter % 5 != 0:
            sock.sendto(data, (MCAST_GRP, MCAST_PORT_A))
            print(
                f"-> Sent packets to both feeds. (Count: {packet_counter}) ", end='\r')
        else:
            print(f"!!! DROPPED packet for Feed A to create a gap (Count: {
                  packet_counter}) !!!")

        time.sleep(1)
    except KeyboardInterrupt:
        print("\nStopping sender.")
        break
