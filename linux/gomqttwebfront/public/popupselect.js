//// (c) Bernhard Tittelbach, 2017
//// License: MIT

function isMobileBrowser() {
	var a = navigator.userAgent||navigator.vendor||window.opera;
	return (/(android|bb\d+|meego).+mobile|avantgo|bada\/|blackberry|blazer|compal|elaine|fennec|hiptop|iemobile|ip(hone|od)|iris|kindle|lge |maemo|midp|mmp|mobile.+firefox|netfront|opera m(ob|in)i|palm( os)?|phone|p(ixi|re)\/|plucker|pocket|psp|series(4|6)0|symbian|treo|up\.(browser|link)|vodafone|wap|windows ce|xda|xiino/i.test(a)||/1207|6310|6590|3gso|4thp|50[1-6]i|770s|802s|a wa|abac|ac(er|oo|s\-)|ai(ko|rn)|al(av|ca|co)|amoi|an(ex|ny|yw)|aptu|ar(ch|go)|as(te|us)|attw|au(di|\-m|r |s )|avan|be(ck|ll|nq)|bi(lb|rd)|bl(ac|az)|br(e|v)w|bumb|bw\-(n|u)|c55\/|capi|ccwa|cdm\-|cell|chtm|cldc|cmd\-|co(mp|nd)|craw|da(it|ll|ng)|dbte|dc\-s|devi|dica|dmob|do(c|p)o|ds(12|\-d)|el(49|ai)|em(l2|ul)|er(ic|k0)|esl8|ez([4-7]0|os|wa|ze)|fetc|fly(\-|_)|g1 u|g560|gene|gf\-5|g\-mo|go(\.w|od)|gr(ad|un)|haie|hcit|hd\-(m|p|t)|hei\-|hi(pt|ta)|hp( i|ip)|hs\-c|ht(c(\-| |_|a|g|p|s|t)|tp)|hu(aw|tc)|i\-(20|go|ma)|i230|iac( |\-|\/)|ibro|idea|ig01|ikom|im1k|inno|ipaq|iris|ja(t|v)a|jbro|jemu|jigs|kddi|keji|kgt( |\/)|klon|kpt |kwc\-|kyo(c|k)|le(no|xi)|lg( g|\/(k|l|u)|50|54|\-[a-w])|libw|lynx|m1\-w|m3ga|m50\/|ma(te|ui|xo)|mc(01|21|ca)|m\-cr|me(rc|ri)|mi(o8|oa|ts)|mmef|mo(01|02|bi|de|do|t(\-| |o|v)|zz)|mt(50|p1|v )|mwbp|mywa|n10[0-2]|n20[2-3]|n30(0|2)|n50(0|2|5)|n7(0(0|1)|10)|ne((c|m)\-|on|tf|wf|wg|wt)|nok(6|i)|nzph|o2im|op(ti|wv)|oran|owg1|p800|pan(a|d|t)|pdxg|pg(13|\-([1-8]|c))|phil|pire|pl(ay|uc)|pn\-2|po(ck|rt|se)|prox|psio|pt\-g|qa\-a|qc(07|12|21|32|60|\-[2-7]|i\-)|qtek|r380|r600|raks|rim9|ro(ve|zo)|s55\/|sa(ge|ma|mm|ms|ny|va)|sc(01|h\-|oo|p\-)|sdk\/|se(c(\-|0|1)|47|mc|nd|ri)|sgh\-|shar|sie(\-|m)|sk\-0|sl(45|id)|sm(al|ar|b3|it|t5)|so(ft|ny)|sp(01|h\-|v\-|v )|sy(01|mb)|t2(18|50)|t6(00|10|18)|ta(gt|lk)|tcl\-|tdg\-|tel(i|m)|tim\-|t\-mo|to(pl|sh)|ts(70|m\-|m3|m5)|tx\-9|up(\.b|g1|si)|utst|v400|v750|veri|vi(rg|te)|vk(40|5[0-3]|\-v)|vm40|voda|vulc|vx(52|53|60|61|70|80|81|83|85|98)|w3c(\-| )|webc|whit|wi(g |nc|nw)|wmlb|wonu|x700|yas\-|your|zeto|zte\-/i.test(a.substr(0,4)));
}

var popupselect = {
	target:undefined,
	openinprogress:false,
	options: {
		class_triggerpopup:"popupselect_trigger",
		class_popupoverlay:"popupselect_overlay",
		class_option:"popupselect_option",
		attribute_specifiying_elementid_with_options:"optionsid",
	},

	popupselectOpen:function(event) {
		event.stopPropagation();
		if (popupselect.openinprogress==true)
		{return;}
		popupselect.openinprogress=true;
		popupselect.target = event.target;
		var x,y;
		try {
			x = event.touches[0].pageX;
			y = event.touches[0].pageY;
		} catch (e) {
			x = event.pageX;
			y = event.pageY;
		}
		var optionselementid = event.target.getAttribute("optionsid");
		var oelem = $(document.getElementById(optionselementid));
		var optionscopyattr = event.target.getAttribute("optionscopyattr");
		if (optionscopyattr) {
			var attrvaluetocopy = event.target.getAttribute(optionscopyattr);
			if (attrvaluetocopy) {
				$(oelem).find("."+popupselect.options.class_option).attr(optionscopyattr,attrvaluetocopy);
			}
		}
		oelem.css("left",Math.trunc(x - oelem.width()/2)+"px").css("top",Math.trunc(y - oelem.height()/2)+"px").css("visibility","visible");
		popupselect.openinprogress=false;
	},

	popupselectSelect:function(event) {
		event.stopPropagation();
		//Array.from(document.getElementsByClassName(popupselect.options.class_popupoverlay)).forEach(function(elem) {elem.style.visibility="hidden";elem.style.display="none";});
		var allmypopups = document.getElementsByClassName(popupselect.options.class_popupoverlay)
		for (var i=0; i<allmypopups.length; i++) {
			if (allmypopups[i]) { allmypopups[i].style.visibility="hidden"; }
		}
		var selectedbtn = $(event.target);
		if (selectedbtn.hasClass(popupselect.options.class_option))
		{
			$(popupselect.target).html(selectedbtn.html());
			$(popupselect.target).attr("style",selectedbtn.attr("style"));
			var fun = $(event.target).data("popupfunc");
			if (fun) {fun(event);}
		}
	},

	init: function(options) {
		if (options)
		{
			$.extend(this.options,options)
		}
		if (isMobileBrowser()) {
/*			$("."+this.options.class_triggerpopup).on("click",this.popupselectOpen);
			$("."+this.options.class_option).on("click",this.popupselectSelect);*/
		} else {
			$("."+this.options.class_triggerpopup).on("mousedown",this.popupselectOpen);
			//$(document).on("mouseup",this.popupselectSelect);
		}
		$("."+this.options.class_triggerpopup).on("touchstart",this.popupselectOpen);
		$("."+this.options.class_triggerpopup).on("click",this.popupselectOpen);
		//$("."+this.options.class_option).on("click",this.popupselectSelect);
		//$(document).on("touchend",this.popupselectSelect);
		//$(document).on("mouseup",this.popupselectSelect);
		$(document).on("click",this.popupselectSelect);
	},

	addSelectHandler:function(elem, func) {
		if ($(elem).hasClass(this.options.class_option) && typeof(func) == "function")
		{
			$(elem).data("popupfunc",func);
		}
	},

	addSelectHandlerToAll:function(func) {
		if (typeof(func) == "function")
		{
			$("."+this.options.class_option).each(function(elem, idx) {$(elem).data("popupfunc",func);});
		}
	},
}
