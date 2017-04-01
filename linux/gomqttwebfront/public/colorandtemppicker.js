// This blog post was a big help:
// http://www.nixtu.info/2014/12/html5-canvas-gradients-rectangle.html

// http://www.javascripter.net/faq/rgbtohex.htm
function rgbToHex(R,G,B) {return toHex(R)+toHex(G)+toHex(B)}

function toHex(n) {
  n = parseInt(n,10);
  if (isNaN(n)) return "00";
  n = Math.max(0,Math.min(n,255));
  return "0123456789ABCDEF".charAt((n-n%16)/16)  + "0123456789ABCDEF".charAt(n%16);
}

function rgb2hsv (r,g,b) {
	var computedH = 0;
	var computedS = 0;
	var computedV = 0;

	//remove spaces from input RGB values, convert to int
	var r = parseInt( (''+r).replace(/\s/g,''),10 ); 
	var g = parseInt( (''+g).replace(/\s/g,''),10 ); 
	var b = parseInt( (''+b).replace(/\s/g,''),10 ); 

	if ( r==null || g==null || b==null ||
	 isNaN(r) || isNaN(g)|| isNaN(b) ) {
	alert ('Please enter numeric RGB values!');
	return;
	}
	if (r<0 || g<0 || b<0 || r>255 || g>255 || b>255) {
	alert ('RGB values must be in the range 0 to 255.');
	return;
	}
	r=r/255; g=g/255; b=b/255;
	var minRGB = Math.min(r,Math.min(g,b));
	var maxRGB = Math.max(r,Math.max(g,b));

	// Black-gray-white
	if (minRGB==maxRGB) {
	computedV = minRGB;
	return [0,0,computedV];
	}

	// Colors other than black-gray-white:
	var d = (r==minRGB) ? g-b : ((b==minRGB) ? r-g : b-r);
	var h = (r==minRGB) ? 3 : ((b==minRGB) ? 1 : 5);
	computedH = 60*(h - d/(maxRGB - minRGB));
	computedS = (maxRGB - minRGB)/maxRGB;
	computedV = maxRGB;
	return [computedH,computedS,computedV];
}

function hsv2rgb(h,s,v) {
	var range=0;
	var f=0.0;
	//let's use the same, slighlty adjusted ranges, as for the canvas drawing
	if (h > 0.835) {range=5; f=(h-0.835)/(1-0.835)}
	else if (h > 0.665) {range=4;f=(h-0.665)/(0.835-0.665)}
	else if (h > 0.470) {range=3;f=(h-0.470)/(0.665-0.470)}
	else if (h > 0.333) {range=2;f=(h-0.333)/(0.470-0.333)}
	else if (h > 0.166) {range=1;f=(h-0.166)/(0.333-0.166)}
	else {range = 0; f=h/0.166}

	var v = v * 255.0;
	var p = v * (1 - s);
	var q = v * (1 - f * s);
	var t = v * (1 - (1 - f) * s);
	console.log(range, h, f);
	switch (range)
	{
	    case 0:
	        return [v, t, p];
	    case 1:
	        return [q, v, p];
	    case 2:
	        return [p, v, t];
	    case 3:
	        return [p, q, v];
	    case 4:
	        return [t, p, v];
	}
	return [v, p, q];
}

