function renderCeilingButtonUpdate(data) {
  for (var keyid in data) {
    on_btn = document.getElementById("onbtn_"+keyid);
    off_btn = document.getElementById("offbtn_"+keyid);
    if (on_btn && off_btn)
    {
      on_btn.className = "onbutton";
      off_btn.className = "offbutton";
      if (data[keyid])
      { on_btn.className += " enableborder"; }
      else
      { off_btn.className += " enableborder"; }
    }
  }
}

function renderRFIRButtonUpdate(data) {
  var snd_btn = document.getElementById(data.name);
  if (snd_btn)
  {
    var origclass = snd_btn.className;
    snd_btn.className += " activatedbtn";

    setTimeout(function(){
      snd_btn.className = origclass;
    },900);
  }
}

function updateButtons(uri) {
  var req = new XMLHttpRequest;
  req.overrideMimeType("application/json");
  req.open("GET", uri, true);
  req.onload  = function() {
    if (req.status != 200) {
      return;
    }
    var data = JSON.parse(req.responseText);
    renderCeilingButtonUpdate(data);
  };
  req.setRequestHeader("googlechromefix","");
  req.send(null);
}

function sendMultiButton( str ) {
  url = "/cgi-bin/mswitch.cgi?"+str;
  updateButtons(url);
}

function sendYmhButton( btn ) {
  //alert(btn);
  document.getElementById('indicator').style.backgroundColor="red";
  document.getElementById('commandlabel').innerHTML=btn;
  sendMultiButton(btn+"=1");
  document.getElementById('indicator').style.backgroundColor="white";
  document.getElementById('commandlabel').innerHTML='&nbsp;';
}

function remoteKeyboard( e ) {
  e = e || window.event;
  switch( e.keyCode )
  {
    //case 81: sendYmhButton( ( e.altKey ? 'REBOOT' : 'w' ) ); break;       // ALT-Q = reboot, Q = Power
    case 38: sendYmhButton( 'ymhprgup' ); break; //up
    case 37: sendYmhButton( 'ymhminus' ); break; //left
    //case 13: sendYmhButton( '' ); break;  //enter
    case 39: sendYmhButton( 'ymhplus' ); break; //right
    case 40: sendYmhButton( 'ymhprgdown' ); break; //down
    //case  8: sendYmhButton( 'T_back' ); break; //backspace
    //case 27: sendYmhButton( 't_stop' ); break;    // ESC
    case 77: sendYmhButton( 'ymhmenu' ); break;     // M
    case 84: sendYmhButton( 'ymhtimelevel' ); break;        // T
    //case 32: sendYmhButton( '' ); break;  // Space
    case 83: sendYmhButton( 'ymhsleep' ); break;            // S
    case 36: sendYmhButton( 'ymhmute' ); break; //pos1
    case 33: sendYmhButton( 'ymhvolup' ); break;    // P = 80, PAGEUP = 33
    case 43: sendYmhButton( 'ymhvolup' ); break;    // +
    case 34: sendYmhButton( 'ymhvoldown' ); break;  // N = 78, PAGEDOWN = 34
    case 45: sendYmhButton( 'ymhvoldown' ); break;  // -
    case 46: sendYmhButton( 'ymhtunplus' ); break;  // .
    case 44: sendYmhButton( 'ymhtunminus' ); break; // ,
    case 59: sendYmhButton( 'ymhtunabcde' ); break; // ;
    case 69: sendYmhButton( 'ymheffect' ); break;   // E
    case 80: sendYmhButton( 'ymhpower' ); break;    // P
    case 48: sendYmhButton( 'ymhtest' ); break;     // 0
    case 49: sendYmhButton( 'ymhcd' ); break;       // 1
    case 50: sendYmhButton( 'ymhtuner' ); break;    // 2
    case 51: sendYmhButton( 'ymhtape' ); break;     // 3
    case 52: sendYmhButton( 'ymhwdtv' ); break;     // 4
    case 53: sendYmhButton( 'ymhsattv' ); break;    // 5
    case 54: sendYmhButton( 'ymhvcr' ); break;      // 6
    case 55: sendYmhButton( 'ymh7' ); break;        // 7
    case 56: sendYmhButton( 'ymhaux' ); break;      // 8
    case 57: sendYmhButton( 'ymhextdec' ); break;   // 9
  }
}

document.onkeydown = remoteKeyboard;

var fancycolorpicker_apply_name="ceiling1";
function popupFancyColorPicker(event) {
  var x = event.pageX;
  var y = event.pageY;
  $("#fancycolorpicker").css("left",x).css("top",y).css("visibility","visible").animate(1000);
  fancycolorpicker_apply_name = this.getAttribute("name");
}


var webSocketUrl = 'ws://'+window.location.hostname+'/sock';
var cgiUrl = '/cgi-bin/mswitch.cgi';
//var cgiUrl = 'fake.json';

var webSocketSupport = null;

setLedPipePattern = function(){};

function openWebSocket(webSocketUrl) {
  ws.registerContext("ceilinglights",renderCeilingButtonUpdate);
  ws.registerContext("wbp",renderRFIRButtonUpdate);
  ws.open(webSocketUrl);
}

