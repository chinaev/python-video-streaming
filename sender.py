import socket
from queue import Queue
import threading
from threading import Lock
import argparse
import cv2
from video_streaming import PyVideoEncoder

class FrameSender:
    
    def __init__(self, width, height, bitrate=10000000, codec_name="libx264", fps=25, gop_size=10, max_b_frames=1):

        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        self.frame_queue = Queue()

        self.image_sending_thread = threading.Thread(target=FrameSender._encode_send_frame, args=(self,))


        self.encoder = PyVideoEncoder(width, height, bitrate, codec_name.encode('utf-8'), fps, gop_size, max_b_frames)


    def connect(self, ip, port):
        
        self.socket.connect((ip, port))
        self.image_sending_thread.start()

    def _encode_send_frame(self):
        
        while True:
            frame, pts = self.frame_queue.get()
            frame_bytes = self.encoder.encodeFrame(frame)
            self.socket.sendall(frame_bytes)

    def send_frame(self, frame, pts):
        self.frame_queue.put((frame, pts))


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--ip', type=str, default="0.0.0.0")
    parser.add_argument('--port', type=int, default=17098)
    parser.add_argument('--frame-width', type=int, default=640)
    parser.add_argument('--frame-height', type=int, default=480)
    parser.add_argument('--bitrate', type=int, default=10000000)
    parser.add_argument('--codec', type=str, default="libx264")

    args = parser.parse_args()
    return args


if __name__ == "__main__":
    
    args = parse_args()
    
    frame_sender = FrameSender(args.frame_width, args.frame_height, bitrate=args.bitrate, codec_name=args.codec)
    frame_sender.connect(args.ip, args.port)
    
    cap = cv2.VideoCapture(0)
    
    pts = 0
    while True:
        
        ret, frame = cap.read()
        
        if not ret:
            continue
        
        frame_sender.send_frame(frame, pts)
        pts += 1
        
