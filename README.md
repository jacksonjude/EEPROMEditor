# EEPROMEditor
An EEPROM storage viewer and editor for the Arduboy, with a generic structure that can be easily ported to other boards.

## Features
* View any range of EEPROM memory as defined in the constants EEPROM_ADDRESS_START and EEPROM_ADDRESS_END (by default 0-1023)
  * Selected cell number, cell range (on the current page), and current page are shown in the heading
* Memory is automatically sorted into pages and cells based on CHARACTER_LENGTH, CHARACTER_HEIGHT, screen size, and spacing
  * Pages are scrolled through using the (A) and (B) buttons
  * Cells on each page are scrolled through by using the (LEFT) and (RIGHT) buttons
  * Every scrolling control will increase in speed if held down
* Cell edits using the (UP) and (DOWN) buttons
  * Once a cell has been edited, the heading will change to reflect the proposed cell change and the led will turn yellow
  * The (LEFT) and (RIGHT) buttons in this mode will set the cell to the minimum or maximum value as defined in the EEPROM_MIN_VALUE and EEPROM_MAX_VALUE constants
  * The (A) and (B) buttons in this mode will confirm or cancel the edit, accompanied by a green or red flash respectively
* Different data display modes
  * The value of the cell defined in the constant VALUE_DISPLAY_TYPE_ADDRESS (by default 1023) will determine the display type
  * Integer (0, Default), Hexadecimal (1), and ASCII (2) are the current modes
