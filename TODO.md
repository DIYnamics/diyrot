Done:
- finish final problem: how to when video is available on server to be loaded
    (without downloading?) [done]
- reorg files nicely for reproducibility across servers / easy cleanup / editing
    [done]
- add indicator of current progress, make client fetch current processing
    progress instead of having serving process wait until done [done]
- add cookies to survive refresh [done]
- make rpm float + change num text input + indicate rotation of tank [fixed] 
- add c o l o r and style [done]
- use a practice video [done]
- github page tutorial for diyrotate [ongoing haha]
- video tutorial link on page [doneish]
- make output filename diyrotate+rot rate+rand+.mp4 (done)
- bootstrap pretty website [logos spin/diy/epss] (done)
- add logging / rate limiting (logging done)
- make all return 200s (done)
- make pictures go to links (done)
- visitor counter (GA analytics added) (done)
- side by side option [done]
- add dreded documentation [ done]
- adjust does not actually visualize circle correctly (done)
- side by side add text showing each (done)
- simply checking existence of file is not enough (file may not be completely
    written yet) (fixed)
- add special code to allow touch devices (ipad/phone) to draw circle (done)
- should be able to drag circle around (tint surface maybe?) to change the
    selection circle
- resize on circle select requires a redraw of circle [done]
- center crosshair for circle [done]
- application arch documentation, ndpr.js docs, per-file header description
docs [done]
- textual step by step instead of the current nonsense [done]


Will not do (for a more motivated person than myself):
- process reporting on upload and derotation:
    - we would like the user to know how much time remains in the upload or derotation
    - for upload, modify the process element length callback to somehow cache time passed
        since last callback; use that to compute time left to upload
    - for derotation, need some method of communicating between server and frontend.
        - method 1: GET a special /progress/ endpoint, have derot.cpp write a number to files there
        - method 2: w e b s o c k e t s
- optimizations to derotation algorithm
    - currently `putText()` is called for every processed frame; would be more
        sensible to cache text drawing into a matrix and add that to main matrix
    - while current iteration seems to achieve zero matrix copies, likely not
        true; cannot convince myself warpAffine has no copies. if copies exist,
        can we reduce the data copied? i.e. when reading a video frame, only
        read region corresponding to designated circle, and constrain size of
        vid_frame/circle_mask matrix to exactly that size (visually, remove
        black bars)
- readability / maintainability of frontend
    - react / some sort of state machine for frontend: sit down and actually
        think about states of UI, and code it with that model in mind
- fortify backend against nefarious input
    - try-except pattern is pretty bad. should more verbosely explain what went
        wrong, reset backend state. is there any way bad agents can exploit bad
        backend code?
- SQL
    - site counter, video progress should be stored in a lightweight database
        and accessible across submitted jobs. naturally extends itself to a
        queue, job tracking, and site counter. file or processed based tracking
        is not good enough
- apple hevc codec auto reencode on upload
    - determine what happens when HEVC encoded videos taken from phone are
        uploaded to diyrot. Is user intervention needed to specify a suitable
        input for our program? 
- containerization
    - containerize all the things
