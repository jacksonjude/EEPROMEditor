/**

EEPROM System Map -- Arduboy2 v5.3.0
(Addresses 0-15)

0     -- "Reserved for bootloader use" (DO NOT EDIT)
1     -- System flags
        - Bits 1-4 are 1
        - Bit 5 is 0
        - Bit 6 toggles LEDs on logo screen
        - Bit 7 toggles logo screen on boot
        - Bit 8 toggles unit name on logo screen
2     -- System audio toggle
3-7   -- "Reserved for future use"
8-9   -- 16-bit unit binary ID
10-15 -- 6 character unit name
        - Cannot contain 0x00, 0xFF, 0x0A, 0x0D
        - Names with <6 characters are padded with 0x00

**/

#include <Arduboy2.h>
#include <SPI.h>
#include <EEPROM.h>

Arduboy2 arduboy;

const int FRAMES_PER_SECOND = 30;
const int FRAMES_TO_UPDATE_AFTER_INPUT = 1;

const int PAGE_BUTTONS_DELAY_TIME = 130;
const int CELL_SCROLL_BUTTONS_DELAY_TIME = 110;
const int CELL_BUTTONS_DELAY_TIME = 100;

const int FRAMES_TO_INCREASE_SPEED = 10;
const int DELAY_SPEED_MULTIPLIER = 8;

const int CHARACTER_LENGTH = 5;
const int CHARACTER_HEIGHT = 8;
const int CHARACTER_HORIZONTAL_SPACING = 1;
const int CHARACTER_TOP_VERTICAL_SPACING = 1;
const int CHARACTER_BOTTOM_VERTICAL_SPACING = 2;
const int CHARACTER_VERTICAL_SPACING = CHARACTER_TOP_VERTICAL_SPACING + CHARACTER_BOTTOM_VERTICAL_SPACING;

const int HEADING_HEIGHT = CHARACTER_HEIGHT + CHARACTER_VERTICAL_SPACING;

const int SPACE_CHARACTER_LENGTH = 1;

const int EEPROM_ADDRESS_START = 0;
const int EEPROM_ADDRESS_END = 1023;

const int EEPROM_MIN_VALUE = 0;
const int EEPROM_MAX_VALUE = 255;

const int BIN_BYTE_LENGTH = String(EEPROM_MAX_VALUE-EEPROM_MIN_VALUE, BIN).length();
const int HEX_BYTE_LENGTH = String(EEPROM_MAX_VALUE-EEPROM_MIN_VALUE, HEX).length();
const int INT_BYTE_LENGTH = String(EEPROM_MAX_VALUE-EEPROM_MIN_VALUE).length();
const int ASCII_BYTE_LENGTH = String(char(EEPROM_MAX_VALUE-EEPROM_MIN_VALUE)).length();

const int VALUE_DISPLAY_TYPE_ADDRESS = 1023;
//const int CAN_EDIT_SYSTEM_DATA_ADDRESS = 1022; //TBA
//const int SELECTED_ADDRESS_ADDRESS = 1021; //TBA

const int EDITING_LED_BRIGHTNESS = 40;
const int EDITING_LED_DELAY = 300;
const int EDITING_LED_ACTIVE_COLORS[] = { RED_LED, GREEN_LED };
const int EDITING_LED_CONFIRM_COLORS[] = { GREEN_LED };
const int EDITING_LED_CANCEL_COLORS[] = { RED_LED };

enum ValueDisplayType
{
  BINA,
  INT,
  HEXA,
  ASCII,
};

ValueDisplayType displayType;

int EEPROMAddressLength;

int byteCharacterLength;

int screenWidth;
int screenHeight;

int pageCount;
int cellLength;
int cellCount;
int cellRows;
int cellColumns;
int cellIndent;

int headingLength;
int headingIndent;
boolean showPageNumbers;

int currentPage;
int currentCell;
int currentAddress;
int currentBit;

int framesToShow;

int framesInputHasTriggered;

boolean editingCell;
int originalValue;
int newValue;

void setup()
{
  Serial.begin(9600);
  arduboy.begin();
  arduboy.setFrameRate(FRAMES_PER_SECOND);

  arduboy.setRGBled(0, 0, 0);

  currentAddress = EEPROM_STORAGE_SPACE_START;

  setupDisplayVariables();
}

