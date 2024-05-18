import matplotlib
from matplotlib import pyplot as plt
import cv2
import numpy as np
import imageio
from IPython.display import Video, Image
from collections import deque

cap = cv2.VideoCapture('example_irl.mp4')

# parameters for goodfeaturestotrack
feature_params = dict( maxCorners = 100,
                      qualityLevel = 0.30,
                      minDistance = 7,
                      blockSize = 7)

# Parameters for lucas kanade optical flow
lk_params = dict(winSize  = (15, 15),
                 maxLevel = 2,
                 criteria = (cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 10, 0.03))

# Create some random colors
color = np.random.randint(0, 255, (1000, 3))

fps = 29.97; rpm = 13.45; r = 227; center = np.array([462, 260]).astype('int')

_, old_frame = cap.read()

# mask on good features to track to only Region Of Interest; greyscale and find 
# only needed for auto tracking
mask = np.zeros_like(old_frame)
mask = cv2.circle(mask, center, 1, color[-1].tolist(), -1)
old_gray = cv2.cvtColor(old_frame, cv2.COLOR_BGR2GRAY)

m1 = cv2.circle(np.zeros_like(old_frame), center, 10, (255,)*3, -1)
m2 = cv2.circle(np.zeros_like(old_frame), center, r-20, (255,)*3, -1)
old_mask = cv2.cvtColor(cv2.subtract(m2, m1), cv2.COLOR_BGR2GRAY)

# provide a first set of points to track, using auto or manual
p0 = cv2.goodFeaturesToTrack(old_gray, mask = old_mask, **feature_params)

# pick a window of frames for force computation
window = 30

# set up double ended queues for each point in the initial input
xs = [deque([0]*window) for i in range(p0.shape[0])]
ys = [deque([0]*window) for i in range(p0.shape[0])]
# also track time and radius, theta for velocity derivation (for physics)
t = deque([0]*window)
r = [deque([0]*window) for i in range(p0.shape[0])]
theta = [deque([0]*window) for i in range(p0.shape[0])]


nframes = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
_, frame = cap.read()
# second argument is a 3-tuple of frames and shape; out has axies frames,
# (...shape), but also rgb pixel
out = np.resize(np.zeros_like(frame[np.newaxis, ...]), (nframes-window,) + frame.shape)
# we subtract window=30 frames from the original video... but not necessary??
for nf in range(nframes-window):
    ret, frame = cap.read()
    if not ret:
        break
    frame_gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    # calculate optical flow, if the optical flow can be found; forever discard
    # points which can't be tracked (but keep them for physics csv?)
    p1, st, err = cv2.calcOpticalFlowPyrLK(old_gray, frame_gray, p0, None, **lk_params)
    # for physics: draw the tracks
    t.popleft(); t.append(nf / fps)
    for i, (valid, coord) in enumerate(zip(st, p1)):
        if not valid:
            continue
        c, d = coord.ravel()
        c -= center[0]
        d -= center[1]
        x = xs[i]; y = ys[i]
        x.popleft(); y.popleft()#; r.popleft(); theta.popleft()
        x.append(c)
        y.append(d)
        # for physcis: verbatim translate
        r.append(np.sqrt(((c**2)+(d**2))))
        ballTheta = np.arctan2(d, c)
        ballTheta += 2*np.pi if ballTheta < 0 else 0 # radians unit-ify
        theta[i].append(ballTheta)
        if nf > window:
            ## for physics: draw physics things (also again only draw one)
            #ux = np.gradient(x, t)
            #uy = np.gradient(y, t)
        # TODO: these methods seem slightly different than the ones in the
        # original digipyro; specifically, we're saving r, but never using
        # it; in the original it gets transfomed and eventually saved to the csv
            #fTh = 4*rpm*np.pi / 60.0
            #rInert = np.sqrt(ux**2 + uy**2) / fTh
            #rInertSmooth = np.polyval(np.polyfit(t, rInert, 20), t)
            #uxSmooth = np.polyval(np.polyfit(t, ux, 20), t)
            #uySmooth = np.polyval(np.polyfit(t, uy, 20), t)
            #mask = cv2.arrowedLine(mask, (int(x[-1]+center[0]), int(y[-1]+center[1])),
            #                (int(x[-1]+center[0]+uxSmooth[-1]), int(y[-1]+center[1]+uySmooth[-1])),
            #                (255, 0, 0), 1)
            # draw the tracking points onto the mask, for the color specified
            # for the corner point
            for index in range(window):
                ox, oy = x[index] + center[0], y[index] + center[1]
                mask = cv2.circle(mask, (int(ox), int(oy)), 1, color[i].tolist(), -1)
                ## for physics: draw more physics things (TODO: should only draw one, not all
                #ang = np.arctan2(uy[index], ux[index])
                #rad = -rInertSmooth[index]
                #mask = cv2.line(mask, (int(ox), int(oy)),
                #         (int(ox+rad*np.sin(ang)), int(oy-rad*np.cos(ang))),
                #         color[i].tolist(), 1)
    out[nf] = cv2.cvtColor(cv2.add(frame, mask), cv2.COLOR_BGR2RGB)
    old_gray = frame_gray.copy()
    p0 = p1

vid_out = cv2.VideoWriter('out.mp4', cv2.VideoWriter.fourcc('a', 'v', 'c', '1'),
                          fps, (frame.shape[1], frame.shape[0]))

for i, fr in enumerate(out):
    vid_out.write(fr)
vid_out.release()


# PROBABLY don't need this, remember something about optical flow being too slow.
#cap = cv2.VideoCapture('diyrot_13rpm_HOJGy_derot.mp4')
#nframes = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
#ret, frame1 = cap.read()
## make gray
#prvs = cv2.cvtColor(frame1, cv2.COLOR_BGR2GRAY)
#hsv = np.zeros_like(frame1)
#out = np.resize(np.zeros_like(frame1[np.newaxis, ...]), (nframes,) + frame1.shape) 
#hsv[..., 1] = 255
#for x in range(nframes):
#    ret, frame2 = cap.read()
#    if not ret:
#        break
#    cur = cv2.cvtColor(frame2, cv2.COLOR_BGR2GRAY)
#    flow = cv2.calcOpticalFlowFarneback(prvs, cur, None, 0.5, 3, 15, 3, 5, 1.1, 0)
#    mag, ang = cv2.cartToPolar(flow[..., 0], flow[..., 1])
#    hsv[..., 0] = ang*180/np.pi/2
#    hsv[..., 2] = cv2.normalize(mag, None, 0, 255, cv2.NORM_MINMAX)
#    bgr = cv2.cvtColor(hsv, cv2.COLOR_HSV2BGR)
#    out[x] = bgr
#    prev = cur
#good = [0 for i in range(out.shape[0]) if np.sum(out[i]) == 0]
#show_video(out[good])
