/*******************************************
*
*	UKinect 
*   Author: Jan Kedzierski
*
********************************************/

#include "UKinect.h"


/// <summary>
/// Create speech recognizer that will read Kinect audio stream data.
/// </summary>
/// <returns>
/// <para>S_OK on success, otherwise failure code.</para>
/// </returns>
bool UKinect::setSpeechStream()
{

	if (CLSID_ExpectedRecognizer != CLSID_SpInprocRecognizer)
	{
		cerr <<"[UKinect] ERROR: Test speech (MSP) version failed.\nThis module was compiled against an incompatible version of Microsoft Speech Platform SDK (ver. 11.0).\nPlease ensure that Microsoft Speech SDK and other sample requirements are installed." << endl;
		return false;
	} 

	

	IStream* pStream = NULL;

	hr = pKinectAudioStream->QueryInterface(IID_IStream, (void**)&pStream);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Set audio output type." << endl;
		return false;
	}

	speechStream = NULL;

	hr = CoCreateInstance(CLSID_SpStream, NULL, CLSCTX_INPROC_SERVER, __uuidof(ISpStream), (void**)&speechStream);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Set audio output type." << endl;
		return false;
	}

	WAVEFORMATEX wfxOut = {AudioFormat, AudioChannels, AudioSamplesPerSecond, AudioAverageBytesPerSecond, AudioBlockAlign, AudioBitsPerSample, 0};

	hr = speechStream->SetBaseStream(pStream, SPDFID_WaveFormatEx, &wfxOut);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Set audio output type." << endl;
		return false;
	}

	SafeRelease(pStream);
	
	hr = CoCreateInstance(CLSID_SpInprocRecognizer, NULL, CLSCTX_INPROC_SERVER, __uuidof(ISpRecognizer), (void**)&pSpeechRecognizer);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not set recognizer instance." << endl;
		return false;
	}
	

	hr = pSpeechRecognizer->SetInput(speechStream, FALSE);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not set recognizer input." << endl;
		return false;
	}

	ISpObjectToken *pEngineToken = NULL;
	IEnumSpObjectTokens*  cpIEnum = NULL;

	vector<char *> list;
	list.clear();


	// Enumerate all available recognizers.
	hr = SpEnumTokens(SPCAT_RECOGNIZERS, NULL, NULL, &cpIEnum);
	if(FAILED(hr)) { 
		cout << "[UKinect] ERROR: Can NOT get recognizers list!" << endl; 
	}

	ULONG sizel;
	cpIEnum->GetCount(&sizel);
	if (sizel <=0)
	{
		cout << "[UKinect] ERROR: No available recognizers." << endl; 
	} else {
		while (cpIEnum->Next(1, &pEngineToken, NULL) == S_OK)
		{
			CSpDynamicString dstrText;
			hr = SpGetDescription(pEngineToken, &dstrText);
			if (SUCCEEDED(hr))
				list.push_back(dstrText.CopyToChar());
			else
				cout << "[UKinect] ERROR: SpGetDescription error." << endl; 
			pEngineToken->Release();
			dstrText.Clear();
			delete dstrText;
		}
	}

	speechAvailableRecognizers = list;

	if(speechRecognizer.as<int>()>=0){
		hr = cpIEnum->Item((ULONG)speechRecognizer.as<int>(), &pEngineToken);
		if(FAILED(hr)) { 
			cout << "[URecog] ERROR: Can NOT get choosen token!" << endl; 
			return 1;
		}
	} else {

		hr = SpFindBestToken(SPCAT_RECOGNIZERS,L"Language=409;Kinect=True",NULL,&pEngineToken);
		if (FAILED(hr))
		{
			cerr <<"[UKinect] ERROR: Can not find any token." << endl;
			return false;
		}
	}

	hr = pSpeechRecognizer->SetRecognizer(pEngineToken);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not set recognizer token." << endl;
		return false;
	}
	
	SafeRelease(cpIEnum);
	SafeRelease(pEngineToken);

	hr = pSpeechRecognizer->CreateRecoContext(&pSpeechContext);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not set create speech context." << endl;
		return false;
	}

	// For long recognition sessions (a few hours or more), it may be beneficial to turn off adaptation of the acoustic model. 
	// This will prevent recognition accuracy from degrading over time.
	//if (SUCCEEDED(hr))
	//{
	//    hr = m_pSpeechRecognizer->SetPropertyNum(L"AdaptationOn", 0);                
	//}

	hr = pSpeechContext->CreateGrammar(1, &pSpeechGrammar);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not create speech grammar." << endl;
		return false;
	}


	/*	
	// Specify that engine should always be reading audio
	hr = pSpeechRecognizer->SetRecoState(SPRST_ACTIVE);
	if (FAILED(hr))
	{
	cerr <<"[UKinect] ERROR: Can not set engine to alway reading audio." << endl;
	return false;
	}
	*/

	const ULONGLONG ullInterest = SPFEI(SPEI_SOUND_START) | SPFEI(SPEI_SOUND_END) | SPFEI(SPEI_RECOGNITION)
		/* | SPFEI(SPEI_INTERFERENCE) | 
		SPFEI(SPEI_false_RECOGNITION) | SPFEI(SPEI_HYPOTHESIS) |
		SPFEI(SPEI_RECO_OTHER_CONTEXT) |SPFEI(SPEI_PHRASE_START) |
		SPFEI(SPEI_REQUEST_UI) | SPFEI(SPEI_RECO_STATE_CHANGE) | 
		SPFEI(SPEI_PROPERTY_NUM_CHANGE) | SPFEI(SPEI_PROPERTY_STRING_CHANGE)*/;

	// Specify that we're only interested in receiving recognition events
	hr = pSpeechContext->SetInterest(ullInterest, ullInterest);

	speechEvent = pSpeechContext->GetNotifyEventHandle();

	cout <<"[UKinect] INFO: Speech stream has been created. "<< endl;


	return true;
}



