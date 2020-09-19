import os
import random
import cv2
from flask import Flask, request, send_file, send_from_directory, url_for
from werkzeug.utils import secure_filename

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = '/home/alch/uploads'
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024

@app.route('/')
def index():
    return send_file('ndpr.html')

@app.route('/jquery.min.js')
def jquery():
    return send_file('jquery.min.js')

@app.route('/ndpr.js')
def netdprjs():
    return send_file('ndpr.js')

@app.route('/upload', methods=['POST'])
def process():
    try:
        rpm, r, v = float(request.form['rpm']), float(request.form['r']), request.files['v']
        vpath = os.path.join(app.config['UPLOAD_FOLDER'], secure_filename(v.filename))
        v.save(vpath)
        v.close()
        vidcv = cv2.VideoCapture(vpath)
        center, r = opencv_detect(vidcv, r)
        vout = opencv_derot(vidcv, center, r, rpm)
    except:
        return "bad img", 400
    return url_for('sendback', fn=vout), 200

@app.route('/return/<fn>')
def sendback(fn):
    return send_from_directory(app.config['UPLOAD_FOLDER'], fn)

def opencv_detect(cv_vid, r):
    _, f = cv_vid.read()
    f = cv2.cvtColor(f, cv2.COLOR_BGR2GRAY)
    f = cv2.medianBlur(f, 5)
    ans = cv2.HoughCircles(f, cv2.HOUGH_GRADIENT, 1, r * 0.8, 300, 100, 250, 0)[0]
    assert(len(ans[0]) == 3) # at least 1 and correctly detected
    return tuple(ans[0][:2]), ans[0][-1]

def opencv_derot(vidcv, center, r, rpm):
    fps = int(vidcv.get(cv2.CAP_PROP_FPS))
    frames = int(vidcv.get(cv2.CAP_PROP_FRAME_COUNT) / 10)
    dim = (int(vidcv.get(cv2.CAP_PROP_FRAME_WIDTH)), int(vidcv.get(cv2.CAP_PROP_FRAME_HEIGHT)))
    dt = -1 * 6 * rpm / fps
    voutfn = "%x.mp4"%random.getrandbits(30)
    vidout = cv2.VideoWriter(os.path.join(app.config['UPLOAD_FOLDER'], voutfn),
                             cv2.VideoWriter_fourcc(*'h264'), fps, dim)
    for i in range(frames):
        _, f = vidcv.read()
        f = cv2.warpAffine(f, cv2.getRotationMatrix2D(center, i*dt, 1.0), dim)
        vidout.write(f)
    vidout.release()
    return voutfn

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=80)
