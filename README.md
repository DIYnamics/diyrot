# NetDPR
True client-server based DigiPyRo

## User flow
- Website loads
	- User selects video
	- User selects approx. circle on video canvas
	- [approx. cicle, RPM, video] form is sent
		- with jquery progress bar
- Website shows processing queue status
- Website shows video processing status OR video processing error
	- reselect approx. circle OR
	- download derotated video
- display derotated video

## Server Flow
- [approx. circle, RPM, video] form enters
	- enters task queue
		- *set cookie*
		- client must send periodic positions request
	- leave form flow
- poll request enters
	- get current status, reply
	- leave request flow
- concurrent task processing: pop from task queue
	- pick:
		- synchronously use python openCV Houghs to find radius
		- subprocess to C++ binary to find radius
	- put status/error to next poll request
	- pick:
		- synchronous derotate video in python
		- subprocess to C++ binary, derotate (preferred)
	- reply to client poll with file

## TaskQueue Implementation
- Input: # of "cores" assigned to TaskQueue
- Instantiate 1 + 1.5*cores threads
	- 1 handles queue input / queue requests
	- 0.5*core handle circle detection (circdet) queue + subproc management
	- 1*core handle derotation queue + derotation subprocess management (..)
- circdet queue
	- items can fail (requeue at end)
	- success items get passed onto derotation queue
	- subproc c++ program to detect houghcircles, print on stdout
- derotation queue
	- items do not fail
	- subproc c++ program to read job file and write files (perfect parallel)

## Notes
- see handwritten note for full application structure
- separating openCV processing from requests processing MEANS that uploaded 
	files must be saved to a temporary location
- asyncio queues are not threadsafe: use `Queue.queue`
	- do we actually need threadsafe?
	- more useful than `collections.deque`
- unfortunately, the most efficient c++ subroutines would instantiate once,
	then repeatedly process on pipe/network inputs. Having python threads which
	call subprocesses instantiate n times. Can change later
- use 2 queues: one for circle detection, one for actual derotation
	- circdet queue can fail; in that case, requeue from end
	- derotation queue cannot fail

## OpenCV Compilation
Only need `core`, `imgproc` (for hough), and `videoio` (for something really
transforming in and out of matrices). Write and compile static libs, link to
fully static binaries, eventually compile to webasm...

## Links
['hackproof' filenames](https://werkzeug.palletsprojects.com/en/1.0.x/utils/#werkzeug.utils.secure_filename)  
[file upload dir](https://flask.palletsprojects.com/en/1.1.x/patterns/fileuploads/#uploading-files)  
[werkzeug file obj](https://werkzeug.palletsprojects.com/en/1.0.x/datastructures/#werkzeug.datastructures.FileStorage)  
[file in Requests](https://flask.palletsprojects.com/en/1.1.x/api/#flask.Request)  
[uWSGI (+NGINX)](https://flask.palletsprojects.com/en/1.1.x/deploying/wsgi-standalone/)  
