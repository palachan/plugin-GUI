# -*- coding: utf-8 -*-
"""
Created on Tue Nov 28 15:58:54 2017

@author: Patrick
"""

import zmq
import cv2
import numpy as np
from scipy import ndimage
import csv

def run_video_acq(vc,frame,vidwriter,hostname='localhost', port=5556):
    with zmq.Context() as ctx:
        with ctx.socket(zmq.REQ) as sock:
            sock.connect('tcp://%s:%d' % (hostname, port))
            framecount = 0
            time_a = 0
            elapsed_sum = 0
            
            green_frame = np.zeros_like(frame)
            red_frame = np.zeros_like(frame)
            while True:
         
                cv2.imshow("raw video", frame)
                rval, frame = vc.read()
                key=cv2.waitKey(1)
                if key==99:
                    green_frame = np.zeros_like(frame)
                    red_frame = np.zeros_like(frame)
                sock.send_string('timestamp')
                timestamp=int(sock.recv_string())
#                print(timestamp)
                
                if framecount == 0:
                    time_a = timestamp
                    
                else:
                    time_b = timestamp

                if framecount > 0:
                    elapsed = float(time_b - time_a)/33000.
                    elapsed_sum += elapsed
                    avg_time = elapsed_sum/framecount
                    avg_fps = 1./avg_time
                    print('%f fps' % avg_fps)
                framecount += 1
                time_a = timestamp
                
                
                thresh = cv2.GaussianBlur(frame, (11, 11), 0)
                blue_img, green_img, red_img = cv2.split(thresh)
                gray = cv2.cvtColor(thresh, cv2.COLOR_BGR2GRAY)
                rbin, thresh_r = cv2.threshold(red_img, 100, 255, cv2.THRESH_TOZERO )  # thresholding
                rbin, thresh_g = cv2.threshold(green_img, 100, 255, cv2.THRESH_TOZERO)
#                rbin, thresh_b = cv2.threshold(blue_img, 100, 255, cv2.THRESH_TOZERO)
                rbin, thresh_bright = cv2.threshold(gray, 50, 150, cv2.THRESH_TOZERO)
#                if framecount == 40:
#                    return thresh_bright
#                
                thresh_g[thresh_bright == 0] = 0
                thresh_r[thresh_bright == 0] = 0

#                thresh_r[thresh_b > 0] = 0
#                thresh_g[thresh_b > 0] = 0
                
#                cv2.imshow("threshed",thresh_bright)
                
#                thresh_r = cv2.erode(thresh_r, None, iterations=1)
                thresh_r = cv2.dilate(thresh_r, None, iterations=3)
                
#                thresh_g = cv2.erode(thresh_g, None, iterations=1)
                thresh_g = cv2.dilate(thresh_g, None, iterations=3)
                
#                thresh_b = cv2.erode(thresh_b, None, iterations=2)
#                thresh_b = cv2.dilate(thresh_b, None, iterations=4)
                
#                thresh_bright = cv2.erode(thresh_bright, None, iterations=2)
#                thresh_bright = cv2.dilate(thresh_bright, None, iterations=4)
                
                thresh_r[thresh_g>thresh_r] = 0
                thresh_g[thresh_r>thresh_g] = 0
                
                rcoords = ndimage.measurements.center_of_mass(thresh_r)
                if np.isnan(rcoords[0]):
                    rcoords=(0,0)
                rx = int(rcoords[0])
                ry = int(rcoords[1])
                print rcoords
                
                gcoords = ndimage.measurements.center_of_mass(thresh_g)
                if np.isnan(gcoords[0]):
                    gcoords=(0,0)
                gx = int(gcoords[0])
                gy = int(gcoords[1])
                print gcoords

                vidwriter.writerow([timestamp,rx,ry,gx,gy])
            
                green_frame[thresh_g > 0] = [0,255,0]
                
#                red_frame = np.zeros_like(frame)
                red_frame[thresh_r > 0] = [0,0,255]
#                
                thresh =  green_frame + red_frame

                cv2.imshow("threshed",thresh)

cv2.namedWindow("raw video")
cv2.namedWindow("threshed")
#cv2.resizeWindow('raw video', 600,600)
#cv2.resizeWindow('threshed', 600,600)
vc = cv2.VideoCapture(0)
#vc.set(cv2.CAP_PROP_SETTINGS, 1);
vc.set(cv2.CAP_PROP_FRAME_WIDTH,1280)
vc.set(cv2.CAP_PROP_FRAME_HEIGHT,720)
vc.set(cv2.CAP_PROP_AUTOFOCUS, 0)
vc.set(cv2.CAP_PROP_EXPOSURE, -10)
vc.set(cv2.CAP_PROP_FPS, 30)



if vc.isOpened(): # try to get the first frame
    
    rval, frame = vc.read()
else:
    rval = False

if rval:
    with open('vt1.csv', 'wb') as csvfile:
        vidwriter = csv.writer(csvfile)
        vidwriter.writerow(['timestamp','red_x','red_y','green_x','green_y'])
        tb = run_video_acq(vc,frame,vidwriter)

cv2.destroyWindow("preview")
vc.release()