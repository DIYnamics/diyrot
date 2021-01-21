'use strict';

const getStatusJson = () => {
	if (document.cookie.indexOf("status") == -1)
		return JSON.parse("{}")
	return JSON.parse(document.cookie.split('; ').find(row => row.startsWith('status')).split('=')[1])
}
const changeInstruction = text => $( '#status' ).first().html(text)
const sleep = m => new Promise(r => setTimeout(r, m))
const saveCookie = (j) => document.cookie = "status=" + JSON.stringify(j) + "; secure"
const clearCookie = () => document.cookie = "status={}; expires=0; secure"

var center = {x: 0, y: 0}
var radii = undefined // set to values by user input and after circle
var scale_factor = undefined
var drawable = 0

const promptSubmit = () => {
	const status = getStatusJson()
    changeInstruction('Step 4/5: verify. The server has received your file and automatically detected the rotation perimeter.\n \
    If this looks correct, click submit again to have the server derotate your video.')
	const drawSurf = $( '#drawSurf' )[0].getContext('2d')
	drawSurf.clearRect(0, 0, drawSurf.canvas.width, drawSurf.canvas.height)
	drawSurf.beginPath()
	drawSurf.arc(status.x / scale_factor, status.y / scale_factor, status.r / scale_factor, 0, 2 * Math.PI)
	drawSurf.stroke()
	$( '#sendBut' )[0].style.visibility = "visible"
}

const pollWait = () => {
	const status = getStatusJson()
	if (status.waiting == undefined) {
		changeInstruction('Done! Pick new file to start again, or download from the link above.')
		return
	}
	const timesince = Math.floor((Date.now() - status.waiting) / 1000)
    changeInstruction('Step 5/5: Waiting for server to reply... this page will automatically update. \
		Last refreshed ' + timesince  + ' seconds ago')
	if (timesince > 7) {
		status.waiting = Date.now()
		$( '#videoIn' )[0].load()
		$( '#videoIn' )[0].play()
		// what a fucking hack
			.then(() => {
				$("body").prepend('<a download href=\"/return/'+status.fn+'\"> Download the video here </a>')
				clearCookie()}
			)
			.catch(() => saveCookie(status))
	}
	setTimeout(pollWait, 1000)
}

