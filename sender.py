import socket
import time
import os

MCAST_GRP = '239.0.0.1'
MCAST_PORT_A = 5007  # Feed A
MCAST_PORT_B = 5008  # Feed B
MULTICAST_TTL = 2

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, MULTICAST_TTL)

# Read the entire binary test file
try:
    with open('test_data.bin', 'rb') as f:
        data = f.read()
    print(f"Successfully read {len(data)} bytes from test_data.bin.")
    print(f"Broadcasting to Port A ({
          MCAST_PORT_A}) and Port B ({MCAST_PORT_B})...")
except FileNotFoundError:
    print("Error: test_data.bin not found.")
    exit()

# Loop forever, sending the same data to both ports
while True:
    try:
        sock.sendto(data, (MCAST_GRP, MCAST_PORT_A))
        sock.sendto(data, (MCAST_GRP, MCAST_PORT_B))
        print(f"-> Sent packets to both feeds.", end='\r')
        time.sleep(1)
    except KeyboardInterrupt:
        print("\nStopping sender.")
        break