function drawcolourtemppicker(elemid) {
    var canvas = document.getElementById(elemid);
    var canvas2d = canvas.getContext('2d');
    console.log(canvas);

    quadGradient(canvas, canvas2d, {
      topLeft: [1,1,1,1],
      topRight: [1, 0xfa/0xff, 0xc0/0xff, 1],
      bottomLeft: [71/0xff, 171/0xff, 1, 1],
      bottomRight: [0, 0, 0, 1]
    });

	var pickcolour = function(event) {
	  if (event.type == "mousemove" && event.buttons != 1) {return;}
	  // getting user coordinates
	  //var untransY = event.pageY - this.offsetTop;
	  var transX = (typeof event.offsetX == "number") ? event.offsetX : event.layerX || 0;
	  var transY = (typeof event.offsetY == "number") ? event.offsetY : event.layerY || 0;

	  // getting image data and RGB values
	  // var CWwhole = Math.trunc((rotatedwidth - x)*1000/rotatedwidth);
	  // var WWwhole = Math.trunc(x*1000/rotatedwidth);
	  var squarewidth = $(canvas).width();
	  var diamondwidth = Math.sqrt(squarewidth*squarewidth*2);
	  var CW = Math.trunc(Math.max(0,Math.min(1000,(squarewidth - transX)   *1000/squarewidth)));
	  var WW = Math.trunc(Math.max(0,Math.min(1000,(squarewidth - transY)   *1000/squarewidth))); //0...1000
	  //var brightness = (diamondwidth - untransY) *1000/diamondwidth;
	  var brightness = 1000 - Math.trunc(Math.sqrt(transX*transX+transY*transY)*1000/diamondwidth);
	  var ctempmix = WW*1000/CW/2;	
	  // making the color the value of the input
	  $('#CW input').val(CW);
	  $('#WW input').val(WW);
	  $('#CW div.colorlevel').css("width",CW/10+"%");
	  $('#WW div.colorlevel').css("width",WW/10+"%");
	  $('#WhiteBrightness input').val(brightness);
	  $('#WhiteTemp input').val(ctempmix);
	  var img_data = canvas2d.getImageData(transX, transY, 1, 1);
	  $('#cwwwoverrgbcolor').css("background-color","rgb("+img_data.data[0]+","+img_data.data[1]+","+img_data.data[2]+")").css("opacity",brightness/1000.0);
	  $('#cwwwcolor').css("background-color","rgb("+img_data.data[0]+","+img_data.data[1]+","+img_data.data[2]+")");
	};
	$(canvas).click(pickcolour);
	$(canvas).mousemove(pickcolour);

}

var lower_black_percent = 0.95;

function drawcolourpicker(elemid) {
	var canvas = document.getElementById(elemid);
	var canvas2d = canvas.getContext('2d');

	rainbowHSLpicker(canvas, canvas2d);
	var pickcolour = function(event){
	  if (event.type == "mousemove" && event.buttons != 1) {return;}
	  // getting user coordinates
	  var x = (typeof event.offsetX == "number") ? event.offsetX : event.layerX || 0;
	  var y = (typeof event.offsetY == "number") ? event.offsetY : event.layerY || 0;
	  // getting image data and RGB values
	  //var img_data = canvas2d.getImageData(0,0, this.width,this.height);
	  //var pxoffset = (y*img_data.width+x)*4;
	  //let's  cheat a bit... make upper bit H(S)L picker and lower part how we think HS(V) picker should work
	  if (y <= this.height/2 || y > this.height*lower_black_percent) { //get HSL from image
		  var img_data = canvas2d.getImageData(x, y, 1, 1);
		  var pxoffset=0;
		  var R = img_data.data[pxoffset+0];
		  var G = img_data.data[pxoffset+1];
		  var B = img_data.data[pxoffset+2];
	  } else { //calc HSV from coordiantes
	  	heightminusblack = Math.trunc(this.height*lower_black_percent);
	  	var h = x / this.width;
	  	var s = 1.0;
	  	var v = (heightminusblack - y) / heightminusblack * 2.15;
	  	var rgb = hsv2rgb(h,s,v);
	  	var R = rgb[0];
	  	var G = rgb[1];
	  	var B = rgb[2];
	  }
	  var r1k = Math.trunc(Math.max(0,Math.min(1000,R*1000/255)));
	  var g1k = Math.trunc(Math.max(0,Math.min(1000,G*1000/255)));
	  var b1k = Math.trunc(Math.max(0,Math.min(1000,B*1000/255)));
	  // making the color the value of the input
	  $('#R input').val(r1k);
	  $('#G input').val(g1k);
	  $('#B input').val(b1k);
	  $('#R div.colorlevel').css("width",r1k/10+"%");
	  $('#G div.colorlevel').css("width",g1k/10+"%");
	  $('#B div.colorlevel').css("width",b1k/10+"%");
	  $('#rgbcolor').css("background-color","rgb("+R+","+G+","+B+")");
	};
	$(canvas).click(pickcolour);
	$(canvas).mousemove(pickcolour);
}

