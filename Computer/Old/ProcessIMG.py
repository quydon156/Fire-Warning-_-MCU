import cv2
from ultralytics import YOLO

import socket
import struct
import time

# My_Communication
TCP_IP = '192.168.6.96'
TCP_PORT = 10000
BUFFER_SIZE = 1024
value = 1
notDetected = 0
count = 0
count0th = 0

# My_Connection
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))

# Load the YOLOv8 model
model = YOLO('best.pt')

# Open the video file
video_path = "rtsp://192.168.6.97:8554/mjpeg/1"
cap = cv2.VideoCapture(video_path)

# Loop through the video frames
while cap.isOpened():
    # Read a frame from the video
    success, frame = cap.read()

    if success:
        # Run YOLOv8 tracking on the frame, persisting tracks between frames
        results = model.track(frame, persist=True)
        
        # View results
        for r in results:
            if (list(r.boxes.shape)[0] != 0):
                print("Fire Detected by QuyDon")
                count += 1
                count0th = 0

            if (list(r.boxes.shape)[0] == 0):
                count = 0
                count0th +=1

            if (count == 25):
                mesg = bytearray(struct.pack("f", value)) 
                s.send(mesg)
                count = 0
                
            if (count0th == 2):
                mesg = bytearray(struct.pack("f", notDetected)) 
                s.send(mesg)
                count0th = 0
                #value=value+5.0
                #time.sleep(3)
            #else: 
                #mesg = bytearray(struct.pack("f", notDetected)) 
                #s.send(mesg)
            
            #print(list(r.boxes.shape))
            '''
            print(r.boxes.data)
            print(type(r.boxes.data))
            print(r.boxes.shape)  # print the Boxes object containing the detection bounding boxes
            print(list(r.boxes.shape))
            print(type(r.boxes.shape))
            '''

        # Visualize the results on the frame
        annotated_frame = results[0].plot()

        # Display the annotated frame
        cv2.imshow("YOLOv8 Tracking", annotated_frame)

        # Break the loop if 'q' is pressed
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break
    else:
        # Break the loop if the end of the video is reached
        break

# Release the video capture object and close the display window
s.close()
cap.release()
cv2.destroyAllWindows()