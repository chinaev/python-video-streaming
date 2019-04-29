import socket
import threading
from queue import Queue
from video_streaming import PyVideoDecoder
import numpy as np
import argparse
import cv2

BUFSIZE = 4 * 1024

class FrameReceiver:

    def __init__(self, port, width, height, codec="h264"):

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.bind(('0.0.0.0', port))
        self.socket = None

        self.frame_queue = Queue()
        
        self.decoder = PyVideoDecoder(width, height, codec.encode('utf-8'))

        self.image_receiving_thread = threading.Thread(target=FrameReceiver._receive_images, args=(self,))
        self.image_decoding_thread = threading.Thread(target=FrameReceiver._decode_images, args=(self,))
        
    def _decode_images(self):

        while True:
            frame, num = self.decoder.getFrame()
            self.frame_queue.put_nowait((num, frame))
            
    def _receive_images(self):

        while True:
            bytes_chunk = self.socket.recv(BUFSIZE)
            self.decoder.supplyBytes(bytes_chunk)

    def run(self):
        
        print('awaiting connection')
        
        self.sock.listen(5)
        self.socket, address = self.sock.accept()
        print('connection established')
        
        self.image_receiving_thread.start()
        self.image_decoding_thread.start()
        print("receiver is running")
        
        while True:
            pts, frame = self.frame_queue.get()
            cv2.imshow('frame',frame)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--port', type=int, default=17098)
    parser.add_argument('--frame-width', type=int, default=640)
    parser.add_argument('--frame-height', type=int, default=480)
    parser.add_argument('--codec', type=str, default="h264")

    args = parser.parse_args()
    return args

if __name__ == "__main__":

    args = parse_args()

    receiver = FrameReceiver(args.port, args.frame_width, args.frame_height, args.codec)
    receiver.run()