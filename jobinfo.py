''' jobinfo
Stores constants and object structure related to OpenCV jobs.
Meant to be used with TaskQueue. '''

PROC_SUCC = 0
PROC_EXCEPT = -1 # unknown error
PROC_NO_CIRCLE = -2 # can't detect circle
PROC_QUEUED = 1
PROC_PROC = 2

# probably get this from flask config
_VIDEO_DROPBOX = ''

from os.path import exists, join

class Job:
    def __init__(self, num: int, vid_filepath: str, rpm: float
                 cir_x: float, cir_y: float, cir_r: float) -> None:
        # ensure file exists before allowing job
        if !exists(join(_VIDEO_DROPBOX, vid_filepath)):
            raise FileNotFoundError(f'{vid_filepath} not found in dropbox folder!')
        self.vid_filepath = vid_filepath
        self.num = num
        self.rpm = rpm
        self.cir_x = cir_x
        self.cir_y = cir_y
        self.cir_r = cir_r
        self.status = PROC_EXCEPT
