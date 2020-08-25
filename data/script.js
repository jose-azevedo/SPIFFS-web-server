window.onload = function() {listFilesInDirectory('/', document.getElementById('years-table'));};

function listFilesInDirectory (path, tableElement) {
  const table = tableElement;
  const cellsBody = document.createElement('tbody');
  table.lastElementChild.remove();
  table.appendChild(cellsBody);

  const xml = new XMLHttpRequest();
  xml.open('GET', 'listFilesInDirectory?path=' + path, true);
  xml.onreadystatechange = function () {
    if ((xml.readyState==4) && (xml.status==200)) {
      const monthsTable = document.getElementById('months-table');
      const filesTable = document.getElementById('files-table');

      if(table.id == 'years-table') {
        showYears(cellsBody, xml.responseText);      
        monthsTable.style.display = 'none';
        filesTable.style.display = 'none';
      }
      if(table.id == 'months-table') {
        showMonths(cellsBody, xml.responseText);      
        monthsTable.style.display = 'table';
        filesTable.style.display = 'none';
      }
      if(table.id == 'files-table') {
        showFiles(cellsBody, xml.responseText);
        filesTable.style.display = 'table';
      }
    }
  }
  xml.send();
}

function showYears(yearCellsBody, response) {
  const yearsArray = JSON.parse(response).files;
  yearsArray.shift();

  const yearRow = document.createElement('tr');
  yearCellsBody.appendChild(yearRow);

  for(const year in yearsArray) {

    const yearCell = document.createElement('td');
    yearCell.setAttribute('onClick', `listFilesInDirectory('${yearsArray[year]}', document.getElementById('months-table'))`);
    yearCell.innerHTML = yearsArray[year].substring(1);
    yearRow.appendChild(yearCell);
    document.getElementById('year-table-title').setAttribute('colspan', `${Number(year) + 1}`);
  }
}

function showMonths(monthCellsBody, response) {
  const monthsArray = JSON.parse(response).files;
  monthsArray.sort((a, b) => {
    a = a.substring(6);
    b = b.substring(6);
    if (a == 'JAN') a = 1;
    if (b == 'JAN') b = 1;
    if (a == 'FEV') a = 2;
    if (b == 'FEV') b = 2;
    if (a == 'MAR') a = 3;
    if (b == 'MAR') b = 3;
    if (a == 'ABR') a = 4;
    if (b == 'ABR') b = 4;
    if (a == 'MAI') a = 5;
    if (b == 'MAI') b = 5;
    if (a == 'JUN') a = 6;
    if (b == 'JUN') b = 6;
    if (a == 'JUL') a = 7;
    if (b == 'JUL') b = 7;
    if (a == 'AGO') a = 8;
    if (b == 'AGO') b = 8;
    if (a == 'SET') a = 9;
    if (b == 'SET') b = 9;
    if (a == 'OUT') a = 10;
    if (b == 'OUT') b = 10;
    if (a == 'NOV') a = 11;
    if (b == 'NOV') b = 11;
    if (a == 'DEZ') a = 12;
    if (b == 'DEZ') b = 12;
    return a-b;
  });
  
  const monthRow = document.createElement('tr');
  monthCellsBody.appendChild(monthRow);

  for(const month in monthsArray) {
    const monthCell = document.createElement('td');
    const monthPrefix = monthsArray[month].substring(6);
    switch (monthPrefix){
      case 'JAN':
        monthCell.innerHTML = 'Janeiro';
      break;
      case 'FEV':
        monthCell.innerHTML = 'Fevereiro';
      break;
      case 'MAR':
        monthCell.innerHTML = 'Março';
      break;
      case 'ABR':
        monthCell.innerHTML = 'Abril';
      break;
      case 'MAI':
        monthCell.innerHTML = 'Maio';
      break;
      case 'JUN':
        monthCell.innerHTML = 'Junho';
      break;
      case 'JUL':
        monthCell.innerHTML = 'Julho';
      break;
      case 'AGO':
        monthCell.innerHTML = 'Agosto';
      break;
      case 'SET':
        monthCell.innerHTML = 'Setembro';
      break;
      case 'OUT':
        monthCell.innerHTML = 'Outubro';
      break;
      case 'NOV':
        monthCell.innerHTML = 'Novembro';
      break;
      case 'DEZ':
        monthCell.innerHTML = 'Dezembro';
      break;
    }
    monthCell.setAttribute('onClick', `listFilesInDirectory('${monthsArray[month]}', document.getElementById('files-table'))`);
    monthRow.appendChild(monthCell);
    document.getElementById('month-table-title').setAttribute('colspan', `${Number(month) + 1}`);
  }
}

function showFiles(fileCellsBody, response) {
  const arrayList = JSON.parse(response).files.sort(); // Recebe a lista bruta de todos os arquivos em uma única string separados por um "|"

  const fileListA = []; // Vetores que receberão os nomes dos arquivos relativos ao seu gerador
  const fileListB = [];
  const fileListC = [];
  const fileListD = [];
  var gen; // Variável que recebe o prefíxo do nome do arquivo que possibilita a distinção entre geradores

  for (const i in arrayList){
    gen = arrayList[i].slice(-5, -4) // Variável recebe o prefixo contido nos 8 primeiros caracteres
    // Condicional switch para atribuir à última posição de cada vetor o nome de arquivo correspondente ao seu gerador, sem o prefíxo de diretório
    switch (gen) {
      case "A":
        fileListA.push(arrayList[i]);
      break;
      case "B":
        fileListB.push(arrayList[i]);
      break;
      case "C":
        fileListC.push(arrayList[i]);
      break;
      case "D":
        fileListD.push(arrayList[i]);
      break;
    }
  }

  const fileLists = [
    fileListA,
    fileListB,
    fileListC,
    fileListD
  ]
  
  for (const i in fileLists) { // Laço se repete enquanto houverem elementos nos vetores
    for (const j in fileLists[i]) {

      var fileRow = document.getElementById(`file-row-${j}`)
      if (fileRow == null) {
        fileRow = document.createElement('tr');
        fileRow.setAttribute('id', `file-row-${j}`);
        fileCellsBody.appendChild(fileRow);
      }

      const fileCell = document.createElement('td');
      fileCell.innerHTML = fileLists[i][j].substring(10);
      fileCell.setAttribute('onclick', `downloadFile('${fileLists[i][j]}')`);
      fileRow.appendChild(fileCell)
    }
  }
}

function downloadFile (filePath) {
  const xml = new XMLHttpRequest;
  xml.open('GET', 'downloadFile?filePath=' + filePath, true);
  xml.onreadystatechange = function () {
    if ((xml.readyState==4) && (xml.status==200)) {
      const a = document.createElement('a');
      a.style.display = 'none';
      document.body.appendChild(a);

      a.href = window.URL.createObjectURL(
        new Blob([xml.responseText], { type: xml.responseType })
      );
      a.setAttribute('download', filePath.substring(10));

      a.click();

      window.URL.revokeObjectURL(a.href);
      document.body.removeChild(a);
    }
  }
  xml.send();
}