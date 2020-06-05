
function listar() {
var xml;
xml = new XMLHttpRequest();
xml.open('GET', 'listFiles', true);
xml.onreadystatechange = function () {
  if ((xml.readyState==4) && (xml.status==200)){
    var tab = document.getElementById('tabela')
    var alvoA = document.getElementsByClassName('cellA')
    var alvoB = document.getElementsByClassName('cellB')
    var alvoC = document.getElementsByClassName('cellC')
    var alvoD = document.getElementsByClassName('cellD')
    var listString;
    listString = xml.responseText;
    
    var dataList = mkList(listString)

    for (var i = 0; i < dataList.A.length; i++) {
      var newRow = document.createElement('tr')
      tab.appendChild(newRow)

      var newCellA = document.createElement('td')
      newCellA.setAttribute('class', 'CellA')
      newRow.appendChild(newCellA)
      document.getElementsByClassName('CellA')[i].innerHTML = dataList.A[i]
      
      var newCellB = document.createElement('td')
      newCellB.setAttribute('class', 'CellB')
      newRow.appendChild(newCellB)
      document.getElementsByClassName('CellB')[i].innerHTML = dataList.B[i]
      
      var newCellC = document.createElement('td')
      newCellC.setAttribute('class', 'CellC')
      newRow.appendChild(newCellC)
      document.getElementsByClassName('CellC')[i].innerHTML = dataList.C[i]
      
      var newCellD = document.createElement('td')
      newCellD.setAttribute('class', 'CellD')
      newRow.appendChild(newCellD)
      document.getElementsByClassName('CellD')[i].innerHTML = dataList.D[i]
      
      
      /*
      alvoA[i].innerHTML = dataList.A[i]
      alvoB[i].innerHTML = dataList.B[i]
      alvoC[i].innerHTML = dataList.C[i]
      alvoD[i].innerHTML = dataList.D[i]
      */
    }




  }
}
xml.send();
}

function mkList (stringList) {

  var arrayList = stringList.split("|");
  var dataListA = [];
  var dataListB = [];
  var dataListC = [];
  var dataListD = [];
  var gen;

  for (var i in arrayList){

    gen = arrayList[i].slice(0, 8)
    switch (gen) {
      case "/dados/A":
        dataListA.push(arrayList[i].slice(7));
      break;
      case "/dados/B":
        dataListB.push(arrayList[i].slice(7));
      break;
      case "/dados/C":
        dataListC.push(arrayList[i].slice(7));
      break;
      case "/dados/D":
        dataListD.push(arrayList[i].slice(7));
      break;
    }
  }

  return {
    A: dataListA,
    B: dataListB,
    C: dataListC,
    D: dataListD
  }
}