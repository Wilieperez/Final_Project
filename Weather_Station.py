#! /usr/bin/python

import socket
import struct
import time
import random

def main():
    HOST = "127.0.0.112"
    #HOST = "192.168.43.207"
    PORT = 5003

    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((HOST, PORT))

    header = 0x10
    remLen = 0x0007
    protNL = 0x0004
    protNa = "MQTT"
    protNa = protNa.encode()
    vers = 0x02
    ConnFl = 0x04
    kA = 0x003C
    ClIDLe = 0x0007
    ClieID = ".....Weather Report"
    ClieID = ClieID.encode()

    frame =  struct.pack('>3B4s4B20s',header,remLen,protNL,protNa,vers,ConnFl,kA,ClIDLe,ClieID)
    client_socket.send(frame)
    data = client_socket.recv(1024).decode
    print(data)

    header = 0x30
    remLen = 0x02

    p_header = 0xC0
    p_code = 0x00
    p_frame = struct.pack('>2B',p_header,p_code)

    while 1:
        client_socket.send(p_frame)
        data = client_socket.recv(1024).decode
        print("ping")
        time.sleep(2)

        topic = 0x00
        weather = random.randint(15,30) #temperature
        msg = "Temperature: "
        msg += str(weather)
        msg_bytes = msg.encode()
        frame = struct.pack('>BBB49s',header,remLen,topic,msg_bytes)
        client_socket.send(frame)
        data = client_socket.recv(1024).decode()
        time.sleep(2)

        topic = 0x01
        weather = random.randint(10,90) #chance of rain %
        msg = "Chance of rain: "
        msg += str(weather)
        msg += "%"
        msg_bytes = msg.encode()
        frame = struct.pack('>BBB49s',header,remLen,topic,msg_bytes)
        client_socket.send(frame)
        data = client_socket.recv(1024).decode()
        time.sleep(2)

        topic = 0x02
        weather = random.randint(16,27) #humidity
        msg = "Humidity: "
        msg += str(weather)
        msg += "%"
        msg_bytes = msg.encode()
        frame = struct.pack('>BBB49s',header,remLen,topic,msg_bytes)
        client_socket.send(frame)
        data = client_socket.recv(1024).decode()
        time.sleep(2)

        topic = 0x03
        weather = random.randint(1,10) #UV Index
        msg = "UV Index: "
        msg += str(weather)
        if weather == 1 or weather == 2:
            msg += ", no UV protection needed"
            msg_bytes = msg.encode()
            frame = struct.pack('>BBB49s',header,remLen,topic,msg_bytes)
        elif weather > 2 and weather < 8:
            msg += ", UV protection needed"
            msg_bytes = msg.encode()
            frame = struct.pack('>BBB49s',header,remLen,topic,msg_bytes)
        elif weather > 7:
            msg += ", avoid going outside"
            msg_bytes = msg.encode()
            frame = struct.pack('>BBB49s',header,remLen,topic,msg_bytes)
        client_socket.send(frame)
        data = client_socket.recv(1024).decode()
        time.sleep(2)

        topic = 0x04
        weather = random.randint(0,300) # Air Quality
        msg = "Air Quality: "
        msg += str(weather)
        if weather >=0 and weather <= 50:
            msg += ", good"
            msg_bytes = msg.encode()
            frame = struct.pack('>BBB49s',header,remLen,topic,msg_bytes)
        elif weather > 50 and weather <= 100:
            msg += ", moderate"
            msg_bytes = msg.encode()
            frame = struct.pack('>BBB49s',header,remLen,topic,msg_bytes)
        elif weather > 100 and weather <= 150:
            msg += ", unhealthy for sensitive groups"
            msg_bytes = msg.encode()
            frame = struct.pack('>BBB49s',header,remLen,topic,msg_bytes)
        elif weather > 150 and weather <= 200:
            msg += ", unhealthy"
            msg_bytes = msg.encode()
            frame = struct.pack('>BBB49s',header,remLen,topic,msg_bytes)
        elif weather > 200:
            msg += ", very unhealthy"
            msg_bytes = msg.encode()
            frame = struct.pack('>BBB49s',header,remLen,topic,msg_bytes)
        client_socket.send(frame)
        data = client_socket.recv(1024).decode()
        time.sleep(2)

    client_socket.close()

main()