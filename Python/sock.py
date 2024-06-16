import socket
import struct
import os

NLMSG_DONE = 0x3

sock = socket.socket(socket.AF_NETLINK, socket.SOCK_RAW, 31)
sock.bind((0, 0))
msg = "Hello from PYTHON"
#sock.send(b"Hello from PYTHON")
nl_hdr = struct.pack("IHHII", 16 + len(msg), 0, NLMSG_DONE, 0, os.getpid())
sock.send(nl_hdr + msg.encode())
while True:
	response = sock.recv(128)
	nl_hdr = response[:16]
	length, msg_type, flags, seq, pid = struct.unpack("IHHII", nl_hdr)
	msg = response[16:length].decode()
	print("Button status: ", ord(msg[0]))
