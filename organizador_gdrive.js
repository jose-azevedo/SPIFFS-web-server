// Código Apps Script para organizar os arquivos em pastas referentes aos eus meses e anos de medição

const root = DriveApp.getFolderById('12laC6FeSPaeqy3mrEDzfnW2x3Aq8GqdJ');

function organizeFiles() {
  var files = root.getFiles();
  while (files.hasNext()) {
    var file = files.next();
    getYearFolder(file);
  }
}

function getYearFolder(file){
  const year = file.getName().substring(6, 10);
  const yearFolders = DriveApp.getFoldersByName(year);
  
  var foldersCounter = 0;
  var yearFolder;
  while (yearFolders.hasNext()) {
    yearFolder = yearFolders.next();
    foldersCounter++;
  }
  
  if (foldersCounter != 0) {
    Logger.log('Pasta de ano encontrada: ' + yearFolder.getName());
  } else {
    yearFolder = root.createFolder(year);
    Logger.log('Pasta de ano criada: ' + yearFolder.getName());
  }
  getMonthFolder(file, yearFolder);
}

function getMonthFolder(file, yearFolder) {
  const monthName = getMonthName(file.getName());
  const monthFolders = yearFolder.getFoldersByName(monthName);
  
  var foldersCounter = 0;
  var monthFolder;
  while (monthFolders.hasNext()) {
    monthFolder = monthFolders.next();
    foldersCounter++;
  }
  
  if (foldersCounter != 0) {
    Logger.log('Pasta de mês encontrada: ' + monthFolder.getName());
  } else {
    monthFolder = yearFolder.createFolder(monthName);
    Logger.log('Pasta de mês criada: ' + monthFolder.getName());
  }
  moveFileToCorrespondingFolder(file, monthFolder);
}

function moveFileToCorrespondingFolder(file, monthFolder) {
  const currentFolder = file.getParents().next().getName();
  file.moveTo(monthFolder);
  Logger.log('Movendo o arquivo ' + file.getName() + ' da pasta ' + currentFolder + ' para a pasta ' + monthFolder.getParents().next().getName() + '/' + monthFolder.getName());
}

function getMonthName(fileName) {
  var monthNumber = Number(fileName.substring(3, 5));
  switch (monthNumber){
    case 1:
      return "Janeiro";
    break;
    case 2:
      return "Fevereiro";
    break;
    case 3:
      return "Março";
    break;
    case 4:
      return "Abril";
    break;
    case 5:
      return "Maio";
    break;
    case 6:
      return "Junho";
    break;
    case 7:
      return "Julho";
    break;
    case 8:
      return "Agosto";
    break;
    case 9:
      return "Setembro";
    break;
    case 10:
      return "Outubro";
    break;
    case 11:
      return "Novembro";
    break;
    case 12:
      return "Dezembro";
    break;
  }
}

function createTrigger() {
  ScriptApp.newTrigger('organizeFiles')
  .timeBased()
  .everyDays(1)
  .atHour(1)
  .create();
}