(function() {
  webSocketSupport = hasWebSocketSupport();
  if (webSocketSupport) {
    openWebSocket(webSocketUrl);
  } else {
    updateButtons("/cgi-bin/mswitch.cgi");
    setInterval("updateButtons(\"/cgi-bin/mswitch.cgi\");", 30*1000);
  }

  var onbtns = document.getElementsByClassName('onbutton');
  var offbtns = document.getElementsByClassName('offbutton');
  var fancypresetbtns = document.getElementsByClassName('fancylightpresetbutton');
  var ledpipepresetbtns = document.getElementsByClassName('ledpipepresetbutton');
  for (var i = 0; i < onbtns.length; i++) {
    onbtns[i].addEventListener('click', function() {
      var id = this.getAttribute('id');
      if (!id) {
        return;
      }
      var name = id.substr(id.indexOf("_")+1);
      if (webSocketSupport) {
        ws.send("LightCtrlActionOnName",{Name:name, Action:'1'});
      } else {
        sendMultiButton(name+"=1");
      }
    });
  }
  for (var i = 0; i < offbtns.length; i++) {
    offbtns[i].addEventListener('click', function() {
      var id = this.getAttribute('id');
      if (!id) {
        return;
      }
      var name = id.substr(id.indexOf("_")+1);
      if (webSocketSupport) {
        ws.send("LightCtrlActionOnName",{Name:name, Action:'0'});
      } else {
        sendMultiButton(name+"=0");
      }
    });
  }
  for (var i = 0; i < fancypresetbtns.length; i++) {
    fancypresetbtns[i].addEventListener('click', function() {
      var name = this.getAttribute("name");
      if (!name) { return;  }
      var R = parseInt(this.getAttribute("ledr")) || 0;
      var G = parseInt(this.getAttribute("ledg")) || 0;
      var B = parseInt(this.getAttribute("ledb")) || 0;
      var CW = parseInt(this.getAttribute("ledcw")) || 0;
      var WW = parseInt(this.getAttribute("ledww")) || 0;
      var settings = {r:R,g:G,b:B,cw:CW,ww:WW,fade:{}};
      if (webSocketSupport) {
        ws.send("FancyLight",{name:name, setting:settings});
      } else {
        var req = new XMLHttpRequest;
        req.overrideMimeType("application/json");
        req.open("GET", "/cgi-bin/fancylight.cgi?name="+name+"&setting="+JSON.stringify(settings), true);
        req.onload  = function() {
          if (req.status != 200) {
            return;
          }
        };
        req.setRequestHeader("googlechromefix","");
        req.send(null);
      }
    });
  }
  $(".fancylightcolourtempselectorbutton").click(popupFancyColorPicker);
  $("#fancycolorpicker_close_button").click(function(event){$("#fancycolorpicker").css("visibility","hidden")});
  $("#fancycolorpicker_apply_button").click(function(event){
      var R = parseInt($('#R input').val()) || 0;
      var G = parseInt($('#G input').val()) || 0;
      var B = parseInt($('#B input').val()) || 0;
      var CW = parseInt($('#CW input').val()) || 0;
      var WW = parseInt($('#WW input').val()) || 0;
      var settings = {r:R,g:G,b:B,cw:CW,ww:WW,fade:{}};
      if (webSocketSupport) {
        ws.send("FancyLight",{name:fancycolorpicker_apply_name, setting:settings});
      } else {
        var req = new XMLHttpRequest;
        req.overrideMimeType("application/json");
        req.open("GET", "/cgi-bin/fancylight.cgi?name="+fancycolorpicker_apply_name+"&setting="+JSON.stringify(settings), true);
        req.onload  = function() {
          if (req.status != 200) {
            return;
          }
        };
        req.setRequestHeader("googlechromefix","");
        req.send(null);
      }
  });
  //draw color picker canvases
  init_colour_temp_picker();
  //define setLedPipePattern(objdata)
  if (webSocketSupport) {
      setLedPipePattern=function(objdata) {
        ws.send("SetPipeLEDsPattern",objdata);
      }
  } else {
      setLedPipePattern=function(objdata) {
        var req = new XMLHttpRequest;
        req.overrideMimeType("application/json");
        req.open("GET", "/cgi-bin/ledpipe.cgi?data="+JSON.stringify(objdata), true);
        req.onload  = function() {
          if (req.status != 200) {
            return;
          }
        };
        req.setRequestHeader("googlechromefix","");
        req.send(null);
      }
  }
for (var i = 0; i < ledpipepresetbtns.length; i++) {
    ledpipepresetbtns[i].addEventListener('click', function() {
      var pipepattern = this.getAttribute("pipepattern");
      if (!pipepattern) { return;  }
      var hue = parseInt(this.getAttribute("pipehue")) || undefined;
      var brightness = parseInt(this.getAttribute("pipebrightness")) || undefined;
      var speed = parseInt(this.getAttribute("pipespeed")) || undefined;
      var arg = parseInt(this.getAttribute("pipearg")) || undefined;
      var arg1 = parseInt(this.getAttribute("pipearg1")) || undefined;
      var data = {pattern:pipepattern, hue:hue, brightness:brightness, speed:speed, arg:arg, arg1:arg1};
      setLedPipePattern(data);
      if (brightness) {
        document.getElementById("pipebrightness").value=brightness;
      }
      if (hue) {
        document.getElementById("pipehue").value=hue;
      }
      if (speed) {
        console.log(speed)
        document.getElementById("pipespeed").value=speed;
      }      
    });
  }

  //TODO: support yamahabuttons and rf433 buttons via websocket

})();