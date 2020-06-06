
function listar() {
var xml;
xml = new XMLHttpRequest();
xml.open('GET', 'listFiles', true);
xml.onreadystatechange = function () {
  if ((xml.readyState==4) && (xml.status==200)){
    var tab = document.getElementById('dataCells')
    var listString;
    listString = xml.responseText;
    
    var dataList = mkList(listString)

    for (var i = 0; i < dataList.A.length; i++) {
      var newRow = document.createElement('tr')
      tab.appendChild(newRow)

      var newCellA = document.createElement('td')
      newCellA.setAttribute('class', 'cellA')
      newRow.appendChild(newCellA)
      document.getElementsByClassName('cellA')[i].innerHTML = dataList.A[i]
      newCellA.setAttribute('onclick', 'document.getElementById("filename").setAttribute("value", this.innerText)')

      var newCellB = document.createElement('td')
      newCellB.setAttribute('class', 'cellB')
      newRow.appendChild(newCellB)
      document.getElementsByClassName('cellB')[i].innerHTML = dataList.B[i]
      newCellB.setAttribute('onclick', 'document.getElementById("filename").setAttribute("value", this.innerText)')
      
      var newCellC = document.createElement('td')
      newCellC.setAttribute('class', 'cellC')
      newRow.appendChild(newCellC)
      document.getElementsByClassName('cellC')[i].innerHTML = dataList.C[i]
      newCellC.setAttribute('onclick', 'document.getElementById("filename").setAttribute("value", this.innerText)')
      
      var newCellD = document.createElement('td')
      newCellD.setAttribute('class', 'cellD')
      newRow.appendChild(newCellD)
      document.getElementsByClassName('cellD')[i].innerHTML = dataList.D[i]
      newCellD.setAttribute('onclick', 'document.getElementById("filename").setAttribute("value", this.innerText)')
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

function submit() {
  document.getElementById('submit').click()
}