var key = true;

function listar() {
  if(key){  // Validação para que a lista seja carregada somente uma vez após carregar a página
    var xml;
    xml = new XMLHttpRequest();
    xml.open('GET', 'listFiles', true); // Requisição HTTP do tipo GET na path "/listFiles" do servidor
    xml.onreadystatechange = function () {
      if ((xml.readyState==4) && (xml.status==200)) { // Quando receber um OK do servidor e a lista bruta, inicia o processamento da lista
        key = false // Impedimento para que a lista seja carregada de novo
        var tab = document.getElementById('dataCells')
        var listString;
        listString = xml.responseText; // Recebe a lista bruta de todos os arquivos em uma única string separados por um "|"
        
        var dataList = mkList(listString) // Função recebe a lista bruta e retorna um objeto com as propriedades relativas a lista de cada gerador 

        for (var i = 0; i < dataList.A.length; i++) { // Laço se repete enquanto houverem elementos nos vetores
          var newRow = document.createElement('tr') // Criação de uma nova linha na tabela
          tab.appendChild(newRow) // Anexação da nova linha ao corpo da tabela

          var newCellA = document.createElement('td') // Criação de uma nova célula
          var newCellB = document.createElement('td')
          var newCellC = document.createElement('td')
          var newCellD = document.createElement('td')

          newCellA.setAttribute('class', 'cellA') // Definição do atributo classe do elemento célula
          newCellB.setAttribute('class', 'cellB') // Passo importante para garantir que cada arquivo fique em seu devido lugar
          newCellC.setAttribute('class', 'cellC')
          newCellD.setAttribute('class', 'cellD')

          newRow.appendChild(newCellA) // Anexação da célula
          newRow.appendChild(newCellB)
          newRow.appendChild(newCellC)
          newRow.appendChild(newCellD)

          // O texto da célula é definido como igual ao do elemento no vetor
          document.getElementsByClassName('cellA')[i].innerHTML = dataList.A[i] 
          document.getElementsByClassName('cellB')[i].innerHTML = dataList.B[i]
          document.getElementsByClassName('cellC')[i].innerHTML = dataList.C[i]
          document.getElementsByClassName('cellD')[i].innerHTML = dataList.D[i]

          // Define-se que ao clicar na célula o seu texto, nome do arquivo, seja copiado para a caixa de texto do elemento Formulário. É preciso que isso seja feito para comunicar ao servidor qual o nome do arquivo se deseja baixar quando o formulário for enviado.
          newCellA.setAttribute('onclick', 'document.getElementById("filename").setAttribute("value", this.innerText)')
          newCellB.setAttribute('onclick', 'document.getElementById("filename").setAttribute("value", this.innerText)')
          newCellC.setAttribute('onclick', 'document.getElementById("filename").setAttribute("value", this.innerText)')
          newCellD.setAttribute('onclick', 'document.getElementById("filename").setAttribute("value", this.innerText)')
        }
      }
    }
    xml.send(); // Envio da requisição HTTP ao servidor
  }
}

function mkList (stringList) {

  var arrayList = stringList.split("|"); // Divide a string em um vetor de strings separando os elementos pelo caracter "|". Agora cada elemento deste vetor é o nome de um arquivo
  var dataListA = []; // Vetores que receberão os nomes dos arquivos relativos ao seu gerador
  var dataListB = [];
  var dataListC = [];
  var dataListD = [];
  var gen; // Variável que recebe o prefíxo do nome do arquivo que possibilita a distinção entre geradores

  for (var i in arrayList){

    gen = arrayList[i].slice(0, 8) // Variável recebe o prefixo contido nos 8 primeiros caracteres
    // Condicional switch para atribuir à última posição de cada vetor o nome de arquivo correspondente ao seu gerador, sem o prefíxo de diretório
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

  return { // retorna um objeto com as 4 propriedades sendo vetores relativos a lista de arquivos de cada gerador
    A: dataListA,
    B: dataListB,
    C: dataListC,
    D: dataListD
  }
}

// O elemento de formulário automaticamente envia uma requisição HTTP ao servidor quando seu botão "submit" é clicado, enviando ao servidor o texto escrito em sua caixa de texto. Para reduzir o número de passos para baixar um arquivo, esta função é anexada ao corpo da tabela para que, assim que uma célula for clicada e seu texto copiado para a caixa de texto, o formulário seja enviado logo em seguida.
function submit() {
  document.getElementById('submit').click() // O botão "submit" do formulário é clicado
}