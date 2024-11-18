# python server which calls c++ code in dcppr/
# TODO: 500 is not the right return code, but ndpr.js current uses it to differentiate
# successful requests from failures
import os
import random
import datetime
import string
import subprocess
from flask import Flask, request, send_file, send_from_directory, url_for, jsonify
from werkzeug.utils import secure_filename

if 'ROOT_DIR' in os.environ:
    _root_dir = os.environ['ROOT_DIR']
else:
    _root_dir = '.'
random.seed()

# see derot.cpp `returns` for actual logic. TODO: derot.cpp should pipe to stderr
_errors = {
    255: "Server encountered an error with derotation input",
    254: "Server could not read the first frame of the input video",
    253: "The uploaded input video was too small (<360p) or too big (>5k)",
    252: "Server could not create an output video to write to",
    251: "Server encountered an error with advanced input parameters",
    3: "Server finished derotation, but there were no points to track at the end"
}

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = _root_dir + '/uploads'
# enable max 1G uploads. note that similar settings exist in nginx and uwsgi
app.config['MAX_CONTENT_LENGTH'] = 1000 * 1024 * 1024

# actual production methods
@app.route('/upload/', methods=['POST'])
def save():
    '''
    save incoming video file under random name. call circle detection
    function to find a circle, and if found, generate a preview video with
    found parameters. if cannot be found, return a 500, with the random file
    name (a random name is desirable to prevent brute force downloading)
    '''
    try:
        # get form file object
        v = request.files['v']
        vidfn = secure_filename(v.filename)
        # pick name
        vname = ''.join([*random.choices(string.ascii_letters, k=5),
                         os.path.splitext(vidfn)[-1]])
        preview_img_fn = vname + '.jpeg'
        # pick path
        vpath = os.path.join(app.config['UPLOAD_FOLDER'], vname)
        # save, close
        v.save(vpath)
        v.close()
        # detect circle and generate preview
        x,y,r = opencv_detect(vpath, os.path.join(_root_dir, 'return', preview_img_fn))
        preview_fn = opencv_preview(vname, x, y, r, float(request.form['rpm']),
                                    request.form['sbs'] == 'true')
    except Exception as e:
        # stdout is connected to syslog and viewable thru monitoring
        print(' '.join(['error in upload', str(request.form), str(e)]),
              flush=True)
        if e.returncode in _errors:
            return (jsonify({'err': _errors[e.returncode]}), 500)
        return (jsonify({'fn': vname, 'img': '/return/'+preview_img_fn}), 500)
    return (jsonify({'fn': vname, 'x': x, 'y': y, 'r': r,
                     'src': '/return/'+preview_fn, 'img': '/return/'+preview_img_fn}),
            200)

@app.route('/preview/', methods=['POST'])
def prev():
    try:
        # re-preview a previously uploaded file
        vidfn = secure_filename(request.form['v'])
        preview_fn = opencv_preview(vidfn,
                                    float(request.form['x']),
                                    float(request.form['y']),
                                    float(request.form['r']),
                                    float(request.form['rpm']),
                                    request.form['sbs'] == 'true')
    except Exception as e:
        # stdout is connected to syslog and viewable thru monitoring
        print(' '.join(['error in preview', str(request.form), str(e)]),
              flush=True)
        if e.returncode in _errors:
            return (jsonify({'err': _errors[e.returncode]}), 500)
        return ('', 500)
    return (jsonify({'fn': vidfn,
                    'x': request.form['x'],
                    'y': request.form['y'],
                    'r': request.form['r'],
                    'src': '/return/'+preview_fn}),
            200)

@app.route('/advpreview/', methods=['POST'])
def advprev():
    try:
        vidfn = secure_filename(request.form['v'])
        adv_preview_fn = opencv_preview(vidfn,
                                        float(request.form['x']),
                                        float(request.form['y']),
                                        float(request.form['r']),
                                        float(request.form['rpm']),
                                        request.form['sbs'] == 'true',
                                        request.form['adv'],
                                        request.form['advData'],
                                        request.form['visRadius'] == 'true')
    except Exception as e:
        # prints are connected to syslog and viewable thru monitoring
        print(' '.join(['error in advanced preview', str(request.form),
                        str(e)]), flush=True)
        if e.returncode in _errors:
            return (jsonify({'err': _errors[e.returncode]}), 500)
        return ('', 500)
    return (jsonify({'fn': vidfn,
                    'x': request.form['x'],
                    'y': request.form['y'],
                    'r': request.form['r'],
                    'src': '/return/'+adv_preview_fn}),
            200)

