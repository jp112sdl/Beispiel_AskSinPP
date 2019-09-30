//tran nhat thanh
//version 1.3 fixed the error when display time format in previous version 1.2
//version 1.3 got issue when switching from time to integer number (fixed)
//version 1.3 got issue when use float number with 4 number behind dot signal

#include <Arduino.h>
#include "lcd.h"

//============DEFINE=============
#define  BIAS     0x52             //0b1000 0101 0010
#define  SYSDIS   0X00             //0b1000 0000 0000
#define  SYSEN    0X02             //0b1000 0000 0010
#define  LCDOFF   0X04             //0b1000 0000 0100
#define  LCDON    0X06             //0b1000 0000 0110
#define  XTAL     0x28             //0b1000 0010 1000
#define  RC256    0X30             //0b1000 0011 0000
#define  TONEON   0X12             //0b1000 0001 0010
#define  TONEOFF  0X10             //0b1000 0001 0000
#define  WDTDIS1  0X0A             //0b1000 0000 1010

//initiate the LCD.
void Lcd::begin(int csPin, int wrPin, int dataPin){
  pinMode(csPin, OUTPUT);
  pinMode(wrPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  _csPin = csPin;
  _wrPin = wrPin;
  _dataPin = dataPin;

  //setup the LCD
  wrCMD(BIAS);
  wrCMD(SYSDIS);
  wrCMD(SYSEN);
  wrCMD(RC256);
  wrCMD(WDTDIS1);
  wrCMD(LCDON);
}

//sending command to LCD.
void Lcd::wrCMD(byte command){
  digitalWrite(_csPin, LOW);
  uint16_t cmd = 0x800;
  cmd = cmd | command;

  for (int i =0; i <12; i ++){
    digitalWrite(_wrPin, LOW);
    if (cmd & 0x800){
      digitalWrite(_dataPin, HIGH);
    }
    else {
      digitalWrite(_dataPin, LOW);
    }
    cmd <<=1;
    digitalWrite(_wrPin, HIGH);
  }

  digitalWrite(_csPin, HIGH);
}

//Clear all number on LCD.
void Lcd::clear(){
  uint16_t cmd = 0x140;

  digitalWrite(_csPin, LOW);
  for (int i =0; i<9; i++){
    digitalWrite(_wrPin, LOW);
    if (cmd & 0x100){
      digitalWrite(_dataPin, HIGH);
    }
    else {
      digitalWrite(_dataPin, LOW);
    }
    cmd <<=1;
    digitalWrite(_wrPin, HIGH);
  }

  for (int i =0 ; i<32; i++){
    digitalWrite(_wrPin, LOW);
    digitalWrite(_dataPin, LOW);
    digitalWrite(_wrPin, HIGH);
  }
  digitalWrite(_csPin, HIGH);
}

//diplay integer number.
void Lcd::printH(uint8_t number){

  if (number > 99){
    number = 99;
  }

  //snprintf(localBuffer, 5, "%4li", number); //convert to char.
  dtostrf(number,5,number < 10 ? 2:3,localBuffer);//convert to string

  update(1);
}

//display temperature in C
void Lcd::printC(int16_t number){

  dtostrf((number / 10.0),5,3,localBuffer);//convert to string

  //find dot position
  for (int i =0; i<4;i++){
    if(localBuffer[i] == '.'){
      _dotPosVal = i;
    }
  }

  //remove "." from string
  for (int i =_dotPosVal; i< 5; i ++){
    localBuffer[i] = localBuffer[i+1];
  }
  update(2);
}

//update the LCD segment
void Lcd::update(int type){
  //clear array
  for (int i =0; i< 4; i ++){
    dispNumArray[i] =0;
  }

  //maping number to lcd segment.
  for (int i = 0 ; i < 4; i ++){
    switch(localBuffer[i]){
      case '0':
      dispNumArray[i] = 0xFC; // 11111100
      break;
      case '1':
      dispNumArray[i] = 0x60; // 01100000
      break;
      case '2':
      dispNumArray[i] = 0xDA; // 11011010
      break;
      case '3':
      dispNumArray[i] = 0xF2; // 11110010
      break;
      case '4':
      dispNumArray[i] = 0x66; // 01100110
      break;
      case '5':
      dispNumArray[i] = 0xB6; // 10110110
      break;
      case '6':
      dispNumArray[i] = 0xBE; // 10111110
      break;
      case '7':
      dispNumArray[i] = 0xE0; // 11100000
      break;
      case '8':
      dispNumArray[i] = 0xFE; // 11111110
      break;
      case '9':
      dispNumArray[i] = 0xF6; // 11110110
      break;
      case '-':
      dispNumArray[i] = 0x02; // 00000010
      break;
    }
  }

  switch (type){
    case 1: //for humidity
      dispNumArray[2] = 0x0A; // r
      dispNumArray[3] = 0x6E; // H
    break;

    case 2: //for temp C format
      dispNumArray[_dotPosVal-1] |= 0x01;
      dispNumArray[3] = 0x9D; //print *C
      break;
  }

  //sending 101 to inform HT1621 that MCU send data then follow by address 000000.
  uint16_t cmd =0x140;
  digitalWrite(_csPin, LOW);
  for(int i = 0 ;i < 9;i ++){
    digitalWrite(_wrPin, LOW);
    if (cmd & 0x100){
      digitalWrite(_dataPin, HIGH);
    }
    else {
      digitalWrite(_dataPin, LOW);
    }
    cmd <<=1;
    digitalWrite(_wrPin, HIGH);
  }

  //sending 32 bit of segment to LCD RAM.
  for (int i =0; i < 8; i++){
    for (int j =0; j<4;j++){
      digitalWrite(_wrPin,LOW);
      bufferMem =dispNumArray[j]<<i;
      if (bufferMem & 0x80){
        digitalWrite(_dataPin, HIGH);
      }
      else {
        digitalWrite(_dataPin, LOW);
      }
      digitalWrite(_wrPin, HIGH);
    }
  }
  digitalWrite(_csPin, HIGH);
}

