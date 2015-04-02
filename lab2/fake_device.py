import os
import sys

inconnection = "/tmp/test_connection_dev"
outconnection = "/tmp/test_connection_driv"
try:
	os.mkfifo(inconnection)
	os.mkfifo(outconnection)
except OSError:
	pass
infifo = os.open(inconnection, os.O_RDONLY)
outfifo = os.open(outconnection, os.O_WRONLY)
disk = []
	
exits = False

def process_open_request():
	fifo.close()
	fifo = open(connection, "w")
	fifo.write("OPEN_ACK")
	fifo.close()
	print("Open Device Processed")
	fifo = open(connection, "r")
	
def process_release_request():
	fifo.close()
	fifo = open(connection, "w")
	fifo.write("REL_ACK")
	fifo.close()
	print("Close Device Processed")
	fifo = open(connection, "r")

def process_read_request():
	fifo.close()
	fifo = open(connection, "w")
	for i in disk:
		fifo.write(i)
	fifo.write("READ_COMP")
	fifo.close()
	print("Read Request Processed")
	fifo.open(connection, "r")

def process_write_request(fifo):
	fifo = open(connection, "r")
	for line in fifo:
		disk.append(line)
	fifo.close()
	print("disk contents:\n")
	for i in disk:
		print(i)
	fifo = open(connection, "r+")
	

while(exits == False):
	line = os.read(infifo, 52)
	"""if line == "release":
		process_release_request()
	elif line == "open":
		process_open_request()
	elif line == "read":
		process_read_request()"""
	if (True):
		print("WRITE REQUEST!")
		os.write(outfifo, bytes("WRITE_ACK", 'UTF-8'))
		print("Write Request Processed")
	else:
		infifo.close()
		exits = True
