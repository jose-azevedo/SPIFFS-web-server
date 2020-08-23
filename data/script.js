window.onload = function() {listYears()};

function listYears () {
  const yearsTable = document.getElementById('years-table');
  var yearsCells = document.getElementById('years-cells');
  yearsCells.remove();
  yearsCells = document.createElement('tbody');
  yearsCells.setAttribute('id', 'years-cells');
  yearsTable.appendChild(yearsCells);

  const xml = new XMLHttpRequest();
  xml.open('GET', 'listFilesInDirectory?path=/', true);
  xml.onreadystatechange = function () {
    if ((xml.readyState==4) && (xml.status==200)) {
      yearsList = xml.responseText;

      var yearsArray = yearsList.split('|');
      yearsArray.pop();
      yearsArray.sort();
      yearsArray.pop();

      const yearRow = document.createElement('tr');
      yearsCells.appendChild(yearRow);

      for(const year in yearsArray) {

        var yearCell = document.createElement('td');
        yearCell.setAttribute('onClick', `listMonths('${yearsArray[year]}')`);
        yearCell.innerHTML = yearsArray[year].substring(1);
        yearRow.appendChild(yearCell);
        document.getElementById('year-table-title').setAttribute('colspan', `${Number(year) + 1}`);
      }
      document.getElementById('months-table').style.display = 'none';
      document.getElementById('files-table').style.display = 'none';
    }
  }
  xml.send();
}

function listMonths (yearDirectory) {
  const monthsTable = document.getElementById('months-table');
  var monthsCells = document.getElementById('months-cells');
  monthsCells.remove();
  monthsCells = document.createElement('tbody');
  monthsCells.setAttribute('id', 'months-cells');
  monthsTable.appendChild(monthsCells);

  document.getElementById('files-table').style.display = 'none';

  const xml = new XMLHttpRequest();
  xml.open('GET', 'listFilesInDirectory?path=' + yearDirectory, true);
  xml.onreadystatechange = function () {
    if ((xml.readyState==4) && (xml.status==200)) {
      monthsList = xml.responseText;
      
      const monthRow = document.createElement('tr');
      monthsCells.appendChild(monthRow);
      var monthsArray = monthsList.split('|');
      monthsArray.pop();
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

      for(const month in monthsArray) {
        var monthCell = document.createElement('td');
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
        monthCell.setAttribute('onClick', `listFiles('${monthsArray[month]}')`);
        monthRow.appendChild(monthCell);
        document.getElementById('month-table-title').setAttribute('colspan', `${Number(month) + 1}`);
      }
      monthsTable.style.display = 'table';
    }
  }
  xml.send();
}

function listFiles(monthDirectory) {
  const filesTable = document.getElementById('files-table');
  var filesCells = document.getElementById('file-cells');
  filesCells.remove();
  filesCells = document.createElement('tbody');
  filesCells.setAttribute('id', 'file-cells');
  filesTable.appendChild(filesCells);

  const xml = new XMLHttpRequest();
  xml.open('GET', 'listFilesInDirectory?path=' + monthDirectory, true); // Requisição HTTP do tipo GET na path "/listFiles" do servidor
  xml.onreadystatechange = function () {
    if ((xml.readyState==4) && (xml.status==200)) { // Quando receber um OK do servidor e a lista bruta, inicia o processamento da lista
      
      const rawFileLists = xml.responseText; // Recebe a lista bruta de todos os arquivos em uma única string separados por um "|"
      
      const fileLists = formatFileLists(rawFileLists) // Função recebe a lista bruta e retorna um objeto com as propriedades relativas a lista de cada gerador 

      for (const i in fileLists) { // Laço se repete enquanto houverem elementos nos vetores
        for (const j in fileLists[i]) {

          var fileRow = document.getElementById(`file-row-${j}`)
          if (fileRow == null) {
            fileRow = document.createElement('tr');
            fileRow.setAttribute('id', `file-row-${j}`);
            filesCells.appendChild(fileRow);
          }

          const fileCell = document.createElement('td');
          fileCell.innerHTML = fileLists[i][j].substring(10);
          fileCell.setAttribute('onclick', `downloadFile('${fileLists[i][j]}')`);
          fileRow.appendChild(fileCell)
        }
      }
      filesTable.style.display = 'table';
    }
  }
  xml.send(); // Envio da requisição HTTP ao servidor
}

function formatFileLists (rawFileLists) {

  const arrayList = rawFileLists.split("|"); // Divide a string em um vetor de strings separando os elementos pelo caracter "|". Agora cada elemento deste vetor é o nome de um arquivo
  arrayList.sort();
  var fileListA = []; // Vetores que receberão os nomes dos arquivos relativos ao seu gerador
  var fileListB = [];
  var fileListC = [];
  var fileListD = [];
  var gen; // Variável que recebe o prefíxo do nome do arquivo que possibilita a distinção entre geradores

  for (const i in arrayList){
    gen = arrayList[i].slice(10, 11) // Variável recebe o prefixo contido nos 8 primeiros caracteres
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

  return [ // retorna um objeto com as 4 propriedades sendo vetores relativos a lista de arquivos de cada gerador
    fileListA,
    fileListB,
    fileListC,
    fileListD
  ]
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