void UKinect::speechSetPause()
{
	if (pSpeechContext==NULL) return;
	if (speechPause.as<bool>()) {
		hr = pSpeechContext->SetInterest(NULL, NULL);
	} else {
		const ULONGLONG ullInterest = SPFEI(SPEI_SOUND_START) | SPFEI(SPEI_SOUND_END) | SPFEI(SPEI_RECOGNITION);
		hr = pSpeechContext->SetInterest(ullInterest, ullInterest);
	}

	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not pause/resume speech recognition." << endl;
	} 

	if (debug) cout << "Pause: "<< speechPause.as<bool>() <<endl;
}

/// <summary>
/// Load speech recognition grammar into recognizer.
/// </summary>
/// <returns>
/// <para>S_OK on success, otherwise failure code.</para>
/// </returns>
bool UKinect::speechLoadGrammar(const string& xml_grammar)
{
	if ((pSpeechGrammar==NULL)||(pSpeechContext==NULL)) return false;

	const wstring text= wstring(xml_grammar.begin(), xml_grammar.end());

	hr = pSpeechGrammar->LoadCmdFromFile(text.c_str(), SPLO_STATIC);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not load speech grammar." << endl;
		return false;
	}

	// Specify that all top level rules in grammar are now active
	hr = pSpeechGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not set all top level rules in active." << endl;
		return false;
	}

	speechResult="";
	speechResultTag="";
	return true;
}

bool UKinect::speechResetGrammar(){

	if ((pSpeechGrammar==NULL)||(pSpeechContext==NULL)) return false;

	hr = pSpeechGrammar->ResetGrammar(NULL);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not reset speech grammar." << endl;
		return false;
	}

	speechResult="";
	speechResultTag="";
	return true;
}


