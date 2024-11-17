'use strict';
// Utility functions
const setInfo = t => {$('#status').attr('class', 'col-md-8 mb-0 alert alert-info'); $( '#status' ).first().html(t)}
const setWarning = t => {$('#status').attr('class', 'col-md-8 mb-0 alert alert-warning'); $( '#status' ).first().html(t)}
const setDanger = t => {$('#status').attr('class', 'col-md-8 mb-0 alert alert-danger'); $( '#status' ).first().html(t)}
const setSuccess = t => {$('#status').attr('class', 'col-md-8 mb-0 alert alert-success'); $( '#status' ).first().html(t)}
const sleep = m => new Promise(r => setTimeout(r, m))
// check whether we should drop to advanced from submission page or manual page
const isAdvanced = () => {
    // don't report we're in advanced to canvas if we're adjusting a preview
    if (!$( '#previewBut' )[0].hidden)
        return "";
    if ($('#manualTrack')[0].checked)
        return "manual";
    if ($('#autoTrack')[0].checked)
        return "auto";
    return ""
}

// State cookie functions
const saveState = (j) => document.cookie = "state=" + j + "; samesite=lax"
const clearState = () => document.cookie = "state={}; samesite=lax; expires=0; "

// status of upload is stored a state object which is a cookie
// state will have x/y/r of returned or drawn circle, filename on server, last HEAD time
const getState = () => {
    if (document.cookie.indexOf("state") == -1)
        return JSON.parse("{}")
    return JSON.parse(document.cookie.split('; ').find(row => row.startsWith('state')).split('=')[1])
}

const setState = (k, v) => {
    // set k: v with replacement
    var s = new Object()
    if (document.cookie.indexOf("state") != -1)
        s = getState()
    s[k] = v
    saveState(JSON.stringify(s))
    return s
}

const setTrackingPoint = (x, y, i = -1) => {
    // given viewpoint xy, save them as logical coords
    const sf = $('#drawSurf')[0].scale_factor
    const argPoint = [Math.round(x * sf), Math.round(y * sf)]
    var cur = getState().points
    try {
        if ((x < 0 || y < 0) && i != -1) {
            cur.splice(i, 1);
        } else if (i == -1 && cur.length < 10) {
            cur.push(argPoint)
        } else if (i != -1) {
            cur[i] = argPoint
        } else {
            cur.shift();
            cur.push(argPoint)
        }
        setState('points', cur)
    } catch {
        // typeError attribute access on undefined iff nonexistant
        // outOfBounds iff i is not a good parameter
        setState('points', [argPoint])
    }
}

const getTrackingPoints = () => {
    // returns points in their viewport coords, if any exist
    const sf = $('#drawSurf')[0].scale_factor
    return getState().points == undefined ? 
        [] :
        getState().points.map(p => [p[0] / sf, p[1] / sf]) 
}

const pointOverlap = (x, y) => {
    // detect if a viewpoint coord point overlaps another, return idx in state. return -1 if no overlap.
    const isPoint = p => Math.sqrt(Math.pow(p[0] - x, 2) + Math.pow(p[1] - y, 2)) < 7
    return getTrackingPoints().reduce((cur, p, i) => isPoint(p) ? i : cur, -1)
}

const drawPoint = (x, y) => {
    // draws a dot at viewpoint coords xy
    if (x <= 0 || y <= 0)
        return
    const canvctxt = $( '#drawSurf' )[0].getContext('2d');
    canvctxt.strokeStyle = 'blue'
    canvctxt.beginPath()
    canvctxt.arc(x, y, 2, 0, 2 * Math.PI)
    canvctxt.stroke()
    canvctxt.closePath()
}

const drawTrackingPoints = () => {
    getTrackingPoints().forEach(p => drawPoint(p[0], p[1]))
}

const selectedPoint = () => {
    const isSelected = p => p[0] == -1 && p[1] == -1
    return getTrackingPoints().reduce((cur, p, i) => isSelected(p) ? i : cur, -1)
}

