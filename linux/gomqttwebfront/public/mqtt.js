
var mqtttopic_activatescript = "action/ceilingscripts/activatescript";
var mqtttopic_pipeledpattern = "action/PipeLEDs/pattern";
function mqtttopic_golightctrl(lightname) {
  return "action/GoLightCtrl/"+lightname;
}
function mqtttopic_fancylight(fancyid) {
  return "action/"+fancyid+"/light";
}

function sendMQTT_XHTTP(ctx, data) {
  var req = new XMLHttpRequest;
  req.open("POST", cgiUrl, true);
  req.onload = function() {
    if (req.status != 200) {
      return;
    }
    var data = JSON.parse(req.responseText);
    setButtonStates(data);
  };
  var param = "Ctx=" + encodeURIComponent(ctx);
  params = params + "&Data="+encodeURIComponent(data);
  params = params.replace(/%20/g, '+');
  req.overrideMimeType("application/json");
  req.setRequestHeader("googlechromefix","");
  req.setRequestHeader("Content-length", params.length);
  req.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  req.setRequestHeader("Connection", "close");
  req.send(params);
}

function sendMQTT(ctx, data) {
  if (webSocketSupport) {
    ws.send(ctx,data);
  } else {
    sendMQTT_XHTTP(ctx, data);
  }
}

function eventOnRawMqttElement(event) {
  var topic = event.target.getAttribute("topic");
  var payloadobj = JSON.parse(event.target.getAttribute("payload"));
  if (payloadobj) {
    sendMQTT(topic, payloadobj);
  }
}

function eventOnFancyLightPresent(event) {
  var name = event.target.getAttribute("name");
  if (!name) { return;  }
  var R = parseInt(event.target.getAttribute("ledr")) || 0;
  var G = parseInt(event.target.getAttribute("ledg")) || 0;
  var B = parseInt(event.target.getAttribute("ledb")) || 0;
  var CW = parseInt(event.target.getAttribute("ledcw")) || 0;
  var WW = parseInt(event.target.getAttribute("ledww")) || 0;
  var settings = {r:R,g:G,b:B,cw:CW,ww:WW,fade:{}};
  sendMQTT("action/"+name+"/light",settings);
};

//takes function with signature (fancyid, data)
//and calls it if fancy light updates externally
function registerFunctionForFancyLightUpdate(fun) {
  ["ceiling1","ceiling2","ceiling3","ceiling4","ceiling5","ceiling6","ceilng7","ceiling8","ceiling9","ceilingAll"].forEach(function(fancyid) {
    ws.registerContext("action/"+fancyid+"/light",function(fancyid){
      return function(data) {
        fun(fancyid, data);
      }
    }(fancyid));
  });
}

function calcDayLevelFromColor(data)
{
  var value = Math.min(1000,data.cw + data.ww + data.r*3)/1000.0;
  var day_factor;
  if (data.cw+data.ww == 0) {
    day_factor = 0.0;
  } else {
    day_factor = data.cw * 1.0 / (data.cw+data.ww) - data.r / 1000.0;
    day_factor = Math.min(1.0,Math.max(-1.0,  day_factor ));
  }
  return {"balance":day_factor, "intensity":value};
}

function calcColorFromDayLevel(day_factor, value)
{
  var day_factor = Math.min(1.0,Math.max(-1.0,day_factor));
  var r = 1000 * value * Math.max(0.0, -1.0 * day_factor);
  var b = 0;
  var cw = 1000 * value * Math.max(0.0, day_factor);
  var ww = Math.max(0,1000 * value - cw - (r/3));
  return {"r":Math.trunc(r), "b":Math.trunc(b), "cw":Math.trunc(cw), "ww":Math.trunc(ww)};
}

function calcCompoundRGB(data)
{
  var warmwhite_representation  = [255.0/255, 250.0/255, 192/255];
  var coldwhite_representation = [220.0/255, 220.0/255, 255/255];

  //fill data with zero if missing
  data.r = data.r || 0;
  data.g = data.g || 0;
  data.b = data.b || 0;
  data.ww = data.ww || 0;
  data.cw = data.cw || 0;

  var r_factor = 1;
  var g_factor = 3; //green 3 times as bright as red
  var b_factor = g_factor*3; //blue 2 times as bright as green
  var ww_factor = 44; //yes warmwhite is about 2 times as bright as cw and 44 times as bright as red
  var cw_factor = 22;

  var r = data.r*r_factor + data.ww*warmwhite_representation[0]*ww_factor + data.cw*coldwhite_representation[0]*cw_factor;
  var g = data.g*g_factor + data.ww*warmwhite_representation[1]*ww_factor + data.cw*coldwhite_representation[1]*cw_factor;
  var b = data.b*b_factor + data.ww*warmwhite_representation[2]*ww_factor + data.cw*coldwhite_representation[2]*cw_factor;

  var maximum = Math.max(r,g,b);

  //now normalize
  r = r * 255.0 / maximum;
  g = g * 255.0 / maximum;
  b = b * 255.0 / maximum;

  data.compound_r = Math.min(255,Math.floor(r));
  data.compound_g = Math.min(255,Math.floor(g));
  data.compound_b = Math.min(255,Math.floor(b));  
}