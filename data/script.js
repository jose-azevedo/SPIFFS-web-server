window.onload = function() {listYears()};

function listYears () {
  var yearsTable = document.getElementById('years-table');
  var yearsCells = document.getElementById('years-cells');
  yearsCells.remove();
  yearsCells = document.createElement('tbody');
  yearsCells.setAttribute('id', 'years-cells');
  yearsTable.appendChild(yearsCells);

  var xml;
  xml = new XMLHttpRequest();
  xml.open('GET', 'listYears', true);
  xml.onreadystatechange = function () {
    if ((xml.readyState==4) && (xml.status==200)) {
      yearsList = xml.responseText;

      var yearsArray = yearsList.split('|');
      yearsArray.pop();

      for(const year in yearsArray) {
        const yearRow = document.createElement('tr');
        yearsCells.appendChild(yearRow);

        var yearCell = document.createElement('td');
        yearCell.setAttribute('onClick', `listMonths('${yearsArray[year]}')`);
        yearCell.innerHTML = yearsArray[year].substring(1);
        
        yearRow.appendChild(yearCell);
      }
      document.getElementById('months-table').style.display = 'none';
      document.getElementById('files-table').style.display = 'none';
    }
  }
  xml.send();
}

function listMonths (year) {
  var monthsTable = document.getElementById('months-table');
  var monthsCells = document.getElementById('months-cells');
  monthsCells.remove();
  monthsCells = document.createElement('tbody');
  monthsCells.setAttribute('id', 'months-cells');
  monthsTable.appendChild(monthsCells);

  var xml;
  xml = new XMLHttpRequest();
  xml.open('GET', 'listMonths?year=' + year, true);
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
        document.getElementById('month-table-title').setAttribute('colspan', month + 1)
      }
      monthsTable.style.display = 'table';
      document.getElementById('files-table').style.display = 'none';
    }
  }
  xml.send();
}

function listFiles(month) {
  var filesCells = document.getElementById('file-cells');
  var filesTable = document.getElementById('files-table');
  filesCells.remove();
  filesCells = document.createElement('tbody');
  filesCells.setAttribute('id', 'file-cells');
  filesTable.appendChild(filesCells);

  var xml;
  xml = new XMLHttpRequest();
  xml.open('GET', 'listFiles?month=' + month, true); // Requisição HTTP do tipo GET na path "/listFiles" do servidor
  xml.onreadystatechange = function () {
    if ((xml.readyState==4) && (xml.status==200)) { // Quando receber um OK do servidor e a lista bruta, inicia o processamento da lista
      
      var rawFileLists = xml.responseText; // Recebe a lista bruta de todos os arquivos em uma única string separados por um "|"
      
      var fileLists = formatFileLists(rawFileLists) // Função recebe a lista bruta e retorna um objeto com as propriedades relativas a lista de cada gerador 

      for (const i in fileLists.A) { // Laço se repete enquanto houverem elementos nos vetores
        var newRow = document.createElement('tr') // Criação de uma nova linha na tabela
        filesCells.appendChild(newRow) // Anexação da nova linha ao corpo da tabela

        var newCellA = document.createElement('td') // Criação de uma nova célula
        var newCellB = document.createElement('td')
        var newCellC = document.createElement('td')
        var newCellD = document.createElement('td')

        newRow.appendChild(newCellA) // Anexação da célula
        newRow.appendChild(newCellB)
        newRow.appendChild(newCellC)
        newRow.appendChild(newCellD)

        // O texto da célula é definido como igual ao do elemento no vetor
        newCellA.innerHTML = fileLists.A[i].substring(10);
        newCellB.innerHTML = fileLists.B[i].substring(10);
        newCellC.innerHTML = fileLists.C[i];
        newCellD.innerHTML = fileLists.D[i];

        // Define-se que ao clicar na célula o seu texto, nome do arquivo, seja copiado para a caixa de texto do elemento Formulário. É preciso que isso seja feito para comunicar ao servidor qual o nome do arquivo se deseja baixar quando o formulário for enviado.
        newCellA.setAttribute('onclick', `downloadFile('${fileLists.A[i]}')`);
        newCellB.setAttribute('onclick', `downloadFile('${fileLists.B[i]}')`);
        newCellC.setAttribute('onclick', `downloadFile('${fileLists.C[i]}')`);
        newCellD.setAttribute('onclick', `downloadFile('${fileLists.D[i]}')`);
      }
      filesTable.style.display = 'table';
    }
  }
  xml.send(); // Envio da requisição HTTP ao servidor
}

function formatFileLists (rawFileLists) {

  var arrayList = rawFileLists.split("|"); // Divide a string em um vetor de strings separando os elementos pelo caracter "|". Agora cada elemento deste vetor é o nome de um arquivo
  arrayList.sort();
  var fileListA = []; // Vetores que receberão os nomes dos arquivos relativos ao seu gerador
  var fileListB = [];
  var fileListC = [];
  var fileListD = [];
  var gen; // Variável que recebe o prefíxo do nome do arquivo que possibilita a distinção entre geradores

  for (var i in arrayList){
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

  return { // retorna um objeto com as 4 propriedades sendo vetores relativos a lista de arquivos de cada gerador
    A: fileListA,
    B: fileListB,
    C: fileListC,
    D: fileListD
  }
}

function downloadFile (filePath) {
  var xml = new XMLHttpRequest;
  xml.open('GET', 'downloadFile?filePath=' + filePath, true);
  xml.onreadystatechange = function () {
    if ((xml.readyState==4) && (xml.status==200)) {
      console.log(xml.responseText);
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