// Canvas functions
// make canvas match video
const initCanvas = () => {
    const canv = $( '#drawSurf' )[0];
    const previewPic = $( '#previewPic')[0]
    canv.scale_factor = previewPic.scale_factor
    canv.height = previewPic.height
    canv.width = previewPic.width
    canv.getContext('2d').lineWidth = 3
    canv.getContext('2d').strokeStyle = 'red'
    clearCanvas()
}

const clearCanvas = () => {
    const canv = $( '#drawSurf' )[0];
    const canvctxt = $( '#drawSurf' )[0].getContext('2d');
    canvctxt.clearRect(0, 0, canv.width, canv.height)
}

// draw circle with crosshair and optional radius from x, y to xc, yc
// using viewpoint coords only
const drawCirc = (x, y, r, xc = -1, yc = -1, ss = 'red') => {
    xc = (xc == -1 ? x : xc)
    yc = (yc == -1 ? y : yc)
    const canv = $( '#drawSurf' )[0];
    const canvctxt = $( '#drawSurf' )[0].getContext('2d');
    canvctxt.strokeStyle = ss
    canvctxt.beginPath()
    canvctxt.moveTo(x, y)
    canvctxt.lineTo(xc, yc)
    canvctxt.stroke()
    canvctxt.closePath()
    canvctxt.beginPath()
    canvctxt.moveTo(x-5, y)
    canvctxt.lineTo(x+5, y)
    canvctxt.stroke()
    canvctxt.closePath()
    canvctxt.beginPath()
    canvctxt.moveTo(x, y-5)
    canvctxt.lineTo(x, y+5)
    canvctxt.stroke()
    canvctxt.closePath()
    canvctxt.beginPath()
    canvctxt.arc(x, y, r, 0, 2 * Math.PI)
    canvctxt.stroke()
    canvctxt.closePath()
}

const drawBoundsCirc = () => {
    const sf = $('#drawSurf')[0].scale_factor
    drawCirc(getState().x / sf, getState().y / sf, getState().r / sf)
}

// check if xsel, ysel is on circle (x, y, r), entirely on viewpoint coords
const inCirc = (x, y, r, xsel, ysel) => {
    const rcurr = Math.sqrt(Math.pow(xsel - x, 2) + Math.pow(ysel - y, 2))
    return (rcurr > r - 13 && rcurr < r + 13) ? true : false
}

const withinBoundsCirc = (xsel, ysel) => {
    // checks whether the viewport coord (xsel, ysel) is within the logical circle
    const sf = $('#drawSurf')[0].scale_factor
    return (getState().r/sf) - 5 > Math.sqrt(Math.pow(xsel - (getState().x/sf), 2) + Math.pow(ysel - (getState().y/sf), 2))
}

/* Canvas events: mostly interact within viewpoint coords,
 *  and check against logical coords.
 * three types: mousedown, mousemove, mouseup
 * mouseup does not have a touch up event 
 *
 * flow 1: mousedown -> drag -> mouseup 
 *      redraw from stored center 
 * flow 2: mousedown -> drag -> mouseup
 *      only occurs when r is not -1
 *      redraw from same r, moved center
 * state x, y, r: current specified circle params in video coords
 * canv x, y: last known touch positions, in canvas coords
 */
const canvasDown = (x, y) => {
    const canv = $( '#drawSurf' )[0];
    const canvctxt = $( '#drawSurf' )[0].getContext('2d');
    const centerx = getState().x / canv.scale_factor
    const centery = getState().y / canv.scale_factor
    switch (isAdvanced()) {
        case "auto":
            if (!withinBoundsCirc(x, y)) {
                return
            }
            if (canv.drawable == true) {
                clearCanvas()
                drawBoundsCirc()
                if (Math.sqrt(Math.pow(x - centerx, 2) + Math.pow(y - centery, 2)) < 5) {
                    setState('r_a', 0)
                    return
                }
                drawCirc(centerx, centery, 
                    Math.sqrt(Math.pow(x - centerx, 2) + Math.pow(y - centery, 2)), -1, -1, 'blue');
                setState('r_a', -1)
            }
            break
        case "manual":
            if (!withinBoundsCirc(x, y)) {
                return
            }
            var idx = pointOverlap(x, y)
            if (idx != -1) {
                canv.x = x; canv.y = y
                x = -1; y = -1
            }
            setTrackingPoint(x, y, idx)
            clearCanvas()
            drawBoundsCirc()
            drawTrackingPoints()
            $( '#advBut' ).prop('disabled', false)
            break
        case "":
            const r = getState().r / canv.scale_factor
            if (getState().r > 0 && canv.drawable == true && inCirc(centerx, centery, r, x, y)) {
                // in moving state: flag r through flipped, canvxy is first down loc
                setState('r', -getState().r)
                canv.x = x; canv.y = y
                return
            }
            if (canv.drawable == true) {
                clearCanvas()
                setState('x', x * canv.scale_factor); setState('y', y * canv.scale_factor)
                setState('r', -1)
                canv.x = x; canv.y = y
            }
    }}

