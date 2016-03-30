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
*******************************************
*
*	URecog
*   Simple module to Speech Recognition based on Microsoft Speech SDK
*   authors:
*			Jan Kedzierski,
*			Mateusz Zarkowski
*			@Wroclaw University of Technology
*   date: 17.06.2014
*	ver: 2.77
*
*******************************************
*/

#include "URecog.h"

using namespace urbi;
using namespace std;


URecog::URecog(const std::string& s)
  : urbi::UObject(s) {
  UBindFunction(URecog, init);
  UBindFunctions(URecog, AvailableRecognizers, AvailableInputs);
  UBindThreadedFunction(URecog, LoadGrammar, LOCK_INSTANCE);
  UBindThreadedFunction(URecog, ResetGrammar, LOCK_INSTANCE);
  UBindThreadedFunction(URecog, AddPhraseGrammar, LOCK_INSTANCE);
  UBindThreadedFunction(URecog, SetDictationState, LOCK_FUNCTION);
  UBindThreadedFunction(URecog, Poll, LOCK_FUNCTION);
  UBindThreadedFunction(URecog, ClearResult, LOCK_FUNCTION);

  UBindVars(URecog, result, resultTag, semantics, isListening, confidenceThreshold, confidence, pause, debug);
};


URecog::~URecog() {

  cpRecoGrammar->SetDictationState(SPRS_INACTIVE);
  cpRecoGrammar->UnloadDictation();

  pRecog.Release();
  cpRecoContext.Release();
  cpRecoGrammar.Release();
  CoUninitialize();
}

