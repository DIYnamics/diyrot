// javascript for frontend 
'use strict';

// Utility functions
const changeInstruction = text => $( '#status' ).first().html(text)
const changeInfo = () => $('#status').attr('class', 'alert alert-info')
const changeWarning = () => $('#status').attr('class', 'alert alert-warning')
const changeDanger = () => $('#status').attr('class', 'alert alert-danger')
const changeSuccess = () => $('#status').attr('class', 'alert alert-success')
const sleep = m => new Promise(r => setTimeout(r, m))

// State cookie functions
const saveState = (j) => document.cookie = "state=" + j + ";"
const clearState = () => document.cookie = "state={}; expires=0;"

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

// Canvas functions
// make canvas match video
const initCanvas = () => {
    const canv = $( '#drawSurf' )[0];
    const canvctxt = $( '#drawSurf' )[0].getContext('2d');
	const vidIn = $( '#videoIn' )[0]
	canv.scale_factor = vidIn.scale_factor
	canv.height = vidIn.height
	canv.width = vidIn.width
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
const drawCirc = (x, y, r, xc, yc) => {
    const canv = $( '#drawSurf' )[0];
    const canvctxt = $( '#drawSurf' )[0].getContext('2d');
    clearCanvas()
    canvctxt.beginPath()
    canvctxt.moveTo(x, y)
    canvctxt.lineTo(xc, yc)
    canvctxt.closePath()
    canvctxt.stroke()
    canvctxt.beginPath()
    canvctxt.moveTo(x-5, y)
    canvctxt.lineTo(x+5, y)
    canvctxt.closePath()
    canvctxt.stroke()
    canvctxt.beginPath()
    canvctxt.moveTo(x, y-5)
    canvctxt.lineTo(x, y+5)
    canvctxt.closePath()
    canvctxt.stroke()
    canvctxt.beginPath()
    canvctxt.arc(x, y, r, 0, 2 * Math.PI)
    canvctxt.stroke()
    canvctxt.closePath()
}

// check if xsel, ysel is on circle (x, y, r)
const inCirc = (x, y, r, xsel, ysel) => {
    const rcurr = Math.sqrt(Math.pow(xsel - x, 2) + Math.pow(ysel - y, 2))
    return (rcurr > r - 10 && rcurr < r + 10) ? true : false
}

/* Canvas events:
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
}

const canvasDrag = (x, y) => {
    const canv = $( '#drawSurf' )[0];
    const canvctxt = $( '#drawSurf' )[0].getContext('2d');
    if (getState().r < -1 && canv.drawable == true) {
        // we are in moving state: rescale and redraw
        const newx = (getState().x / canv.scale_factor) + (x - canv.x)
        const newy = (getState().y / canv.scale_factor) + (y - canv.y)
        const r = -getState().r / canv.scale_factor
        drawCirc(newx, newy, r, newx, newy);
        setState('x', newx * canv.scale_factor); setState('y', newy * canv.scale_factor)
        canv.x = x; canv.y = y
    }
    // have x, y, but no definitive r 
    if(canv.drawable == true && getState().x && getState().y && getState().r == -1) {
        const centerx = getState().x / canv.scale_factor
        const centery = getState().y / canv.scale_factor
        drawCirc(centerx, centery, 
            Math.sqrt(Math.pow(x - centerx, 2) + Math.pow(y - centery, 2)), x, y);
        canv.x = x; canv.y = y
    }
}

const canvasUp = () => {
    const canv = $( '#drawSurf' )[0];
    const canvctxt = $( '#drawSurf' )[0].getContext('2d');
    if(canv.drawable == true && getState().r < -1) {
        setState('r', -getState().r)
    }
    if(canv.drawable == true && getState().r == -1) {
        const centerx = getState().x / canv.scale_factor
        const centery = getState().y / canv.scale_factor
        setState('r', Math.sqrt(
            Math.pow(canv.x - centerx, 2) + Math.pow(canv.y - centery, 2)) * canv.scale_factor)
    }
}

// element helper funcs
const hideEl = (l) => l.hidden = true 
const showEl = (l) => l.hidden = false
const boldEl = (l) => l.style.fontWeight = 'bolder'
const deboldEl = (l) => l.style.fontWeight = 'normal'

// revert to original video when click adjust
const dropManual = () => {
	hideEl($( '#derotBut' )[0])
	$( '#rpm' )[0].disabled = false
	$( '#square' )[0].disabled = false
	$( '#sideBS' )[0].disabled = false
	$( '#previewBut' )[0].value = "Regenerate Preview"
	changeInfo()
	changeInstruction('Manually changing rotation circle or RPM. The auto-detected \
    rotation circle (if found) is drawn. To specify the rotation circle \
	yourself, click-drag your mouse from the circle center to anywhere \
	on the edge of the circle. Once a suitable radius is selected, you can also \
	drag the circle around to adjust the rotation center.')
	setVideo(URL.createObjectURL($('#fileInput')[0].files[0]), () => {
		sleep(500) //hacky hack
		initCanvas()
		const canv = $( '#drawSurf' )[0]
		const sf = canv.scale_factor
        drawCirc(getState().x / sf, getState().y / sf, getState().r / sf, 
            getState().x / sf, getState().y / sf)
		canv.drawable = true
	})
}

// newSub = true => first submission
// does not return until server gives reply
const submitPreview = (newSub) => {
	clearCanvas()
	$( '#rpm' )[0].disabled = true
	$( '#square' )[0].disabled = true
	$( '#sideBS' )[0].disabled = true
    deboldEl($('#st1')[0])
    deboldEl($('#st2')[0])
    hideEl($('#st10')[0])
    hideEl($('#st20')[0])
	const r = new FormData()
	if (newSub) {
		// file is not on server, this is a new upload
		r.append('v', $('#fileInput')[0].files[0])
		changeInfo()
		changeInstruction('Uploading to server, see progress bar below.')
	}
	else {
		// otherwise, submit all for a preview
		changeInfo()
		changeInstruction('Generating new preview...')
		var s = getState()
		r.append('v', s.fn)
		r.append('r', s.r)
		r.append('x', s.x)
		r.append('y', s.y)
	}
	r.append('rpm', $('#rpm')[0].value)
	$.ajax(newSub ? '/upload/' : '/preview/', 
		{ method: 'POST', data: r, dataType: 'text', contentType: false, processData: false,
		beforeSend: () => {
			$( '#drawSurf' )[0].drawable = false
			showEl($( 'progress' )[0])
			hideEl($( '#previewBut' )[0])
			$( '#previewBut' )[0].value = "Adjust"
		},
		success: (d) => {
			saveState(d.split('\n')[0])
			setVideo(getState().src)
			changeWarning()
			changeInstruction('Here is a rough preview of the first few seconds of the derotated video. <br> \
				Make sure the video looks correctly derotated; if so, click \'Derotate\' \
				to get process the full video and get a download link. <br> If the \
				video looks wrong, click \'Adjust\' to manually configure RPM and/or the center of derotation.')
            boldEl($('#st2')[0])
			showEl($( '#derotBut' )[0])
		},
		error: (d) => { 
			saveState(d.responseText.split('\n')[0])
			changeDanger()
			changeInstruction('Your video was uploaded to the server, but the server couldn\'t detect a valid circle. <br> \
				Click \'Adjust\' to manually configure parameters, or pick a new file.')
            showEl($('#st10')[0])
            deboldEl($('#st1')[0])
            boldEl($('#st11')[0])
		},
		complete: () => {
			hideEl($( 'progress' )[0])
			showEl($( '#previewBut' )[0])
		},
		xhr: () => {
			var myXhr = new window.XMLHttpRequest() // drives progress bar
			myXhr.upload.addEventListener('progress', e => {
				if (e.lengthComputable) 
					$('progress').attr({ value: e.loaded, max: e.total, })
				if (e.loaded == e.total) {
					changeInfo()
					changeInstruction('Give the server a few seconds to generate a preview...')
				}
			}, false)
			return myXhr
		}
	})
}

// submit for full derotation
// returns immediatly and starts wait loop to check HEAD
const submitRot = () => {
	const status = getState()
    deboldEl($('#st1')[0])
    deboldEl($('#st2')[0])
    hideEl($('#st10')[0])
    hideEl($('#st20')[0])
    boldEl($('#st3')[0])
	const r = new FormData()
	r.append('v', status.fn)
	r.append('r', status.r)
	r.append('x', status.x)
	r.append('y', status.y)
	r.append('rpm', $('#rpm')[0].value)
	r.append('sbs', $('#sideBS')[0].checked)
	$.ajax('/derot/', { method: 'POST', data: r, dataType: 'text',
		contentType: false, processData: false,
		beforeSend: () => {
			$( '#drawSurf' )[0].drawable = false
			hideEl($('#derotBut')[0])
			hideEl($('#previewBut')[0])
		},
		success: (d) => {
			setState('src', d)
			setState('waiting', Date.now())
			pollWait()
		},
})}

// wait loop function
const pollWait = () => {
	const status = getState()
	if (status.waiting == undefined) {
		changeSuccess()
		changeInstruction('Done! Click <a download href=\"/return/'+getState().src+'\"> here </a> \
			to download the fully processed, high resolution video. <br> Refresh the page to derotate another video.')
		clearState()
		return
	}
	const timesince = Math.floor((Date.now() - status.waiting) / 1000)
	changeInfo()
    changeInstruction('Waiting for server to finish processing... this page will automatically update. \
		Last refreshed ' + timesince  + ' seconds ago')
	if (timesince > 10) {
		status.waiting = Date.now()
		$.ajax('/return/'+getState().src, {method: 'HEAD',
			success: () => {
				setState('waiting', undefined)
                deboldEl($('#st3')[0])
                boldEl($('#st4')[0])
			},
			error: () => {
				saveState(JSON.stringify(status))
			},
		})
	}
	setTimeout(pollWait, 1000)
}

// element maipulation 
const setVideo = (vl, cb = () => {} ) => {
	const vid = $( '#videoIn' )[0]
	vid.loop = true
	vid.src = vl
	vid.load()
	vid.play().then(cb())
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

// both canvas and video, drawing circle if exists
const resizeBoth = () => {
    resizeVideo()
    initCanvas()
    const canv = $( '#drawSurf' )[0]
    const sf = canv.scale_factor
    drawCirc(getState().x / sf, getState().y / sf, getState().r / sf, 
        getState().x / sf, getState().y / sf)
    canv.drawable = true
}

// resize listener
window.onresize = resizeBoth;

// Event Listeners
$(window).on('load', () => {

	// site counter
	$.ajax('/count/', {
		success: (d) => {
			$( '#useCount' ).first().html("This website has been used approximately " + d + " times since the last version update.")
	}})

	// element change listeners
	$( '#fileInput' ).on('change', e => {
		setVideo(URL.createObjectURL(e.target.files[0]))
		submitPreview(true)
	})
	$( '#videoIn' ).on('loadeddata', e => {
		resizeVideo()
	})
	$( '#previewBut' ).on('click', e => {
		var v = $( '#previewBut' )[0].value
		if (v == "Adjust") {
            showEl($('#st20')[0])
            boldEl($('#st21')[0])
            deboldEl($('#st2')[0])
			dropManual();
        }
		else {
			submitPreview(false);
        }
	})

	$( '#derotBut' ).on('click', () =>  submitRot() )
    // simple tooltip
    $( '#sideBS' ).on('click', () => {
        if ($('#sideBS')[0].checked) {
            showEl($('#SBShelp')[0])
        } else {
            hideEl($('#SBShelp')[0])
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
})