const canvasDrag = (x, y) => {
    // note: this is also just a general mousemove
    const canv = $( '#drawSurf' )[0];
    const canvctxt = $( '#drawSurf' )[0].getContext('2d');
    const centerx = getState().x / canv.scale_factor
    const centery = getState().y / canv.scale_factor
    switch (isAdvanced()) {
        case "auto":
            if (canv.drawable == true && getState().r_a == -1 && withinBoundsCirc(x, y)) {
                clearCanvas()
                drawBoundsCirc()
                if (Math.sqrt(Math.pow(x - centerx, 2) + Math.pow(y - centery, 2)) < 5) {
                    setState('r_a', 0)
                    return
                }
                drawCirc(centerx, centery, 
                    Math.sqrt(Math.pow(x - centerx, 2) + Math.pow(y - centery, 2)), -1, -1, 'blue');
                canv.x = x; canv.y = y
            }
            break
        case "manual":
            if (selectedPoint() != -1) {
                clearCanvas()
                drawBoundsCirc()
                drawTrackingPoints()
                if (!withinBoundsCirc(x, y)) {
                    // acts on indicies and sets points, so don't convert to viewport
                    var cur = getState().points
                    cur.splice(selectedPoint(), 1)
                    setState('points', cur)
                } else {
                    drawPoint(x, y)
                    canv.x = x; canv.y = y
                }
            }
            break
        case "":
            if (getState().r < -1 && canv.drawable == true) {
                clearCanvas()
                // we are in moving state: rescale and redraw
                const newx = (getState().x / canv.scale_factor) + (x - canv.x)
                const newy = (getState().y / canv.scale_factor) + (y - canv.y)
                const r = -getState().r / canv.scale_factor
                drawCirc(newx, newy, r)
                setState('x', newx * canv.scale_factor); setState('y', newy * canv.scale_factor)
                canv.x = x; canv.y = y
            }
            // have x, y, but no definitive r 
            if(canv.drawable == true && getState().x && getState().y && getState().r == -1) {
                clearCanvas()
                drawCirc(centerx, centery, 
                    Math.sqrt(Math.pow(x - centerx, 2) + Math.pow(y - centery, 2)), x, y);
                canv.x = x; canv.y = y
            }
    }}

const canvasUp = () => {
    const canv = $( '#drawSurf' )[0];
    const centerx = getState().x / canv.scale_factor
    const centery = getState().y / canv.scale_factor
    switch (isAdvanced()) {
        case "auto":
            if(canv.drawable == true && getState().r_a == -1) {
                setState('r_a', Math.sqrt(
                    Math.pow(canv.x - centerx, 2) + Math.pow(canv.y - centery, 2)) * canv.scale_factor)
                $( '#advBut' ).prop('disabled', false)
            }
            break
        case "manual":
            if (selectedPoint() != -1) {
                clearCanvas()
                drawBoundsCirc()
                setTrackingPoint(canv.x, canv.y, selectedPoint())
                drawTrackingPoints()
            }
            break
        case "":
            if(canv.drawable == true && getState().r < -1) {
                setState('r', -getState().r)
            }
            if(canv.drawable == true && getState().r == -1) {
                setState('r', Math.sqrt(
                    Math.pow(canv.x - centerx, 2) + Math.pow(canv.y - centery, 2)) * canv.scale_factor)
            }
    }}

