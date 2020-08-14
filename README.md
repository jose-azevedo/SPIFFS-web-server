# Photovoltaic-systems-monitor

These sketches make part of a larger measurement system designed to measure electrical parameters from the input and output of photovoltaic microinverters.

## Why?

The current project was designed to meet the needs of monitoring one of the photovoltaic systems present on the test area of the Study and Development of Energetic Alternatives Group (GEDAE) in the Federal University of Pará (UFPA) in Brazil.<br>

## Basic operation

The Arduino microcontroller has it's analog ports connected to printed circuit boards which measure voltage and current.
It collects several points of electrical signal, both DC and AC, and then starts to calculate parameters such as active power, power factor and others. Every five minutes these values are stored locally in a SD card in a CSV file named after the day they were measured. In addition to that, they are also sent to an ESP32 microcontroller to be stored at another SD card. The ESP32 also communicates with both Google Drive and Spreadsheet APIs in order to create another backup of the files. In case of Internet connection loss, the files stored locally in the ESP32 SD card can be accessed and downloaded through the server it provides at the local network it is connected to.