# dspic-phaser-effect (not working yet, under developement)

Phaser effect based on the Microchip's dsPIC33FJ128GP802 Digital Signal Controller

## Project block diagram:
![project block diagram](https://github.com/DanCasterIt/dspic-phaser-effect/blob/master/board_files/draw.io/phaser_block_diagram_description.svg)

## How to compile and run

1) Clone this project
2) Rename this project's folder from "dspic-phaser-effect" to "dspic-phaser-effect.X"
3) Open it with MPLAB X
4) If it does not compile already, right-click on the project name -> properties -> Conf:[default] -> XC16-> xc16-ld -> in "Additional options:" put the string: "--library "dsp""

## Why does it not work?

The phase shifter block requires a VERY FAST atan2() function implementation that works using the Q1.15 fixed point format.
Unfortunately all the implementations I found around the web of this function aren't fast enouth:
The time used to compute atan2() is longer than 1/(36.423529KHz)s that is the period between a sample and the next one.

### Project board:
![project board](https://github.com/DanCasterIt/dspic-phaser-effect/blob/master/board_files/matrix_breadboard/board.jpg)
### Digital board front:
![Digital board front](https://github.com/DanCasterIt/dspic-phaser-effect/blob/master/board_files/matrix_breadboard/dsPIC33FJ128GP802_board.jpg)
### Digital board back:
![Digital board back](https://github.com/DanCasterIt/dspic-phaser-effect/blob/master/board_files/matrix_breadboard/dsPIC33FJ128GP802_board_wirings.jpg)
### Digital board design:
![Digital board design](https://github.com/DanCasterIt/dspic-phaser-effect/blob/master/board_files/matrix_breadboard/dsPIC33FJ128GP802_board_design.jpg)
### Analog board front:
![Analog board front](https://github.com/DanCasterIt/dspic-phaser-effect/blob/master/board_files/matrix_breadboard/analog_board.jpg)
### Analog board back:
![Analog board back](https://github.com/DanCasterIt/dspic-phaser-effect/blob/master/board_files/matrix_breadboard/analog_board_wirings.jpg)
### Analog board design:
![Analog board design](https://github.com/DanCasterIt/dspic-phaser-effect/blob/master/board_files/matrix_breadboard/analog_board_design.jpg)
### Digital board power regulator front:
![Digital board power regulator front](https://github.com/DanCasterIt/dspic-phaser-effect/blob/master/board_files/matrix_breadboard/3v3_board_power_supply.jpg)
### Digital board power regulator back:
![Digital board power regulator back](https://github.com/DanCasterIt/dspic-phaser-effect/blob/master/board_files/matrix_breadboard/3v3_board_power_supply_wirings.jpg)
### Analog circuitry for the ADC:
![analog circuitry for the ADC](https://github.com/DanCasterIt/dspic-phaser-effect/blob/master/board_files/schematics/analog_circuitry_for_ADC.png)
### Analog circuitry for the DAC:
![analog circuitry for the DAC](https://github.com/DanCasterIt/dspic-phaser-effect/blob/master/board_files/schematics/analog_circuitry_for_DAC.png)