// element helper funcs
const hideEl = (...a) => a.forEach(l => $(l)[0].hidden = true)
const showEl = (...a) => a.forEach(l => $(l)[0].hidden = false)
const boldEl = (...a) => a.forEach(l => $(l)[0].style.fontWeight = 'bolder')
const deboldEl = (...a) => a.forEach(l => $(l)[0].style.fontWeight = 'normal')

// revert to original video when click adjust
const dropManual = () => {
    resizePreview()
    showEl('#st30', '#previewPic')
    boldEl('#st31')
    deboldEl('#st3')
    hideEl('#advBut', '#derotBut', '#videoIn')
    $( '#rpm, #fileInput, #sideBS, #manualTrack, #autoTrack, #exportCSV, #visRadius' ).prop('disabled', false)
    $( '#previewBut' )[0].value = "Regenerate Preview"
    setInfo('Respecify RPM, or change the rotation circle. The auto-detected \
        rotation circle (if found) is drawn. <br> To specify the rotation circle \
        yourself, click-drag your mouse from the circle center to anywhere \
        on the edge of the circle. <br> Once a suitable radius is selected, you can also \
        drag the circle around to adjust the rotation center.')
    initCanvas()
    const canv = $( '#drawSurf' )[0]
    const sf = canv.scale_factor
    drawCirc(getState().x / sf, getState().y / sf, getState().r / sf)
    canv.drawable = true
}

const dropAdv = () => {
    boldEl(isAdvanced() == "manual" ? '#stadvman' : '#stadvauto')
    showEl('#previewPic', '#st30')
    hideEl('#st31', '#previewBut', '#derotBut', '#videoIn')
    $( '#advBut' ).prop('disabled', true)
    $('#advBut')[0].value = 'Generate tracking preview'
    setInfo( isAdvanced() == "manual" ?
        'Click inside the red circle of rotation to select up to 10 points to \
     track. <br> Drag points to change their location. \
     Dragging them outside the red circle deletes the points.' :
        'Click or drag inside the red circle of rotation to set an exclusion zone \
     centered at the circle of rotation. <br> \
     The point-picking algorithm will pick ~50 points in between the red \
     and blue circles to track. <br> If no exclusion is needed, drag the blue \
     circle to the plus sign in the center.' )
    initCanvas()
    const canv = $( '#drawSurf' )[0]
    const sf = canv.scale_factor
    drawCirc(getState().x / sf, getState().y / sf, getState().r / sf)
    canv.drawable = true
    resizeAll()
}

// newSub = true => first submission
// does not return until server gives reply
const submitPreview = (newSub) => {
    const r = new FormData()
    if (newSub) {
        // file is not on server, this is a new upload
        r.append('v', $('#fileInput')[0].files[0])
        setInfo('Uploading to server, see progress bar below.')
    }
    else {
        // otherwise, submit all for a preview
        setInfo('Generating new preview...')
        var s = getState()
        r.append('v', s.fn)
        r.append('r', s.r)
        r.append('x', s.x)
        r.append('y', s.y)
    }
    r.append('rpm', $('#rpm')[0].value)
    r.append('sbs', $('#sideBS')[0].checked)
    $.ajax(newSub ? '/upload/' : '/preview/', {
        method: 'POST', data: r, dataType: 'text', contentType: false, processData: false,
        beforeSend: () => {
            clearCanvas()
            $( '#drawSurf' )[0].drawable = false
            $( '#rpm, #fileInput, #sideBS, #manualTrack, #autoTrack, #exportCSV, #visRadius' ).prop('disabled', true)
            deboldEl('#st2', '#st3')
            hideEl('#st20', '#st30', '#previewBut')
            if(newSub)
                showEl('progress')
            $( '#previewBut' )[0].value = "Adjust"
        },
        success: (d) => {
            saveState(d.split('\n')[0])
            setVideo(getState().src)
            showEl('#videoIn')
            hideEl('#previewPic')
            boldEl('#st3')
            if (newSub) {
                $('#previewPic')[0].src = getState().img
            }
            setWarning('Here is a rough preview of the first ten seconds of derotated video. <br>' +
                (isAdvanced() ?
                    'If it looks correct, click \'Next step\' to move onto object tracking.' :
                    'Make sure the video looks correctly derotated; if so, click \'Derotate\' \
                 to process the full video and get a download link.') + 
                '<br> If the video looks wrong, click \'Adjust\' to respecify RPM and/or the center of derotation.')
            showEl(isAdvanced() ? '#advBut' : '#derotBut')
            showEl('#previewBut')
        },
        error: (d) => {
            let err = JSON.parse(d.responseText.split('\n')[0])?.err;
            if (newSub && err == undefined) {
                saveState(d.responseText.split('\n')[0])
                $('#previewPic')[0].src = getState().img
                setDanger('Your video was uploaded to the server, but the server couldn\'t detect a \
                               valid circle in the first frame of the video. <br> \
                               Click \'Adjust\' to manually configure parameters, or pick a new file.')
                showEl('#st20')
                deboldEl('#st1', '#st2')
                boldEl('#st21')
                showEl('#previewBut')
                return;
            }
            clearState()
            setDanger('An error occured. ' + err + ' - please refresh the page to try again.')
        },
        complete: () => {
            hideEl('progress')
        },
        xhr: () => {
            var myXhr = new window.XMLHttpRequest() // drives progress bar
            myXhr.upload.addEventListener('progress', e => {
                if (e.lengthComputable) 
                    $('progress').attr({ value: e.loaded, max: e.total, })
                if (e.loaded == e.total) {
                    setInfo('Give the server a few seconds to generate a preview...')
                }
            }, false)
            return myXhr
        }
    })
}