function init_colour_temp_picker() {
	drawcolourtemppicker("pickcolourtemp");
	drawcolourpicker("pickcolour");

	changecolourlevel = function(event){if (this.value < 0 || this.value > 1000) {return;}; $(this).siblings().find('.colorlevel').css("width",this.value/10+"%");}
	$('#R input').change(changecolourlevel);
	$('#G input').change(changecolourlevel);
	$('#B input').change(changecolourlevel);
	$('#WW input').change(changecolourlevel);
	$('#CW input').change(changecolourlevel);
	changetextfromlevel = function(event){
	  if (event.type == "mousemove" && event.buttons != 1) {return;}
	  var x = (typeof event.offsetX == "number") ? event.offsetX : event.layerX || 0;
	  // var y = event.pageY - this.offsetTop;
	  var promille = Math.trunc(Math.max(0,Math.min(1000,x*1000/this.offsetWidth)));
	  $(this).siblings("input").val(promille);
	  $(this).find('.colorlevel').css("width",x);
	};
	$('#R div.colorlevelcontainer').mousemove(changetextfromlevel);
	$('#G div.colorlevelcontainer').mousemove(changetextfromlevel);
	$('#B div.colorlevelcontainer').mousemove(changetextfromlevel);
	$('#WW div.colorlevelcontainer').mousemove(changetextfromlevel);
	$('#CW div.colorlevelcontainer').mousemove(changetextfromlevel);
	$('#R div.colorlevelcontainer').click(changetextfromlevel);
	$('#G div.colorlevelcontainer').click(changetextfromlevel);
	$('#B div.colorlevelcontainer').click(changetextfromlevel);
	$('#WW div.colorlevelcontainer').click(changetextfromlevel);
	$('#CW div.colorlevelcontainer').click(changetextfromlevel);
}

function rainbowHSLpicker(canvas,ctx) { 
    var w = canvas.width;
    var h = canvas.height;
    var gradient;
  
  	gradient = ctx.createLinearGradient(0,0,w,0);
  	gradient.addColorStop(0,"rgba(255, 0, 0, 1)");
  	gradient.addColorStop(0.166,"rgba(255, 255, 0, 1)");
  	gradient.addColorStop(0.333,"rgba(0, 255, 0, 1)");
  	gradient.addColorStop(0.47,"rgba(0, 255, 255, 1)");
  	gradient.addColorStop(0.665,"rgba(0, 0, 255, 1)");
  	gradient.addColorStop(0.835,"rgba(255, 0, 255, 1)");
  	gradient.addColorStop(1.00,"rgba(255, 0, 0, 1)");
  	ctx.fillStyle = gradient;
    ctx.fillRect(0,0,w,h);
  	gradient = ctx.createLinearGradient(0,0,0,h);
  	gradient.addColorStop(0,"rgba(255, 255, 255, 1)");
  	gradient.addColorStop(0.05,"rgba(255, 255, 255, 1)");
  	gradient.addColorStop(0.35,"rgba(255, 255, 255, 0)");
  	gradient.addColorStop(0.65,"rgba(255,255,255, 0)");
  	gradient.addColorStop(lower_black_percent,"rgba(0, 0, 0, 1)");
  	gradient.addColorStop(1.0,"rgba(0, 0, 0, 1)");
  	ctx.fillStyle = gradient;
    ctx.fillRect(0,0,w,h);  	
}

function quadGradient(canvas, ctx, corners) { 
    var w = canvas.width;
    var h = canvas.height;
    var gradient, startColor, endColor, fac;
  
    for(var i = 0; i < h; i++) {
        gradient = ctx.createLinearGradient(0, i, w, i);
        fac = i / (h - 1);

        startColor = arrayToRGBA(
          lerp(corners.topLeft, corners.bottomLeft, fac)
        );
        endColor = arrayToRGBA(
          lerp(corners.topRight, corners.bottomRight, fac)
        );
      
        gradient.addColorStop(0, startColor);
        gradient.addColorStop(1, endColor);
      
        ctx.fillStyle = gradient;
        ctx.fillRect(0, i, w, i);
    }
}

function arrayToRGBA(arr) {
    var ret = arr.map(function(v) {
        // map to [0, 255] and clamp
        return Math.max(Math.min(Math.round(v * 255), 255), 0);
    });

    // alpha should retain its value
    ret[3] = arr[3];
  
    return 'rgba(' + ret.join(',') + ')';
}

function lerp(a, b, fac) {
    return a.map(function(v, i) {
        return v * (1 - fac) + b[i] * fac;
    });
}