void setupDisplayVariables()
{
  screenWidth = arduboy.width();
  screenHeight = arduboy.height();

  displayType = EEPROM.read(VALUE_DISPLAY_TYPE_ADDRESS);
  switch (displayType)
  {
    case BINA:
    byteCharacterLength = String(EEPROM_MAX_VALUE-EEPROM_MIN_VALUE, BIN).length();
    break;

    case INT:
    default:
    byteCharacterLength = INT_BYTE_LENGTH;
    break;

    case HEXA:
    byteCharacterLength = HEX_BYTE_LENGTH;
    break;

    case ASCII:
    byteCharacterLength = ASCII_BYTE_LENGTH;
    break;
  }

  cellLength = (byteCharacterLength+SPACE_CHARACTER_LENGTH)*(CHARACTER_LENGTH+CHARACTER_HORIZONTAL_SPACING);
  cellColumns = floor(1.0*screenWidth/cellLength);
  cellRows = floor(1.0*(screenHeight-HEADING_HEIGHT+CHARACTER_BOTTOM_VERTICAL_SPACING)/(CHARACTER_HEIGHT+CHARACTER_VERTICAL_SPACING));
  cellCount = cellRows*cellColumns;
  cellIndent = ((screenWidth)%cellLength+CHARACTER_LENGTH+CHARACTER_HORIZONTAL_SPACING)/2;
  pageCount = ceil(1.0*(EEPROM_ADDRESS_END-EEPROM_ADDRESS_START)/cellCount);

  EEPROMAddressLength = String(EEPROM_ADDRESS_END-EEPROM_ADDRESS_START).length();

  calculateHeadingLength();

  currentPage = currentAddress/cellCount;
  currentCell = currentAddress%cellCount;

  framesToShow = 0;
  framesInputHasTriggered = 0;
  editingCell = false;
}

void calculateHeadingLength()
{
  int headingCharacterCount = EEPROMAddressLength+1+EEPROMAddressLength+1+EEPROMAddressLength+1+String(pageCount).length()+1+String(pageCount).length();
  if ((CHARACTER_LENGTH+CHARACTER_HORIZONTAL_SPACING)*headingCharacterCount > screenWidth+CHARACTER_HORIZONTAL_SPACING)
  {
    showPageNumbers = false;
    headingCharacterCount -= 1+String(pageCount).length()+1+String(pageCount).length();
  }
  else
    showPageNumbers = true;

  if (editingCell)
  {
    headingCharacterCount -= EEPROMAddressLength+1+EEPROMAddressLength+(showPageNumbers ? 1+String(pageCount).length()+1+String(pageCount).length() : 0);
    headingCharacterCount += 2+INT_BYTE_LENGTH+4+INT_BYTE_LENGTH+2;
  }

  headingLength = (CHARACTER_LENGTH+CHARACTER_HORIZONTAL_SPACING)*headingCharacterCount-CHARACTER_HORIZONTAL_SPACING;
  headingIndent = (screenWidth+CHARACTER_HORIZONTAL_SPACING)%headingLength/2;
}

