'use strict';

const changeInstruction = text => $( '#status' ).first().html(text)
const sleep = m => new Promise(r => setTimeout(r, m))

var center = {x: 0, y: 0}
var radii = undefined // set to values by user input and after circle
var scale_factor = undefined

// Event Listeners
$(window).on('load', () => {

	// element change listeners
	$( '#fileInput' ).on('change', e => {
		changeInstruction('Click-drag from center of rotation frame to edge of rotation frame.')
		$( '#videoIn' ).attr('src', URL.createObjectURL(e.target.files[0]))
	})

	$( '#videoIn' ).on('loadeddata', e => {
		const vidIn = $( '#videoIn' )[0]
		const canv = $( '#drawSurf' )[0]
		let og_h = vidIn.videoHeight, og_w = vidIn.videoWidth
		scale_factor = Math.max(og_h, og_w) / Math.min(window.screen.availHeight, window.screen.availWidth)
		scale_factor = (scale_factor > 1) ? scale_factor : 1
		vidIn.height = Math.round(og_h / scale_factor)
		vidIn.width = Math.round(og_w / scale_factor)
		drawSurf.height = Math.round(og_h / scale_factor)
		drawSurf.width = Math.round(og_w / scale_factor)

		drawSurf.getContext('2d').lineWidth = 3
		drawSurf.getContext('2d').strokeStyle = 'red'
	})

	$( '#sendBut' ).on('click', e => {
		const drawSurf = $( '#drawSurf' )[0].getContext('2d')
		const r = new FormData()
		r.append('v', $('#fileInput')[0].files[0])
		r.append('r', radii)
		r.append('rpm', Number($('#rpm')[0].value))
		changeInstruction('we are now sending your video to a mystery epss server at ucla \n no one can save you now')
		drawSurf.clearRect(0, 0, drawSurf.canvas.width, drawSurf.canvas.height)
		$( '#videoIn' )[0].removeAttribute('src')
		$( '#videoIn' )[0].load()
		$( '#sendBut' )[0].style.visibility = 'hidden'
		fetch('/upload', {method: 'POST', body: r})
			.then(r => r.text())
			.then(t => {
				console.log(t)
				$( '#videoIn' ).attr('src', t)
				$( '#videoIn' )[0].play()
			})
	})


	// Regular Canvas visual circle indicators

	$( '#drawSurf' ).on('mousedown', e => {
		const drawSurf = $( '#drawSurf' )[0].getContext('2d')
		drawSurf.clearRect(0, 0, drawSurf.canvas.width, drawSurf.canvas.height)
		center.x = e.offsetX; center.y = e.offsetY
		radii = -1
	})

	$( '#drawSurf' ).on('mousemove', e => {
		if(radii == -1 && center.x && center.y) {
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
		if(radii == -1) {
			// need to multiply back to get original size
			radii = Math.sqrt(Math.pow(e.offsetX - center.x, 2) + 
								Math.pow(e.offsetY - center.y, 2)) * scale_factor
			center.x = 0; center.y = 0
			$( '#sendBut' )[0].style.visibility = "visible"
	}})
})
