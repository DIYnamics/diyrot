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