int URecog::init(const std::string& engine_in, const int recognizer, const int input) {

  isListening = 0;
  confidence = 0;
  confidence.rangemin = 0;
  confidence.rangemax = 1;

  confidenceThreshold = 0;
  pause = false;
  pause.rangemin = 0;
  pause.rangemax = 1;
  debug = false;

  if (engine_in == "InShared")
    engine = engine_in;
  else
    engine = "InProc";

  cout << "[URecog] **************************************************************" << endl;
#ifdef SPEECH_MSP 
  cout << "[URecog] **	       U R e c o g    f o r    M S P 11 " << endl;
#endif
#ifdef SPEECH_SAPI 
  cout << "[URecog] **	       U R e c o g    f o r    S A P I 5 " << endl;
#endif
  cout << "[URecog] **    author: Jan Kedzierski" << endl;
  cout << "[URecog] **   address: Wroclaw University of Technology" << endl;
  cout << "[URecog] **   version: 2.76 " << endl;
  cout << "[URecog] **      date: 17.06.2015" << endl;
  cout << "[URecog] **" << endl;

  // Initialize the COM library on the current thread.

  CoUninitialize();

  hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

  if (FAILED(hr)) {
    CoUninitialize();
    cerr << "[URecog] ERROR: Failed to initialize the COM library." << endl;
    return 1;
  }

  if (CLSID_ExpectedRecognizer != CLSID_SpInprocRecognizer) {
#ifdef SPEECH_MSP 
    cerr << "[URecog] ERROR: Test speech version failed.\nThis module was compiled against an incompatible version of Microsoft Speech Platform SDK (ver. 11.0).\nPlease ensure that Microsoft Speech SDK and other sample requirements are installed." << endl;
#endif
#ifdef SPEECH_SAPI 
    cerr << "[URecog] ERROR: Test speech version failed.\nThis module was compiled against an incompatible version of Microsoft SAPI SDK (ver. 5.3).\nPlease ensure that Microsoft Speech SDK and other sample requirements are installed." << endl;
#endif
    return false;
  }

  if (engine == "InShared")
    hr = pRecog.CoCreateInstance(CLSID_SpSharedRecognizer);
  else
    hr = pRecog.CoCreateInstance(CLSID_SpInprocRecognizer);

  if (FAILED(hr)) {
    cout << "[URecog] ERROR: CoCreateInstance error." << endl;
    return 1;
  }


  (void)AvailableInputs();
  (void)AvailableRecognizers();

  ////////////////////////////////////////////////

  ISpObjectToken* cpToken(NULL);

  ULONG tmp;
  cpIEnumInputs->GetCount(&tmp);
  if (input >= (int)tmp) {
    cout << "[URecog] ERROR: There is no given input number!" << endl;
    return 1;
  }

  hr = cpIEnumInputs->Item((ULONG)input, &cpToken);
  if (FAILED(hr)) {
    cout << "[URecog] ERROR: Can NOT get choosen input!" << endl;
    return 1;
  }

  if (engine != "InShared") {
    hr = pRecog->SetInput(cpToken, true);
    if (FAILED(hr)) {
      cout << "[URecog] ERROR: Can NOT set given input." << endl;
      return 1;
    }
  }

  cpToken->Release();

  ////////////////////////////////////////////////

  cpIEnum->GetCount(&tmp);
  if (recognizer >= (int)tmp) {
    cout << "[URecog] ERROR: There is no given recognizer number!" << endl;
    return 1;
  }

  hr = cpIEnum->Item((ULONG)recognizer, &cpToken);
  if (FAILED(hr)) {
    cout << "[URecog] ERROR: Can NOT get choosen recognizer!" << endl;
    return 1;
  }

  hr = pRecog->SetRecognizer(cpToken);
  if (FAILED(hr)) {
    cout << "[URecog] ERROR: Can NOT set choosen recognizer!" << endl;
    return 1;
  }

  cpToken->Release();

  ////////////////////////////////////////////////

  hr = pRecog->CreateRecoContext(&cpRecoContext);
  if (FAILED(hr)) {
    cout << "[URecog] ERROR: CreateRecoContext error." << endl;
    return 1;
  }


  hr = cpRecoContext->CreateGrammar(1, &cpRecoGrammar);
  if (FAILED(hr)) {
    cout << "[URecog] ERROR: CreateGrammar error." << endl;
    return 1;
  }

  const ULONGLONG ullInterest = SPFEI(SPEI_SOUND_START) | SPFEI(SPEI_SOUND_END) | SPFEI(SPEI_RECOGNITION)
    /* | SPFEI(SPEI_INTERFERENCE) |
   SPFEI(SPEI_false_RECOGNITION) | SPFEI(SPEI_HYPOTHESIS) |
   SPFEI(SPEI_RECO_OTHER_CONTEXT) |SPFEI(SPEI_PHRASE_START) |
   SPFEI(SPEI_REQUEST_UI) | SPFEI(SPEI_RECO_STATE_CHANGE) |
   SPFEI(SPEI_PROPERTY_NUM_CHANGE) | SPFEI(SPEI_PROPERTY_STRING_CHANGE)*/;

  hr = cpRecoContext->SetInterest(ullInterest, ullInterest);
  if (FAILED(hr)) {
    cout << "[URecog] ERROR: SetInterest error." << endl;
    return 1;
  }

  ////////////////////////////////////////////////

  speechEvent = cpRecoContext->GetNotifyEventHandle();


  UNotifyThreadedChange(pause, &URecog::setPause, LOCK_FUNCTION);

  cout << "[URecog] OK: Speech recognition initialized correctly." << endl;

  return 0;
};


bool URecog::setPause() {
  if (cpRecoContext == NULL) return false;
  if (pause) {
    hr = cpRecoContext->SetInterest(NULL, NULL);
  } else {
    const ULONGLONG ullInterest = SPFEI(SPEI_SOUND_START) | SPFEI(SPEI_SOUND_END) | SPFEI(SPEI_RECOGNITION);
    hr = cpRecoContext->SetInterest(ullInterest, ullInterest);
  }

  if (FAILED(hr)) {
    cout << "[URecog] ERROR: Pause error." << endl;
    return false;
  }
  if (debug) cout << "[URecog] DEBUG: Pause: " << pause.as<bool>() << endl;
  return true;
}

void URecog::ClearResult() {
  result = "";
  resultTag = "";
  semantics = boost::unordered_map<string, string>();
}