//! Add word to recognition
bool UKinect::speechAddPhrase( const string& s_Rule, const string& s_Word)
{
	if ((pSpeechGrammar==NULL)||(pSpeechContext==NULL)) return false;

	// Declare local identifiers:
	HRESULT hr;
	SPSTATEHANDLE hState;

	const wstring w_Word= wstring(s_Word.begin(), s_Word.end());
	const wstring w_Rule= wstring(s_Rule.begin(), s_Rule.end());


	hr = pSpeechGrammar->GetRule(w_Rule.c_str(), 0, SPRAF_TopLevel | SPRAF_Active, TRUE, &hState); 
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not get grammar rule." << endl;
		return false;
	}

	hr = pSpeechGrammar->AddWordTransition(hState, NULL, w_Word.c_str(), L" ", SPWT_LEXICAL, 1, NULL);

	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not set word transition." << endl;
		return false;
	}

	hr = pSpeechGrammar->Commit(NULL);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not commit grammar." << endl;
		return false;
	}

	hr = pSpeechGrammar->SetGrammarState(SPGS_ENABLED);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not set grammar state." << endl;
		return false;
	}

	hr = pSpeechGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not set all top level rules in active." << endl;
		return false;
	}

	return true;
}





/// <summary>
/// Process recently triggered speech recognition events.
/// </summary>
void UKinect::pollSpeech()
{

	hr = S_OK;

	CSpEvent spevent;
	while (S_OK == spevent.GetFrom(pSpeechContext))
	{
		//switch (curEvent.eEventId)
		switch (spevent.eEventId)
		{
		case SPEI_RECOGNITION:

			if (SPET_LPARAM_IS_OBJECT == spevent.elParamType)
			{
				// this is an ISpRecoResult
				ISpRecoResult* rResult = reinterpret_cast<ISpRecoResult*>(spevent.lParam);

				SPPHRASE* pPhrase = NULL;
				hr = rResult->GetPhrase(&pPhrase);
				if (FAILED(hr))
				{
					cerr <<"[UKinect] ERROR: Can not get phrase." << endl;
					return;
				}

				WCHAR *pwszText;
				hr = rResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &pwszText, NULL);

				if (SUCCEEDED(hr))
				{
					if (pwszText!=NULL) speechResult = utf8_encode(pwszText);

					// for debug:
					if (debug) cout << "[UKinect] DEBUG: Recognized: " << speechResult.as<string>() << endl ;

					if (pPhrase->pProperties != NULL)
						{
							const SPPHRASEPROPERTY* pSemanticTag = pPhrase->pProperties;//->pFirstChild;
							if (pSemanticTag->SREngineConfidence > static_cast<float>(speechConfidenceThreshold.as<double>()))
							{
								speechConfidence = pSemanticTag->SREngineConfidence;
								if (debug) cout << "[URecog] DEBUG: Confidence: " << speechConfidence.as<double>() << endl ;
						
								while (pSemanticTag!=NULL){ 
									if (pSemanticTag->pszValue!=NULL) speechResultTag = utf8_encode(pSemanticTag->pszValue);
									pSemanticTag = pSemanticTag->pFirstChild;
								}
								if (debug) cout << "[URecog] DEBUG: ResultTag: " << speechResultTag.as<string>() << endl;
								
							}
						} else {
							speechConfidence = 0;
							speechResultTag = "";
						}
				}
				CoTaskMemFree(pPhrase);
			}
			break;

		case SPEI_SOUND_START:
			speechIsListening=1;
			// for debug:
			if (debug) cout << "[URecog] DEBUG: Start listening." << endl ;
			break;

		case SPEI_SOUND_END: 
			speechIsListening=0;
			// for debug:
			if (debug) cout << "[URecog] DEBUG: Stop listening." << endl ;
			break;
		}
	}

	return;
}



string UKinect::utf8_encode(const std::wstring &wstr)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    string strTo( size_needed, 0 );
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}
