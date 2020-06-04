
function listar() {
var xml;
xml = new XMLHttpRequest();
xml.open('GET', 'listFiles', true);
xml.onreadystatechange = function () {
  if ((xml.readyState==4) && (xml.status==200)){
    var listString;
    var dataList = [];
    var alvo = document.getElementsByClassName("cell")
    listString = xml.responseText;
    dataList = mkList(listString)

    for (var i in dataList) {
      alvo[i].innerHTML = dataList[i]
    }
    

  }
}
xml.send();
}

function mkList (stringList) {

  var arrayList = stringList.split("|");
  var dataList = [];

  for (var i in arrayList){
    

    if (arrayList[i].startsWith("/dados/")) {
      dataList.push(arrayList[i].slice(7))
    }
  
  
  }

  return dataList
}