void loop()
{
  if (!arduboy.nextFrame() || (!arduboy.buttonsState() && framesToShow == 0 && arduboy.frameCount > 1))
    return;

  arduboy.clear();

  if (arduboy.buttonsState())
    framesToShow = FRAMES_TO_UPDATE_AFTER_INPUT;
  else if (framesToShow > 0)
    framesToShow--;

  currentAddress = currentPage*cellCount+currentCell;

  if (!arduboy.buttonsState() && framesInputHasTriggered > 0)
    framesInputHasTriggered = 0;

  if (arduboy.pressed(A_BUTTON))
  {
    if (!editingCell)
    {
      currentPage--;
      if (currentPage < 0)
        currentPage = pageCount-1;

      checkForInvalidEEPROMSelection();

      delayAfterInput(PAGE_BUTTONS_DELAY_TIME);
    }
    else
    {
      EEPROM.write(currentAddress, newValue);

      arduboy.setRGBled(0, 0, 0);
      for (int i=0; i < sizeof(EDITING_LED_CONFIRM_COLORS); i++)
        arduboy.setRGBled(EDITING_LED_CONFIRM_COLORS[i], EDITING_LED_BRIGHTNESS/sizeof(EDITING_LED_CONFIRM_COLORS));
      delay(EDITING_LED_DELAY);
      for (int i=0; i < sizeof(EDITING_LED_CONFIRM_COLORS); i++)
        arduboy.setRGBled(EDITING_LED_CONFIRM_COLORS[i], 0);

      editingCell = false;
      calculateHeadingLength();

      if (displayType == BINA)
        currentBit = 0;
    }
  }
  else if (arduboy.pressed(B_BUTTON))
  {
    if (!editingCell)
    {
      currentPage++;
      if (currentPage > pageCount-1)
        currentPage = 0;

      checkForInvalidEEPROMSelection();

      delayAfterInput(PAGE_BUTTONS_DELAY_TIME);
    }
    else
    {
      arduboy.setRGBled(0, 0, 0);
      for (int i=0; i < sizeof(EDITING_LED_CANCEL_COLORS); i++)
        arduboy.setRGBled(EDITING_LED_CANCEL_COLORS[i], EDITING_LED_BRIGHTNESS/sizeof(EDITING_LED_CANCEL_COLORS));
      delay(EDITING_LED_DELAY);
      for (int i=0; i < sizeof(EDITING_LED_CANCEL_COLORS); i++)
        arduboy.setRGBled(EDITING_LED_CANCEL_COLORS[i], 0);

      editingCell = false;
      calculateHeadingLength();

      if (displayType == BINA)
        currentBit = 0;
    }
  }

  if (arduboy.pressed(LEFT_BUTTON))
  {
    if (!editingCell)
    {
      currentCell--;
      checkForInvalidEEPROMSelection();
    }
    else if (displayType == BINA)
    {
      currentBit--;
      if (currentBit < 0)
        currentBit = BIN_BYTE_LENGTH;
    }
    else
      newValue = EEPROM_MIN_VALUE;

    delayAfterInput(CELL_SCROLL_BUTTONS_DELAY_TIME);
  }
  else if (arduboy.pressed(RIGHT_BUTTON))
  {
    Serial.println(currentBit);
    if (!editingCell)
    {
      currentCell++;
      checkForInvalidEEPROMSelection();
    }
    else if (displayType == BINA)
    {
      currentBit++;
      if (currentBit > BIN_BYTE_LENGTH)
        currentBit = 0;
    }
    else
      newValue = EEPROM_MAX_VALUE;

    delayAfterInput(CELL_SCROLL_BUTTONS_DELAY_TIME);
  }

  if ((arduboy.buttonsState() & (UP_BUTTON | DOWN_BUTTON)) != 0 && !editingCell)
  {
    editingCell = true;
    calculateHeadingLength();

    originalValue = EEPROM.read(currentAddress);
    newValue = originalValue;
  }

  if ((arduboy.pressed(UP_BUTTON) || arduboy.pressed(DOWN_BUTTON)) && displayType == BINA && currentBit != 0)
  {
    int bitValue = bitRead(newValue, BIN_BYTE_LENGTH-currentBit);
    bitValue = (bitValue == 0) ? 1 : 0;
    bitWrite(newValue, BIN_BYTE_LENGTH-currentBit, bitValue);

    delayAfterInput(CELL_BUTTONS_DELAY_TIME);
  }
  else if (arduboy.pressed(UP_BUTTON))
  {
    newValue++;
    if (newValue > EEPROM_MAX_VALUE)
      newValue = EEPROM_MIN_VALUE;

    delayAfterInput(CELL_BUTTONS_DELAY_TIME);
  }
  else if (arduboy.pressed(DOWN_BUTTON))
  {
    newValue--;
    if (newValue < EEPROM_MIN_VALUE)
      newValue = EEPROM_MAX_VALUE;

    delayAfterInput(CELL_BUTTONS_DELAY_TIME);
  }

  ValueDisplayType currentDisplayType = EEPROM.read(VALUE_DISPLAY_TYPE_ADDRESS);
  if (currentDisplayType != displayType)
    setupDisplayVariables();

  arduboy.setCursor(headingIndent,0);
  arduboy.print(applyZeroPadding(currentAddress, EEPROMAddressLength) + " ");
  arduboy.drawFastHLine(headingIndent, CHARACTER_HEIGHT, headingLength, WHITE);

  if (!editingCell)
    displayPageHeading();
  else
    displayEditingHeading();

  for (int i=0; i < cellRows; i++)
  {
    for (int j=0; j < cellColumns; j++)
    {
      int absoluteCellNumber = currentPage*cellCount+(i*cellColumns+j);
      if (absoluteCellNumber > EEPROM_ADDRESS_END)
        continue;
      arduboy.setCursor(cellIndent+(j*cellLength), HEADING_HEIGHT+(i*(CHARACTER_HEIGHT+CHARACTER_VERTICAL_SPACING)));

      int rawValue;
      if (absoluteCellNumber != currentAddress || !editingCell)
        rawValue = EEPROM.read(absoluteCellNumber);
      else
        rawValue = newValue;

      String valueToDisplay;
      switch (displayType)
      {
        case BINA:
        valueToDisplay = applyCharacterPadding(String(rawValue, BIN), byteCharacterLength, "0");
        break;

        case INT:
        default:
        valueToDisplay = applyZeroPadding(rawValue, byteCharacterLength);
        break;

        case HEXA:
        valueToDisplay = applyCharacterPadding(String(rawValue, HEX), byteCharacterLength, "0");
        break;

        case ASCII:
        valueToDisplay = applyCharacterPadding(String((char) rawValue), byteCharacterLength, " ");
        break;
      }
      arduboy.print(valueToDisplay);
    }
  }

  if (displayType != BINA || currentBit == 0)
    arduboy.drawFastHLine(cellIndent+(currentCell%cellColumns*cellLength), HEADING_HEIGHT+((currentCell/cellColumns)*(CHARACTER_HEIGHT+CHARACTER_VERTICAL_SPACING))+CHARACTER_HEIGHT, byteCharacterLength*(CHARACTER_LENGTH+CHARACTER_HORIZONTAL_SPACING)-CHARACTER_HORIZONTAL_SPACING, WHITE);
  else
    arduboy.drawFastHLine(cellIndent+(currentCell%cellColumns*cellLength)+(currentBit-1)*(CHARACTER_LENGTH+CHARACTER_HORIZONTAL_SPACING), HEADING_HEIGHT+((currentCell/cellColumns)*(CHARACTER_HEIGHT+CHARACTER_VERTICAL_SPACING))+CHARACTER_HEIGHT, CHARACTER_LENGTH, WHITE);

  arduboy.display();
}