@app.route('/derot/', methods=['POST'])
def derot():
    try:
        vidfn = secure_filename(request.form['v'])
        # derotated a video that passes preview
        r = opencv_derot(vidfn,
                         float(request.form['x']),
                         float(request.form['y']),
                         float(request.form['r']),
                         float(request.form['rpm']), 
                         request.form['sbs'] == 'true')
    except Exception as e:
        # prints are connected to syslog and viewable thru monitoring
        print(' '.join(['error in derot', str(request.form), str(e)]), flush=True)
        return ('', 500)
    return (r, 200)

@app.route('/advderot/', methods=['POST'])
def advderot():
    try:
        vidfn = secure_filename(request.form['v'])
        # derotated a video that passes preview
        r = opencv_derot(vidfn,
                         float(request.form['x']),
                         float(request.form['y']),
                         float(request.form['r']),
                         float(request.form['rpm']),
                         request.form['sbs'] == 'true',
                         request.form['adv'],
                         request.form['advData'],
                         request.form['visRadius'] == 'true',
                         request.form['exportCSV'] == 'true')
    except Exception as e:
        # prints are connected to syslog and viewable thru monitoring
        print(' '.join(['error in advderot', str(request.form), str(e)]), flush=True)
        return ('', 500)
    return (r, 200)

@app.route('/count/')
def update_count():
    '''
    Site counter function. Read-increment-writes a file each time the endpoint
    is hit. Can lose count on concurrent accesses
    '''
    c = 0
    try:
        with open('count', 'r+') as f:
            c = int(f.read()) + 1
            f.seek(0)
            f.write(str(c))
        with open('date', 'r') as f:
            d = f.read()
        return ( jsonify({'count': str(c),
                         'date': d}), 200 )
    except:
        with open('count', 'w') as f:
            f.write('0')
        with open('date', 'w') as f:
            d = datetime.datetime.now(datetime.timezone.utc).strftime("%F")
            f.write(d)
    return ( jsonify({'count': '0',
                      'date': d}), 200 )

def opencv_detect(vidfn, preview_out_fn):
    # calls radius checker with give video
    radii_check_bin = os.path.join(_root_dir, 'bin', 'radii_check')
    return subprocess.check_output([radii_check_bin, vidfn, preview_out_fn], text=True).split()

def opencv_preview(vidfn, x, y, r, rpm, sbs,
                   adv='', advData='', visRadius=False):
    # appends '-pre' before extension in filename; frontend attempts to load
    # this video on return
    out_fn = os.path.splitext(vidfn)[0] + ('-advpre' if adv else '-pre')
    extn = '.mp4'

    prog_name = ''.join(['sbs_' if sbs else '', 'adv_' if adv else '', 'prederot'])
    cmd = [os.path.join(_root_dir, 'bin', prog_name),
           os.path.join(_root_dir, 'uploads', vidfn),
           str(x), str(y), str(r), str(rpm),
           os.path.join(_root_dir, 'return', out_fn+extn)]
    if adv:
        cmd += ['1' if adv == 'auto' else '0', advData, '1' if visRadius else '0']
    # waits for cmd return and throws on non-zero
    subprocess.check_call(cmd)

    return out_fn+extn

def opencv_derot(vidfn, x, y, r, rpm, sbs,
                 adv='', advData='', visRadius=False, exportCSV=False):
    # full quality derotation
    # start job and immediately return. up to frontend+nginx to keep track of
    # video derotation progress.
    out_fn = os.path.splitext(vidfn)[0]
    extn = '.mp4'

    # rename nicely with important metadata
    out_fn = "_".join(['diyrot', str(round(rpm))+'rpm', str(out_fn), 'derot'])
    if adv:
        out_fn += "_tracking"

    # run regular job
    prog_name = ''.join(['sbs_' if sbs else '', 'adv_' if adv else '', 'derot'])
    cmd = [os.path.join(_root_dir, 'bin', prog_name),
           os.path.join(_root_dir, 'uploads', vidfn),
           str(x), str(y), str(r), str(rpm),
           os.path.join(_root_dir, 'return', out_fn+extn)]
    if adv:
        cmd += ['1' if adv == 'auto' else '0', advData,
                '1' if visRadius else '0', '1' if exportCSV else '0']

    subprocess.Popen(cmd)
    return out_fn+extn

# used in development only - equivalents exist for nginx in prod
# all are static file routes
@app.route('/')
def index():
    return send_file('../static/index.html')

@app.route('/favicon.ico')
def favico():
    return send_file('../static/favicon.ico')

@app.route('/ndpr.js')
def netdprjs():
    return send_file('../static/ndpr.js')

@app.route('/return/<vid>')
def retvid(vid):
    return send_file(_root_dir + '/return/' + vid)

@app.route('/spin-logo.png')
def spinlogo():
    return send_file('../static/spin-logo.png')

@app.route('/diynamics-logo.png')
def logo():
    return send_file('../static/diynamics-logo.png')

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8081)
