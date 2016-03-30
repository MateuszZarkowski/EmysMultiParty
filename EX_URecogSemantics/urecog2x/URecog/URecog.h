/* This file is part of URecog.
// Copyright (C) 2014 Jan Kedzierski
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <urbi/uobject.hh>
#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <string>


// For speech APIs
#include <sapi.h>
#include <sphelper.h>
// NOTE: To ensure that application compiles and links against correct SAPI versions (from Microsoft Speech
//       SDK), VC++ include and library paths should be configured to list appropriate paths within Microsoft
//       Speech SDK installation directory before listing the default system include and library directories,
//       which might contain a version of SAPI that is not appropriate for use together with Kinect sensor.
//		 Global Unique ID (GUID) is a unique reference number used as an identifier in computer software (in this 
//		 case to identifier Microsoft Speech SDK).
//
#define INITGUID
#include <guiddef.h>

#ifdef SPEECH_SAPI 
	DEFINE_GUID(CLSID_ExpectedRecognizer, 0x41B89B6B, 0x9399, 0x11D2, 0x96, 0x23, 0x00, 0xC0, 0x4F, 0x8E, 0xE6, 0x28);  // SAPI 5
#endif
#ifdef SPEECH_MSP
	DEFINE_GUID(CLSID_ExpectedRecognizer, 0x495648e7, 0xf7ab, 0x4267, 0x8e, 0x0f, 0xca, 0xfb, 0x7a, 0x33, 0xc1, 0x60); // MSP 11 (SAPI 5.5)
#endif

using namespace urbi;
using namespace std;


class URecog: public urbi::UObject
{
  public:
    URecog(const std::string& str);
	~URecog();


    int init(const string&, const int recognizer, const int input);
	string URecog::utf8_encode(const std::wstring &wstr);
	bool setPause();
	void ClearResult();

	vector<char *> AvailableRecognizers();
	vector<char *> AvailableInputs();
	bool AddPhraseGrammar( const string&, const  string&);
	bool LoadGrammar(const string& );
	bool ResetGrammar();
	bool SetDictationState(int);
	void Poll(bool);

	UVar isListening;
	UVar result;
	UVar resultTag;
	UVar semantics;
	UVar confidence;
	UVar confidenceThreshold;
	UVar pause;
	UVar debug;

	string engine;
	HRESULT hr;
	CComPtr<ISpRecognizer>	pRecog;
	CComPtr<ISpRecoContext>	cpRecoContext;
	CComPtr<ISpRecoGrammar>	cpRecoGrammar;
	HANDLE 	speechEvent;
	WCHAR *		pwszText;
	CSpDynamicString	w_Result;
	char *	ch_result;
	IEnumSpObjectTokens*  cpIEnum;
	IEnumSpObjectTokens *  cpIEnumInputs;
};