vector<char *> URecog::AvailableRecognizers() {
  ISpObjectToken* pToken;
  vector<char *> list;
  ULONG sizel;


  list.clear();

  // Enumerate all available recognizers.
  hr = SpEnumTokens(SPCAT_RECOGNIZERS, NULL, NULL, &cpIEnum);
  if (FAILED(hr)) {
    cout << "[URecog] ERROR: Can NOT get recognizers list!" << endl;
    return  list;
  }

  cpIEnum->GetCount(&sizel);
  if (sizel <= 0) {
    cout << "[URecog] ERROR: No available recognizers." << endl;
    return list;
  } else
    while (cpIEnum->Next(1, &pToken, NULL) == S_OK) {
      CSpDynamicString dstrText;
      hr = SpGetDescription(pToken, &dstrText);
      if (SUCCEEDED(hr))
        list.push_back(dstrText.CopyToChar());
      else
        cout << "[URecog] ERROR: SpGetDescription error." << endl;
      pToken->Release();
      dstrText.Clear();
      delete dstrText;
    }

  return list;
}


vector<char *> URecog::AvailableInputs() {
  ISpObjectToken* pToken;
  vector<char *> list;
  ULONG sizel;


  list.clear();

  // Enumerate all available inputs.
  hr = SpEnumTokens(SPCAT_AUDIOIN, NULL, NULL, &cpIEnumInputs);
  if (FAILED(hr)) {
    cout << "[URecog] ERROR: Can NOT get recognizers list!" << endl;
    return  list;
  }

  cpIEnumInputs->GetCount(&sizel);
  if (sizel <= 0) {
    cout << "[URecog] ERROR: No available inputs." << endl;
    return list;
  } else
    while (cpIEnumInputs->Next(1, &pToken, NULL) == S_OK) {
      CSpDynamicString dstrText;
      hr = SpGetDescription(pToken, &dstrText);
      if (SUCCEEDED(hr))
        list.push_back(dstrText.CopyToChar());
      else
        cout << "[URecog] ERROR: SpGetDescription error." << endl;
      pToken->Release();
      dstrText.Clear();
      delete dstrText;
    }

  return list;
}



bool URecog::LoadGrammar(const string& xml_grammar) {
  if ((cpRecoGrammar == NULL) || (cpRecoContext == NULL)) return false;


  const wstring text = wstring(xml_grammar.begin(), xml_grammar.end());

  hr = cpRecoGrammar->LoadCmdFromFile(text.c_str(), SPLO_STATIC);
  if (FAILED(hr)) {
    cerr << "[URecog] ERROR: Can not load speech grammar." << endl;
    return false;
  }

  // Specify that all top level rules in grammar are now active
  hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);
  if (FAILED(hr)) {
    cerr << "[URecog] ERROR: Can not set all top level rules in active." << endl;
    return false;
  }

  ClearResult();

  cout << "[URecog] INFO: Grammar loaded successful." << endl;
  return true;
}

//! Add word to recognition
bool URecog::AddPhraseGrammar(const string& s_Rule, const string& s_Word) {

  if ((cpRecoGrammar == NULL) || (cpRecoContext == NULL)) return false;

  // Declare local identifiers:
  HRESULT hr;
  SPSTATEHANDLE hState;
  if ((cpRecoGrammar == NULL) || (cpRecoContext == NULL)) return false;
  hr = cpRecoContext->Resume(NULL);

  int p = s_Rule.length();
  BSTR w_Rule = SysAllocStringLen(NULL, p);
  MultiByteToWideChar(CP_ACP, 0, s_Rule.c_str(), p, w_Rule, p);

  p = s_Word.length();
  BSTR w_Word = SysAllocStringLen(NULL, p);
  MultiByteToWideChar(CP_ACP, 0, s_Word.c_str(), p, w_Word, p);

  hr = cpRecoGrammar->GetRule(w_Rule, 0, SPRAF_TopLevel | SPRAF_Active, true, &hState);
  if (FAILED(hr)) cout << "[URecog] ERROR: GetRule error." << endl;

  hr = cpRecoGrammar->AddWordTransition(hState, NULL, w_Word, L" -.", SPWT_LEXICAL, 0.001, NULL);
  if (FAILED(hr)) cout << "[URecog] ERROR: AddWordTransition error." << endl;

  hr = cpRecoGrammar->Commit(NULL);
  if (FAILED(hr)) cout << "[URecog] ERROR: Commit error." << endl;

  hr = cpRecoGrammar->SetGrammarState(SPGS_ENABLED);
  if (FAILED(hr)) cout << "[URecog] ERROR: SetGrammarState error." << endl;

  hr = cpRecoGrammar->SetRuleState(0, 0, SPRS_ACTIVE);
  if (FAILED(hr)) cout << "[URecog] ERROR: SetRuleState error." << endl;

  ClearResult();

  return true;
}