void displayPageHeading()
{
  arduboy.print(applyZeroPadding(currentPage*cellCount, EEPROMAddressLength) + "-" + applyZeroPadding(min((currentPage+1)*cellCount-1, EEPROM_ADDRESS_END), EEPROMAddressLength));
  if (showPageNumbers)
    arduboy.print(" " + applyZeroPadding(currentPage, String(pageCount-1).length()) + "/" + String(pageCount-1));
}

void displayEditingHeading()
{
  arduboy.print("* " + applyZeroPadding(originalValue, INT_BYTE_LENGTH) + " -> " + applyZeroPadding(newValue, INT_BYTE_LENGTH) + " *");
  for (int i=0; i < sizeof(EDITING_LED_ACTIVE_COLORS); i++)
  {
    arduboy.setRGBled(EDITING_LED_ACTIVE_COLORS[i], EDITING_LED_BRIGHTNESS/sizeof(EDITING_LED_ACTIVE_COLORS));
  }
}

String applyZeroPadding(int value, int spaces)
{
  return applyCharacterPadding(String(value), spaces, "0");
}

String applyCharacterPadding(String value, int spaces, String character)
{
  while (value.length() < spaces)
    value = character + value;

  return value;
}

void checkForInvalidEEPROMSelection()
{
  if (currentCell < 0)
    currentCell = cellCount-1;
  else if (currentCell > cellCount-1 || currentPage*cellCount+currentCell > EEPROM_ADDRESS_END)
    currentCell = 0;

  while (currentPage*cellCount+currentCell > EEPROM_ADDRESS_END)
    currentCell--;
}

void delayAfterInput(int delayTime)
{
  framesInputHasTriggered++;

  if (framesInputHasTriggered > FRAMES_TO_INCREASE_SPEED)
    delay(delayTime/DELAY_SPEED_MULTIPLIER);
  else
    delay(delayTime);
}