// Event Listeners
$(window).on('load', () => {
	if (getStatusJson().waiting == undefined) {
		// reset to original if not waiting
		clearCookie()
	} else {
		$( '#videoIn' ).attr('loop', true)
		$( '#videoIn' ).attr('src', '/return/' + getStatusJson().fn)
		pollWait()
	}
	// element change listeners
	$( '#fileInput' ).on('change', e => {
        changeInstruction('Step 2/5: select your rotation circle. Draw a line from the center of rotation to anywhere on the perimeter. \
			This does not have to be precise. Click upload when done.')
		$( '#videoIn' ).attr('src', URL.createObjectURL(e.target.files[0]))
		drawable = 1
	})

	$( '#videoIn' ).on('loadeddata', e => {
		const vidIn = $( '#videoIn' )[0]
		const canv = $( '#drawSurf' )[0]
		let og_h = vidIn.videoHeight, og_w = vidIn.videoWidth
		scale_factor = Math.max(og_h / window.screen.availHeight, og_w / $( '#vidDiv' )[0].offsetWidth)
		scale_factor = (scale_factor > 1) ? scale_factor : 1
		vidIn.height = Math.round(og_h / scale_factor)
		vidIn.width = Math.round(og_w / scale_factor)
		drawSurf.height = Math.round(og_h / scale_factor)
		drawSurf.width = Math.round(og_w / scale_factor)

		drawSurf.getContext('2d').lineWidth = 3
		drawSurf.getContext('2d').strokeStyle = 'red'
	})

	$( '#sendBut' ).on('click', () => {
		const drawSurf = $( '#drawSurf' )[0].getContext('2d')
		drawSurf.clearRect(0, 0, drawSurf.canvas.width, drawSurf.canvas.height)
		if (getStatusJson().fn == undefined) {
			// file is not on server, this is a new upload
			changeInstruction('Step 3/5: uploading to server. Upload progress shown below.')
			const r = new FormData()
			r.append('r', radii)
			r.append('v', $('#fileInput')[0].files[0])
			$.ajax('/upload/', { method: 'POST', data: r, dataType: 'text',
				contentType: false, processData: false,
				beforeSend: () => {
					drawable = 0
					$( 'progress' )[0].style.visibility = 'visible'
					$( '#sendBut' )[0].style.visibility = 'hidden'
				},
				success: (d) => {
					document.cookie = 'status='+d.split('\n')[0]+'; secure'
					$( 'progress' )[0].style.visibility = 'hidden'
					promptSubmit()
				},
				error: (d) => { 
					document.cookie = 'status='+d.responseText.split('\n')[0]+'; secure'
					if (getStatusJson().fn == undefined) {
						changeInstruction('Something went wrong submitting to the server. bugs or something')
						return
					}
					changeInstruction('Your video was uploaded to the server, but it couldn\'t find valid circle. Try again:')
					drawable = 1
				},
				xhr: () => {
					var myXhr = new window.XMLHttpRequest()
					myXhr.upload.addEventListener('progress', e => {
						if (e.lengthComputable) { 
							$('progress').attr({ value: e.loaded, max: e.total, })
						}
					}, false)
					return myXhr
				}
			})
		}
		else if (getStatusJson().r == undefined) {
			// this is a "try again submission"
			changeInstruction('Step 3/5: uploading to server. Upload progress shown below.')
			const r = new FormData()
			r.append('r', radii)
			r.append('v', $('#fileInput')[0].files[0])
		}
		else {
			// this is a derotation submission
			const status = getStatusJson()
			const r = new FormData()
			r.append('v', status.fn)
			r.append('r', status.r)
			r.append('x', status.x)
			r.append('y', status.y)
			r.append('rpm', $('#rpm')[0].value)
			$.ajax('/derot/', { method: 'POST', data: r, dataType: 'text',
				contentType: false, processData: false,
				beforeSend: () => {
					drawable = 0
					$( '#sendBut' )[0].style.visibility = 'hidden'
				},
				success: (d) => {
					var x = getStatusJson()
					x.waiting = Date.now()
					x.fn = d
					$( '#videoIn' ).attr('loop', true)
					$( '#videoIn' ).attr('src', '/return/' + x.fn)
					saveCookie(x)
					pollWait()
				},
				error: (d) => { 
					changeInstruction('Something went wrong submitting to the server. idk bugs or something')
				}
			})
		}
	})

	// non-opencv Canvas visual circle indicators
	$( '#drawSurf' ).on('mousedown', e => {
		if (drawable == 1) {
			const drawSurf = $( '#drawSurf' )[0].getContext('2d')
			drawSurf.clearRect(0, 0, drawSurf.canvas.width, drawSurf.canvas.height)
			center.x = e.offsetX; center.y = e.offsetY
			radii = -1
		}
	})

	$( '#drawSurf' ).on('mousemove', e => {
		if(radii == -1 && center.x && center.y && drawable == 1) {
			const drawSurf = $( '#drawSurf' )[0].getContext('2d')
			drawSurf.clearRect(0, 0, drawSurf.canvas.width, drawSurf.canvas.height)
			drawSurf.beginPath()
			drawSurf.moveTo(center.x, center.y)
			drawSurf.lineTo(e.offsetX, e.offsetY)
			drawSurf.closePath()
			drawSurf.stroke()
			drawSurf.beginPath()
			drawSurf.arc(center.x, center.y, 
				Math.sqrt(Math.pow(e.offsetX - center.x, 2) + Math.pow(e.offsetY - center.y, 2)), 
				0, 2 * Math.PI)
			drawSurf.stroke()
	}})

	$( '#drawSurf' ).on('mouseup', e => {
		if(radii == -1 && drawable == 1) {
			// need to multiply back to get original size
			radii = Math.sqrt(Math.pow(e.offsetX - center.x, 2) + Math.pow(e.offsetY - center.y, 2)) * scale_factor
			center.x = 0; center.y = 0
			$( '#sendBut' )[0].style.visibility = "visible"
	}})
})