bool URecog::ResetGrammar() {
  if ((cpRecoGrammar == NULL) || (cpRecoContext == NULL)) return false;

  hr = cpRecoGrammar->ResetGrammar(NULL);
  if (FAILED(hr)) {
    cout << "[URecog] ERROR: ResetGrammar error." << endl;
    return false;
  }

  cout << "[URecog] INFO: Grammar reset successful." << endl;

  ClearResult();

  return true;
}

bool URecog::SetDictationState(int tmp) {
  if (cpRecoGrammar == NULL) return false;
  hr = cpRecoGrammar->SetDictationState((SPRULESTATE)tmp);
  if (FAILED(hr)) {
    cout << "[URecog] ERROR: SetDictationState error." << endl;
    return false;
  }
  return true;
}




void URecog::Poll(bool wait) {

  int t_wait = 0;
  if (wait) t_wait = INFINITE;

  if (WAIT_OBJECT_0 == WaitForSingleObject(speechEvent, t_wait)) {
    CSpEvent spevent;
    while (S_OK == spevent.GetFrom(cpRecoContext)) {
      //switch (curEvent.eEventId)
      switch (spevent.eEventId) {
      case SPEI_RECOGNITION:

        if (SPET_LPARAM_IS_OBJECT == spevent.elParamType) {
          // this is an ISpRecoResult
          ISpRecoResult* rResult = reinterpret_cast<ISpRecoResult*>(spevent.lParam);

          SPPHRASE* pPhrase = NULL;
          hr = rResult->GetPhrase(&pPhrase);
          if (FAILED(hr)) {
            cerr << "[URecog] ERROR: Can not get phrase." << endl;
            return;
          }

          WCHAR *pwszText;
          hr = rResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &pwszText, NULL);

          if (SUCCEEDED(hr)) {

            if (pwszText != NULL) result = utf8_encode(pwszText);

            // for debug:
            if (debug) cout << "[URecog] DEBUG: Recognized: " << result.as<string>() << endl;

            if (pPhrase->pProperties != NULL) {
              //**************** Semantics Filling **********************
              // TODO: add nested semantics, now its first level only <- limited by boost::map strong typing
              const SPPHRASEPROPERTY* pSemanticTag = pPhrase->pProperties;
              while (pSemanticTag->pFirstChild != NULL)
                pSemanticTag = pSemanticTag->pFirstChild;

              if (pSemanticTag->SREngineConfidence > static_cast<float>(confidenceThreshold.as<double>())) {

                boost::unordered_map<string, string> semanticMap;

                confidence = pSemanticTag->SREngineConfidence;
                if (debug) cout << "[URecog] DEBUG: Confidence: " << confidence.as<double>() << endl;
                semanticMap["Confidence"] = confidence;

                while (pSemanticTag != NULL) {
                  if (pSemanticTag->pszValue != NULL)
                    semanticMap[utf8_encode(pSemanticTag->pszName)] = utf8_encode(pSemanticTag->pszValue);
                  pSemanticTag = pSemanticTag->pNextSibling;
                }

                //***backward compatibility hack
                if (semanticMap.find("_value") != semanticMap.end()) {
                  semanticMap["Tag"] = semanticMap["_value"];
                  semanticMap.erase("_value");
                } //***					

                if (debug) cout << "[URecog] DEBUG: Semantics count: " << semanticMap.size() << endl;
                semantics = semanticMap;

                resultTag = semanticMap["Tag"];

                if (debug) cout << "[URecog] DEBUG: ResultTag: " << resultTag.as<string>() << endl;

              }
            }

          }
          CoTaskMemFree(pPhrase);
        }
        break;

      case SPEI_SOUND_START:
        isListening = 1;
        // for debug:
        if (debug) cout << "[URecog] DEBUG: Start listening." << endl;
        break;

      case SPEI_SOUND_END:
        isListening = 0;
        ClearResult();
        // for debug:
        if (debug) cout << "[URecog] DEBUG: Stop listening." << endl;
        break;
      }
    }
  }
}


string URecog::utf8_encode(const std::wstring &wstr) {
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
  string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
  return strTo;
}


UStart(URecog);
