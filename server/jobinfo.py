''' jobinfo
Stores constants and object structure related to OpenCV jobs.
Meant to be used with TaskQueue. '''

from os.path import exists, join
from typing import NamedTuple
from queue import Queue

class CircJob(NamedTuple):
    id: int
    vid_filepath: str
    rpm: float = 10.0

class DerotJob(NamedTuple):
    id: int
    vid_filepath: str
    circ_x: float
    circ_y: float
    circ_r: float
    rpm: float = 10.0

class JobOut(NamedTuple)
    id: int
    status: int
