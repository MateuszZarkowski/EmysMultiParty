/* This file is part of USpeech.
* USpeech  is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This module is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License.
*  If not, see <http://www.gnu.org/licenses/>.
*******************************************
*
*	USpeech 
*   Simple module to TTS based on Microsoft MSP or SAPI
*   author: Jan Kedzierski, Wroclaw University of Technology
*
*******************************************
*/

#include <urbi/uobject.hh>
#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <sphelper.h>
#include <spuihelp.h>
#include <string>

// For speech APIs
#include <sapi.h>
#include <sphelper.h>
// NOTE: To ensure that application compiles and links against correct SAPI versions (from Microsoft Speech
//       SDK), VC++ include and library paths should be configured to list appropriate paths within Microsoft
//       Speech SDK installation directory before listing the default system include and library directories,
//       which might contain a version of SAPI that is not appropriate for use together with Kinect sensor.
//		 Global Unique ID (GUID) is a unique reference number used as an identifier in computer software (in this 
//		 case to identifier Microsoft Speech Platform).
//
#define INITGUID
#include <guiddef.h>  
#ifdef SPEECH_SAPI 
	DEFINE_GUID(CLSID_ExpectedSpeech, 0x96749377, 0x3391, 0x11D2, 0x9E, 0xE3, 0x00, 0xC0, 0x4F, 0x79, 0x73, 0x96);  // SAPI 5
#endif
#ifdef SPEECH_MSP
	DEFINE_GUID(CLSID_ExpectedSpeech, 0xd941651c, 0x44e6, 0x4c17, 0xba, 0xdf, 0xc3, 0x68, 0x26, 0xfc, 0x34, 0x24);  // MSP 11 (SAPI 5.5)
#endif

using namespace urbi;
using namespace std;


class USpeech: public urbi::UObject
{
  public:

	// URBI Interface
    USpeech(const std::string& str);
	~USpeech();

    int init();
	wstring strtowstr(const std::string &str);

	vector<char *> AvailableVoices();
	int Speak(std::string&);
	void Poll();
	
	UVar visemeId;
	UVar nextVisemeId;
	UVar visemeTime;
	UVar visemeTrig;
    UVar isSpeaking;
    UVar progress;
	UVar voiceNo;
	UVar volume;
	UVar rate;
	UVar pitch;

	UVar debug;


	// Private 
	void setRate();
	void setVolume();
	void setVoice();

	HRESULT hr;
	HANDLE 	speechEvent;
	CSpEvent spEvent;
	ISpVoice * Speech;
	IEnumSpObjectTokens*  cpIEnum;
    int speechLength;
    int startPosition;
};