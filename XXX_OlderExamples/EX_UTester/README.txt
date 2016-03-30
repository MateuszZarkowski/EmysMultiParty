/**********************************************************
 **********************************************************
 **
 **				..:: UBoard ::.. 
 **
 **		Very simple module for communication with
 **		evaluation board with MC9S12A64.
 **
 **    ver.: 1.0 
 **    date: 22.06.2012
 **  author: Jan Kedzierski, Adam Oleksy, Wroclaw University of Technology
 ********************************************************* 
 ********************************************************/

///////////
// open port function
//
// 
// Port 	"COM1", "COM2", "COM3"...
// BaudRate 	110, 300, 600, 1200, 2400, 4800, 9600,  14400, 19200, 38400, 56000, 57600, 115200, 128000, 256000
// ByteSize 	8
// Parity 	0 - NOPARITY, 1 - ODDPARITY, 2 - EVENPARITY, 3 - MARKPARITY, 4 - SPACEPARITY
// StopBits 	0 - ONESTOPBIT, 1 - ONESTOPBITS, 2 - TWOSTOPBITS
//
// Retrns bool value 1 on success, 0 if failed
//
//

UBoard.open(Port,BaudRate,ByteSize,Parity,StopBits);
UBoard.close;
UBoard.write(byte);
UBoard.writeStr(string);
UBoard.writeVec(vector);
UBoard.read;
UBoard.readStr;
UBoard.readVec;
UBoard.BytesToRead;

////////////////////////////////
//
// Additional functions

UBoard.setColor(red, green, blue);
UBoard.led1;
UBoard.led2;
UBoard.led3;
UBoard.servo;
UBoard.potentiometer;
UBoard.button;