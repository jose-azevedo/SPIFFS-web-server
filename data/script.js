
function listar() {
var xml;
xml = new XMLHttpRequest();
xml.open('GET', '02-05-2020.txt', true);
xml.onreadystatechange = function () {
  if ((xml.readyState==4) && (xml.status==200)){
    var alvo;
    alvo = document.getElementsByClassName("cell");
    alvo[0].innerHTML = xml.responseText;
  }
}
xml.send();
}
