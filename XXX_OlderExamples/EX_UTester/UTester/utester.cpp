/**********************************************************
 **********************************************************
 **
 **				..:: UTester ::.. 
 **
 **		Very simple module for communication with
 **		evaluation board with MC9S12A64.
 **
 **    ver.: 1.0 
 **    date: 22.06.2012
 **  author: Jan Kedzierski, Adam Oleksy, Wroclaw University of Technology
 ********************************************************* 
 ********************************************************/

#include <urbi/uobject.hh>
#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include <sstream>
#include <cstdlib>

using namespace std;
using namespace urbi;

class UTester: public UObject
{
public:
    UTester(const string& str);
    void init();

	UVar intVar;
	UVar stringVar;
	UVar listVar;
	UVar dictVar;

	void Poll();


	double add(double rhs) const;

	/*
	// Open and close port
	bool open(const char *, DWORD , int, int, int);
	bool close();
	// write to port functions
	bool write(int);
	bool writeStr(const char*);
	bool writeVec(vector<int>);
	// read from port functions
	int read();
	string readStr();
	vector<int> readVec();
	// get waitning numbers of bytes to read
	DWORD BytesToRead();

	bool setColor(int red, int green, int blue);
	UVar led1;
	UVar led2;
	UVar led3;
	UVar servo;
	UVar potentiometer;
	UVar button;
    */
private:

};


UTester::UTester(const string& s) : urbi::UObject(s)
{
	UBindFunction(UTester, init);
};

void UTester::init()
{

	UBindVar(UTester, intVar);
	UBindVar(UTester, stringVar);
	UBindVar(UTester, listVar);
	UBindVar(UTester, dictVar);

	intVar = 31;
	stringVar = "Szatan";
	static const int arr[] = {16,2,77,29};
	listVar = vector<int>(arr, arr + sizeof(arr) / sizeof(arr[0]) );

	
	boost::unordered_map<std::string, int> map1;
	map1["f1"] = 1;
	map1["f2"] = 2;
	dictVar = map1;
	

	UBindFunction(UTester, add);
	UBindFunction(UTester, Poll);
	/*
	mRed = mGreen = mBlue = 0;

	// bind all functions to urbi component
	UBindFunction(UTester, open);
    UBindFunction(UTester, close);
	UBindFunction(UTester, write);
	UBindFunction(UTester, writeStr);
	UBindFunction(UTester, writeVec);
	UBindFunction(UTester, read);
	UBindFunction(UTester, readStr);
	UBindFunction(UTester, readVec);
	UBindFunction(UTester, BytesToRead);
	connected = false; // not connected

	UBindFunction(UTester, setColor);
	UBindVar(UTester, led1);
	UBindVar(UTester, led2);
	UBindVar(UTester, led3);
	UBindVar(UTester, servo);
	UBindVar(UTester, potentiometer);
	UBindVar(UTester, button);

	UNotifyChange(led1, &UTester::setState);
	UNotifyChange(led2, &UTester::setState);
	UNotifyChange(led3, &UTester::setState);
	UNotifyChange(servo, &UTester::setState);

	UNotifyAccess(potentiometer, &UTester::getPotentiometer);
	UNotifyAccess(button, &UTester::getButton);
	*/
}

double UTester::add(double rhs) const{
	return ((double) intVar) + rhs;
}

void UTester::Poll(){
	boost::unordered_map<std::string, int> map1;
	for( int i=rand()%10; i < intVar.as<int>(); i+=10){
		stringstream ss;
		ss << i;
		map1[ss.str()] = i;
	}
	dictVar = map1;
}

/*

bool UTester::setState() {
	if(!connected)
		return false;

	vector<int> frame(9, 0);

	// Set frame
	frame[0] = 255;
	frame[1] = 1;
	frame[2] = led1.as<bool>();
	frame[3] = led2.as<bool>();
	frame[4] = led3.as<bool>();
	frame[5] = mRed;
	frame[6] = mGreen;
	frame[7] = mBlue;
	frame[8] = servo;

	// Write frame to port
	writeVec(frame);
	// usleep(mDelay);
	// frame = readVec();

	return true;
	//return frame.size() == 1;
}

bool UTester::getPotentiometer() {
	if(!connected)
		return false;

	vector<int> frame(9, 0);

	// Set frame
	frame[0] = 255;
	frame[1] = 2;
	frame[2] = 1;

	// Write frame to port
	writeVec(frame);
	usleep(mDelay);
	frame = readVec();

	if(frame.size() == 3) {
		potentiometer = frame[2];
		return true;
	} else
		return false;
}

bool UTester::getButton() {
	if(!connected)
		return false;

	vector<int> frame(9, 0);

	// Set frame
	frame[0] = 255;
	frame[1] = 3;
	frame[2] = 1;

	// Write frame to port
	writeVec(frame);
	usleep(mDelay);
	frame = readVec();

	if(frame.size() == 3) {
		button = frame[2];
		return true;
	} else
		return false;
}
*/

//
UStart(UTester);