const submitAdvPreview = () => {
    const r = new FormData()
    var s = getState()
    r.append('v', s.fn)
    r.append('r', s.r)
    r.append('x', s.x)
    r.append('y', s.y)
    r.append('rpm', $('#rpm')[0].value)
    r.append('sbs', $('#sideBS')[0].checked)
    r.append('adv', isAdvanced())
    r.append('advData', isAdvanced() == 'manual' ? s.points.toString() : (s.r_a ?? 0)+','+s.r)
    r.append('visRadius', $('#visRadius')[0].checked)
    $.ajax('/advpreview/', {
        method: 'POST', data: r, dataType: 'text', contentType: false, processData: false,
        beforeSend: () => {
            clearCanvas()
            $( '#drawSurf' )[0].drawable = false
            $( '#rpm, #fileInput, #sideBS, #manualTrack, #autoTrack, #exportCSV, #visRadius' ).prop('disabled', true)
            deboldEl('#st1', '#st2', '#st3', '#st31', '#st33')
            hideEl('#st20', '#st30', '#st33', '#advBut')
            setInfo('Generating preview of object tracking...')
        },
        success: (d) => {
            saveState(d.split('\n')[0])
            isAdvanced() == 'manual' ? setState('points', s.points) : setState('r_a', s.r_a ?? 0)
            setVideo(getState().src)
            showEl('#videoIn', '#st30', '#st33')
            hideEl('#previewPic')
            boldEl('#st30', '#st33')
            setWarning('Here is a rough preview of the first ten seconds object tracking. <br> \
                Make sure the video looks correctly derotated; if so, click \'Derotate\' \
                to process the full video and get a download link. <br> \
                If the video looks wrong, click \'Adjust\' to change the object tracking parameters.')
            showEl('#derotBut')
            $( '#advBut' )[0].value = "Adjust"
            showEl('#advBut')
        },
        error: (d) => {
            clearState()
            let err = JSON.parse(d.responseText.split('\n')[0])?.err;
            setDanger('An error occured. ' + err + ' - please refresh the page to try again.');
        }
    })
}

