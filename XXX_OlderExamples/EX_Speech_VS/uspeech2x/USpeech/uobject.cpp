/* This file is part of USpeech .
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
*   If not, see <http://www.gnu.org/licenses/>.
*******************************************
*
*	USpeech 
*   Simple module to TTS based on Microsoft SAPI
*   author: Jan Kedzierski, Wroclaw University of Technology
*   date: 13.12.2014
*	ver: 2.17
*
*******************************************
*/

#include "uobject.h"

using namespace urbi;
using namespace std;

// USpeech constructor
USpeech::USpeech(const std::string& s): urbi::UObject(s)
{
	UBindFunction(USpeech, init);
	UBindThreadedFunction(USpeech, Speak, LOCK_NONE);
	UBindFunction(USpeech, AvailableVoices);

	UBindVars (USpeech, visemeId, nextVisemeId, visemeTime, visemeTrig, isSpeaking, voiceNo, volume, rate, pitch, debug);

};

// USpeech initialize
int USpeech::init()
{
	debug = 0;

	visemeId=0;
	visemeTime=0;
	visemeTrig=0;
	isSpeaking=0;
	voiceNo=0;
	voiceNo.rangemin=0;
	volume=100;
	volume.rangemin=0;
	volume.rangemax=100;
	rate=0;
	rate.rangemin=-10;
	rate.rangemax=10;
	pitch=0;
	pitch.rangemin=-10;
	pitch.rangemax=10;


	cout <<"[USpeech] **************************************************************" << endl;
#ifdef SPEECH_MSP 
	cout <<"[USpeech] **	       U S p e e c h    f o r    M S P 11 " << endl;
#endif
#ifdef SPEECH_SAPI 
	cout <<"[USpeech] **	       U S p e e c h    f o r    S A P I 5 " << endl;
#endif
	cout <<"[USpeech] **    author: Jan Kedzierski" << endl;
	cout <<"[USpeech] **   address: Wroclaw University of Technology" << endl;
	cout <<"[USpeech] **   version: 2.17 "<< endl;
	cout <<"[USpeech] **      date: 13.12.2014" << endl;
	cout <<"[USpeech] **" << endl;
	
	CoUninitialize();

	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	if (FAILED(hr))
	{
		CoUninitialize();
		cerr <<"[USpeech] ERROR: Failed to initialize the COM library." << endl;
		return 1;
	}

	if (CLSID_ExpectedSpeech != CLSID_SpVoice)
	{
#ifdef SPEECH_MSP 
			cerr <<"[USpeech] ERROR: Test speech version failed.\nThis module was compiled against an incompatible version of Microsoft Speech Platform SDK (ver. 11.0).\nPlease ensure that Microsoft Speech SDK and other sample requirements are installed." << endl;
#endif
#ifdef SPEECH_SAPI 
			cerr <<"[USpeech] ERROR: Test speech version failed.\nThis module was compiled against an incompatible version of Microsoft SAPI SDK (ver. 5.3).\nPlease ensure that Microsoft Speech SDK and other sample requirements are installed." << endl;
#endif
		return 1;
	} 

	vector<char*> tmp = AvailableVoices();
	
	if (tmp.size()==0){
		cerr << "[USpeech] ERROR: No available voices." << endl; 
		return  1;
	} else {
		//for (int i=0; i<tmp.size();i++) cout << "[USpeech] DEBUG: Voice no: " << i << " \"" << tmp[i] << "\"." << endl;
	}
		
	hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&Speech);
	if(FAILED(hr)) { 
		cerr << "[USpeech] ERROR:  CoCreateInstance error." << endl; 
		return  1;
	}
	
	setVoice();
	setVolume();
	setRate();

	const ULONGLONG ullInterest = SPFEI(SPEI_VISEME)|SPFEI(SPEI_START_INPUT_STREAM)|SPFEI(SPEI_END_INPUT_STREAM);

	hr = Speech->SetInterest(ullInterest,ullInterest); 
	if(FAILED(hr)) { 
		cout << "[USpeech] ERROR:  SetInterest error." << endl; 
		return  1;
	}

	speechEvent = Speech->GetNotifyEventHandle();

	UNotifyChange(rate,	&USpeech::setRate);
	UNotifyChange(volume,	&USpeech::setVolume);
	UNotifyChange(voiceNo, &USpeech::setVoice);
	
	cout <<"[URecog] OK: MSP Text-To-Speech initialized correctly.." << endl;

	return 0;
};


