/* Formatter: Adds dynamicly needed things for styling purposes in manual */

if (document.getElementsByClassName("header").length)
  for each (var n in document.getElementsByClassName("header")[0].childNodes){
    if (n.nodeName && n.nodeName.toLowerCase() == "h1"){
      var d = n.parentNode.insertBefore(document.createElement("div"), n);
      d.className = "js";
      n.className = "js";
      d.appendChild(n);
      var n1 = n.cloneNode(true);
      n1.className = "js outline";
      d.appendChild(n1);
      var n2 = n.cloneNode(true);
      n2.className = "js filling";
      d.appendChild(n2);
      break;
    }
  }

for each (var n in document.getElementsByTagName("a")){
  if (n.href && (n.href.indexOf("http://") != -1))
    n.target = "_blank";
}

for each (var r in document.getElementsByClassName("imagerow")){
  if (r.childNodes) for each (var n in r.childNodes){
    if (n.nodeName && n.nodeName.toLowerCase() == "img"){
      var img = new Image();
      img.src = n.src;
      var size = {
        width: img.width,
        height: img.height,
      };
      var MAX_IMAGE_HEIGHT = 300;
      if (size.height > MAX_IMAGE_HEIGHT){
        var f = size.height / MAX_IMAGE_HEIGHT;
        size.height = size.height / f;
        size.width = size.width / f;
      }
      var style = "min-width: " + (img.width/2) + "px;max-width: " + (size.width) + "px;";
      n.setAttribute("style", style);
      
      if ((img.width <= 260) && (img.height <= MAX_IMAGE_HEIGHT))
        continue;
      
      n.style.cursor = "pointer";
      n.cstyle = style;
      n.ostyle = "max-width: " + (img.width) + "px;width:100%;";
      n.collapsed = true;
      n.onclick = function(e){
        if (!e.target.collapsed){
          e.target.setAttribute("style", e.target.cstyle);
          e.target.collapsed = true;
          for each (var i in e.target.parentNode.childNodes){
            if (i.nodeName && (i.nodeName.toLowerCase() == "img"))
              i.style.display = "";
          }

        } else {
          e.target.setAttribute("style", e.target.ostyle);
          e.target.collapsed = false;
          for each (var i in e.target.parentNode.childNodes){
            if (i.nodeName && (i.nodeName.toLowerCase() == "img") && (i!=e.target))
              i.style.display = "none";
          }
        }
        e.target.style.cursor = "pointer";
      }
    }
  }
}