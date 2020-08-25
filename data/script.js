window.onload = function() {listYears()};

function listYears () {
  document.getElementById('years-cells').remove();
  const yearsTable = document.getElementById('years-table');
  const yearCellsBody = document.createElement('tbody');
  yearCellsBody.setAttribute('id', 'years-cells');
  yearsTable.appendChild(yearCellsBody);

  const xml = new XMLHttpRequest();
  xml.open('GET', 'listFilesInDirectory?path=/', true);
  xml.onreadystatechange = function () {
    if ((xml.readyState==4) && (xml.status==200)) {
      const yearsArray = JSON.parse(xml.responseText).files;
      yearsArray.shift();

      const yearRow = document.createElement('tr');
      yearCellsBody.appendChild(yearRow);

      for(const year in yearsArray) {

        const yearCell = document.createElement('td');
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
  document.getElementById('months-cells').remove();
  const monthsTable = document.getElementById('months-table');
  const monthCellsBody = document.createElement('tbody');
  monthCellsBody.setAttribute('id', 'months-cells');
  monthsTable.appendChild(monthCellsBody);

  document.getElementById('files-table').style.display = 'none';

  const xml = new XMLHttpRequest();
  xml.open('GET', 'listFilesInDirectory?path=' + yearDirectory, true);
  xml.onreadystatechange = function () {
    if ((xml.readyState==4) && (xml.status==200)) {
      const monthsArray = JSON.parse(xml.responseText).files;
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
  document.getElementById('file-cells').remove();
  const filesTable = document.getElementById('files-table');
  const fileCellsBody = document.createElement('tbody');
  fileCellsBody.setAttribute('id', 'file-cells');
  filesTable.appendChild(fileCellsBody);

  const xml = new XMLHttpRequest();
  xml.open('GET', 'listFilesInDirectory?path=' + monthDirectory, true); // Requisição HTTP do tipo GET na path "/listFiles" do servidor
  xml.onreadystatechange = function () {
    if ((xml.readyState==4) && (xml.status==200)) { // Quando receber um OK do servidor e a lista bruta, inicia o processamento da lista
      const arrayList = JSON.parse(xml.responseText).files.sort(); // Recebe a lista bruta de todos os arquivos em uma única string separados por um "|"

      const fileListA = []; // Vetores que receberão os nomes dos arquivos relativos ao seu gerador
      const fileListB = [];
      const fileListC = [];
      const fileListD = [];
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
      filesTable.style.display = 'table';
    }
  }
  xml.send(); // Envio da requisição HTTP ao servidor
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