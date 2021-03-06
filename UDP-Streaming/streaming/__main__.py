from time import sleep, time
import cv2
import socket
import math
import pickle
import sys
import imutils
from core.mqtt import Mqtt
from threading import Thread

max_length = 65000
host = "127.0.0.1"
port = 5000
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.connect(('192.168.254.254',1))
ip = s.getsockname()[0]
mqtt = None

def start_stream():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    cap = cv2.VideoCapture(0)
    ret, frame = cap.read()
    print('stream started')
    while ret:
        # compress frame
        retval, buffer = cv2.imencode(".jpg", frame)
        if retval:
            # convert to byte array
            buffer = buffer.tobytes()
            # get size of the frame
            buffer_size = len(buffer)

            num_of_packs = 1
            if buffer_size > max_length:
                num_of_packs = math.ceil(buffer_size/max_length)

            frame_info = {"packs":num_of_packs}
            sock.sendto(pickle.dumps(frame_info), (host, port))
            
            left = 0
            right = max_length

            for i in range(num_of_packs):
                # truncate data to send
                data = buffer[left:right]
                left = right
                right += max_length

                # send the frames accordingly
                sock.sendto(data, (host, port))
        
        ret, frame = cap.read()
        frame = imutils.resize(frame, width=300)

def on_message(client, userdata, msg):
    payload = msg.payload.decode('utf-8')
    global host
    host = str(payload)
    client.publish('status','1')

def on_connect(client, userdata, flags, rc):
    try:
        print("Connected")
        client.subscribe('basestation')
        client.publish('rama/video',str(ip))
    except:
        print('gagal')
        pass

def listen():
    global mqtt
    mqtt = Mqtt("192.168.254.100", on_message, on_connect)

def publiship():
    while True:
        try:
            mqtt.publish('rama/video',str(ip))
            sleep(1)
        except:
            pass

a = Thread(target=listen, args=(())).start()
b = Thread(target=start_stream, args=(())).start()
c = Thread(target=publiship, args=(())).start()
