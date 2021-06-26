# Photovoltaic systems monitor

The sketches presented in this repository make part of a larger measurement system designed to measure electrical parameters from the input and output of photovoltaic microinverters.

## Description

This project was designed to meet the needs of monitoring one of the photovoltaic systems present on the test area of the Study and Development of Energetic Alternatives Group (GEDAE) in the Federal University of Pará (UFPA) in Belém, Brazil.  
<br/>
It was presented as final course assignment for the electrical engineering program at UFPA on april 2021. The monograph written in portuguese can be accessed [here](https://drive.google.com/file/d/1kmW1RhBxTo-32XDwVud9WWQtqQinaMVj/view?usp=sharing).  
<br/>
This repository contains two sketches destined for two different microcontrollers. The one at `src/main.cpp` is destined to an ESP32 while the one at `Arduino-side/Arduino-side.ino` is destined for an Arduino Mega. Their roles in the monitoring system are briefly described on the next section.

## Basic operation

The Arduino Mega has it's analog ports connected to circuits that measure voltage and current both on the input and output of two microinverters. It collects several points of the electrical signals, both DC and AC, and then starts to calculate parameters such as active power, power factor and others. Every five minutes these values are stored locally in a SD card in a CSV file named after the day they were measured. In addition to that, they are also sent to an ESP32 microcontroller to be stored at another SD card.  
The ESP32 also communicates with both Google Drive and Spreadsheets APIs in order to create another backup of the files. In case of Internet connection loss, the files stored locally in the ESP32 SD card can be accessed and downloaded through the webpage it's local server provides.  
You can follow these links to view photos of the [measurement board](https://drive.google.com/file/d/1-4K3vK2Dm7xz_yw70IXg86MSYsaXVgGz/view?usp=sharing) and the [webpage](https://drive.google.com/file/d/1-rDeoJmV-sZfSGjFYN6nqVejtHickcF6/view?usp=sharing).