// submit for full derotation
// returns immediatly and starts wait loop to check HEAD
const submitRot = () => {
    const status = getState()
    deboldEl('#st1', '#st2', '#st3')
    hideEl('#st20', '#st30')
    boldEl('#st4')
    const r = new FormData()
    r.append('v', status.fn)
    r.append('r', status.r)
    r.append('x', status.x)
    r.append('y', status.y)
    r.append('rpm', $('#rpm')[0].value)
    r.append('sbs', $('#sideBS')[0].checked)
    var endpoint = "/derot/"
    if (isAdvanced()) {
        endpoint = "/advderot/"
        r.append('adv', isAdvanced())
        r.append('advData', isAdvanced() == 'manual' ?
            status.points.toString() :
                (status.r_a ?? 0)+','+status.r)
        r.append('visRadius', $('#visRadius')[0].checked)
        r.append('exportCSV', $('#exportCSV')[0].checked)
    }
    $.ajax(endpoint, { method: 'POST', data: r, dataType: 'text',
        contentType: false, processData: false,
        beforeSend: () => {
            $( '#drawSurf' )[0].drawable = false
            hideEl('#derotBut', '#advBut', '#previewBut')
        },
        success: (d) => {
            setState('src', d)
            setState('waiting', Date.now())
            pollWait()
        },
    })
}

// wait loop function
const pollWait = () => {
    const status = getState()
    if (status.waiting == undefined) {
        setSuccess('Done! Click <a download href=\"/return/'+getState().src+'\">here</a> \
            to download the fully processed, high resolution video. <br> ' +
            (!$('#exportCSV')[0].checked ? "" :
                'Click <a download href=\"/return/'+getState().src+'.csv\">here</a> \
                to download the tracking data in CSV format. <br> '
            + 'Refresh the page to derotate another video.'))
        clearState()
        return
    }
    const timesince = Math.floor((Date.now() - status.waiting) / 1000)
    setInfo('Waiting for server to finish processing... this page will automatically update. \
        Last refreshed ' + timesince  + ' seconds ago. <br> \
        Large videos (>100MB) may take up to 5 minutes to process.')
    if (timesince > 10) {
        status.waiting = Date.now()
        $.ajax('/return/'+getState().src, {method: 'HEAD',
            success: () => {
                setState('waiting', undefined)
                deboldEl('#st4')
                boldEl('#st5')
            },
            error: () => {
                saveState(JSON.stringify(status))
            },
        })
    }
    setTimeout(pollWait, 1000)
}

// element maipulation 
async function setVideo (vl) {
    const vid = $( '#videoIn' )[0]
    const oldsrc = vid.src
    vid.loop = true
    vid.src = vl
    vid.load()
    await vid.play().catch(()=>{})
    resizeVideo()
}

const resizeVideo = () => {
    const vidIn = $( '#videoIn' )[0]
    let og_h = vidIn.videoHeight, og_w = vidIn.videoWidth
    // use video container width, nicely set from flexbox
    const scale_factor = Math.max(og_h / window.innerHeight, og_w / $('#vidDiv')[0].clientWidth)
    // don't make videos bigger
    vidIn.scale_factor = (scale_factor > 1) ? scale_factor : 1
    vidIn.height = Math.round(og_h / vidIn.scale_factor)
    vidIn.width = Math.round(og_w / vidIn.scale_factor)
}

const resizePreview = () => {
    const previewPic = $( '#previewPic' )[0]
    let og_h = previewPic.naturalHeight, og_w = previewPic.naturalWidth
    const scale_factor = Math.max(og_h / window.innerHeight, og_w / $('#vidDiv')[0].clientWidth)
    previewPic.scale_factor = (scale_factor > 1) ? scale_factor : 1
    previewPic.height = Math.round(og_h / previewPic.scale_factor)
    previewPic.width = Math.round(og_w / previewPic.scale_factor)
}

var resizeTimeoutID = -1;

// both canvas and video, drawing circle if exists
// also set a timeout to call itself 100ms after done.
const resizeAll = (first = true) => {
    if (!first)
        clearTimeout(resizeTimeoutID);
    resizeVideo()
    resizePreview()
    const canv = $( '#drawSurf' )[0]
    const sf = canv.scale_factor
    if (canv.drawable) {
        initCanvas()
        drawBoundsCirc()
        // redraw advanced circle and points as well, if they exist
        switch (isAdvanced()) {
            case "auto":
                drawCirc(getState().x / sf, getState().y / sf, getState().r_a / sf, -1, -1, 'blue')
            break
            case "manual":
                drawTrackingPoints()
        }
        canv.drawable = true
    }
    if (first)
        resizeTimeoutID = setTimeout(resizeAll, 100, false);
}

// resize listener
window.onresize = resizeAll;