USpeech::~USpeech(){
	Speech->Release();
	cpIEnum->Release();
	CloseHandle(speechEvent);	
	CoUninitialize();
}

void USpeech::setRate()
{
	hr = Speech->SetRate((long)rate.as<int>());
	if(FAILED(hr)) { 
		cout << "[USpeech] ERROR: Can NOT set speech rate." << endl; 
		return;
	}
}

void USpeech::setVolume()
{

	hr = Speech->SetVolume((USHORT)volume.as<int>());
	if(FAILED(hr)) { 
		cout << "[USpeech] ERROR: Can NOT set speech volume." << endl; 
		return;
	}
}

void USpeech::setVoice()
{
	ISpObjectToken* cpToken(NULL);

	ULONG tmp;
	cpIEnum->GetCount(&tmp);
	if (voiceNo.as<int>()>=(int)tmp) {
		cout << "[USpeech] ERROR: There is no given voice number!" << endl; 
		return;
	}

	hr = cpIEnum->Item((ULONG)voiceNo.as<int>(), &cpToken);
	if(FAILED(hr)) { 
		cout << "[USpeech] ERROR: Can NOT get choosen voice!" << endl; 
		return;
	}

	hr = Speech->SetVoice(cpToken);
	if(FAILED(hr)) { 
		cout << "[USpeech] ERROR: Can NOT set choosen voice!" << endl; 
		return;
	}

	cpToken->Release();
}

vector<char *> USpeech::AvailableVoices()
{
	ISpObjectToken* pToken;
	vector<char *> list;
	ULONG sizel;


	list.clear();

	// Enumerate all available voices.
	hr = SpEnumTokens(SPCAT_VOICES, NULL, NULL, &cpIEnum);
	if(FAILED(hr)) { 
		cout << "[USpeech] ERROR: Can NOT get voices list!" << endl; 
		return  list;
	}

	cpIEnum->GetCount(&sizel);
	if (sizel <=0)
	{
		cout << "[USpeech] ERROR: No available voices." << endl; 
		return list;
	}
	else
		while (cpIEnum->Next(1, &pToken, NULL) == S_OK)
		{
			CSpDynamicString dstrText;
			hr = SpGetDescription(pToken, &dstrText);
			if (SUCCEEDED(hr))
				list.push_back(dstrText.CopyToChar());
			else
				cout << "[USpeech] ERROR: SpGetDescription error." << endl; 
			pToken->Release();
			dstrText.Clear();
			delete dstrText;
		}

		voiceNo.rangemax = list.size()-1;
		return list;
}


// Speak function 
int USpeech::Speak(std::string& text_in)
{

	wstring wtext = strtowstr(text_in); 
	std::wostringstream ws;
	ws << L"<pitch absmiddle = \"" << pitch.as<int>() << L"\" >" << wtext << L"</pitch>" ;
	const wstring str(ws.str());

	hr = Speech->Speak(str.c_str(),SPF_ASYNC|SPF_IS_XML|SPF_PURGEBEFORESPEAK, NULL );
	if(FAILED(hr)) { 
		cout << "[USpeech] ERROR: Speak error." << endl; 
		return FALSE;
	}

	Poll();

	return TRUE;
};

void USpeech::Poll(){

	while(1){
		if (WAIT_OBJECT_0 == WaitForSingleObject(speechEvent, INFINITE))
		{
			CSpEvent spevent;
			while (S_OK == spevent.GetFrom(Speech))
			{
				switch (spevent.eEventId)
				{
				case SPEI_VISEME:
					visemeId =  LOWORD(spevent.lParam);
					nextVisemeId =  LOWORD(spevent.wParam);
					visemeTime =  HIWORD(spevent.wParam);
					visemeTrig=1;
					if (debug.as<int>()>0) cout << "[USpeech] DEBUG: visemID " << visemeId.as<int>() << " visemTime " << visemeTime.as<int>() <<" ms" <<endl ;
					break;

				case SPEI_START_INPUT_STREAM:
					// for debug:
					if (debug.as<int>()>0) cout << "[USpeech] DEBUG: start speaking " << endl;
					isSpeaking=1;
					break;

				case SPEI_END_INPUT_STREAM:
					// for debug:
					if (debug.as<int>()>0) cout << "[USpeech] DEBUG: end speaking " << endl; 
					isSpeaking=0;
					return;
				}
			}
		}
	}
}


wstring USpeech::strtowstr(const std::string &str)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo( size_needed, 0 );
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}


UStart(USpeech);