// Event Listeners
$(window).on('load', () => {

    // site counter
    $.ajax('/count/', {
        success: (d) => {
            $( '#useCount' ).first().html("This website has been used approximately " + d.count +
                " time" + (d.count == 1 ? "" : "s") + " since the last version update on " + d.date + ".")
    }})

    // element change listeners
    $( '#fileInput' ).on('change', e => {
        setVideo(URL.createObjectURL(e.target.files[0]))
        hideEl('#previewPic')
        submitPreview(true)
    })

    $( '#advBut' ).on('click', e => {
        if ($( '#advBut' )[0].value  == 'Next step' || $( '#advBut' )[0].value  == 'Adjust' || $( '#advBut' )[0].value == '') {
            dropAdv()
        }
        else {
            submitAdvPreview()
        }
    })

    $( '#previewBut' ).on('click', e => {
        var v = $( '#previewBut' )[0].value
        if (v == "Adjust") {
            dropManual()
        }
        else {
            submitPreview(false)
        }
    })

    $( '#derotBut' ).on('click', () =>  submitRot())
    // change advanced dropdown text and class
    $( '#advancedBut' ).on('click', () => {
        $('#advancedCard').toggleClass('card card-body');
        $('#advancedLabel').toggleClass('rainbow');
        event.target.innerHTML = event.target.classList.contains('collapsed') ? 'Expand...':'Collapse...';
    })

    // change tracking tooltip
    $( '#manualTrack' ).on('click', () => {
        $('#autoTrack')[0].checked = false
        if (event.target.checked) {
            hideEl('#autoTrackHelp', '#stadvauto')
            showEl('#manualTrackHelp', '#trackExtra', '#stadvman')
        } else {
            $('#exportCSV')[0].checked = false
            hideEl('#manualTrackHelp', '#trackExtra', '#stadvman')
        }
    })

    $( '#autoTrack' ).on('click', () => {
        $('#manualTrack')[0].checked = false
        if (event.target.checked) {
            hideEl('#manualTrackHelp', '#stadvman')
            showEl('#autoTrackHelp', '#trackExtra', '#stadvauto')
        } else {
            $('#exportCSV')[0].checked = false
            hideEl('#autoTrackHelp', '#trackExtra', '#stadvauto')
        }
    })

    const getTouchPos = (canvasDom, touchEvent) => {
        var rect = canvasDom.getBoundingClientRect();
        return {
            x: touchEvent.touches[0].clientX - rect.left,
            y: touchEvent.touches[0].clientY - rect.top
        };
    }

    //  circle listeners
    const canv = $( '#drawSurf' )[0]
    // touch listeners
    canv.addEventListener('touchstart', (e) => {
        e.preventDefault();
        var pos = getTouchPos(canv, e);
        canvasDown(pos.x, pos.y);
    })
    canv.addEventListener("touchmove", (e) => {
        e.preventDefault();
        var pos = getTouchPos(canv, e);
        canvasDrag(pos.x, pos.y);
    })
    canv.addEventListener('touchend', (e) => {
        e.preventDefault();
        canvasUp();
    })
    // regular listeners
    canv.addEventListener('mousedown', (e) => canvasDown(e.offsetX, e.offsetY));
    canv.addEventListener('mousemove', (e) => canvasDrag(e.offsetX, e.offsetY));
    canv.addEventListener('mouseup', (e) => canvasUp());

    // is there a valid waiting cookie? if not, ignore
    if (getState().waiting == undefined) {
        clearState();
    }
    else {
        pollWait();
    }

    // update while in focus
    $( '#rpm' )[0].addEventListener('input', (e) => {
        if (e.target.validity.valid) {
            $( '#fileInput' ).prop('disabled', false)
            $( '#advancedBut' ).prop('disabled', false)
        } else {
            $( '#fileInput' ).prop('disabled', true)
            $( '#advancedBut' ).prop('disabled', true)
        }
    })
    // update after out of focus
    $( '#rpm' )[0].addEventListener('change', (e) => {
        if (e.target.validity.valid) {
            boldEl('#st2')
            deboldEl('#st1')
        } else {
            boldEl('#st1')
            deboldEl('#st2')
        }
    })
})
