/*******************************************
*
*	UKinect 
*   Author: Jan Kedzierski
*
********************************************/

#include "UKinect.h"


/// <summary>
/// Start processing stream
/// </summary>
/// <returns>Indicates success or failure</returns>
bool UKinect::setAudioStream()
{
	IMediaObject*       pDMO;						// Media object from which Kinect audio stream is captured.
   
	// Get the audio source
	hr = sensor->NuiGetAudioSource(&pNuiAudioSource);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Failed to get Audio Source." << endl;
		return false;
	}

	hr = pNuiAudioSource->QueryInterface(IID_IMediaObject, (void**)&pDMO);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Failed to access the DMO (in audio)." << endl;
		return false;
	}

	hr = pNuiAudioSource->QueryInterface(IID_IPropertyStore, (void**)&pPropertyStore);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Failed to access the Audio Property store." << endl;
		return false;
	}

	// Set AEC-MicArray DMO system mode. This must be set for the DMO to work properly.
	// Possible values are:
	//   SINGLE_CHANNEL_AEC = 0
	//   OPTIBEAM_ARRAY_ONLY = 2
	//   OPTIBEAM_ARRAY_AND_AEC = 4
	//   SINGLE_CHANNEL_NSAGC = 5
	PROPVARIANT pvSysMode;
	PropVariantInit(&pvSysMode);
	pvSysMode.vt = VT_I4;
	pvSysMode.lVal = (LONG)(2); // Use OPTIBEAM_ARRAY_ONLY setting. Set OPTIBEAM_ARRAY_AND_AEC instead if you expect to have sound playing from speakers.
	hr = pPropertyStore->SetValue(MFPKEY_WMAAECMA_SYSTEM_MODE, pvSysMode);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not set OPTIBEAM_ARRAY_ONLY setting." << endl;
		return false;
	}
	PropVariantClear(&pvSysMode);

	/* 
	IT IS NOT NECESSARY, BECAUSE MFPKEY_WMAAECMA_SYSTEM_MODE HAS ALREADY SET
	pvSysMode.vt = VT_BOOL;
	pvSysMode.lVal = (VARIANT_TRUE);
	hr = pPropertyStore->SetValue(MFPKEY_WMAAECMA_FEATURE_MODE, pvSysMode);
	if (FAILED(hr))
	{
	cerr <<"[UKinect] ERROR: Can not set FEATURE_MODE setting." << endl;
	}
	*/


	// Set DMO output format
	WAVEFORMATEX wfxOut = {AudioFormat, AudioChannels, AudioSamplesPerSecond, AudioAverageBytesPerSecond, AudioBlockAlign, AudioBitsPerSample, 0};
	//memcpy_s (&wfxOut, sizeof(WAVEFORMATEX), &fmt, sizeof(WAVEFORMATEX));
	DMO_MEDIA_TYPE mt = {0};
	hr = MoInitMediaType(&mt, sizeof(WAVEFORMATEX));
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Init audio media type structure." << endl;
		return false;
	}

	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = MEDIASUBTYPE_PCM;
	mt.lSampleSize = 0;
	mt.bFixedSizeSamples = TRUE;
	mt.bTemporalCompression = FALSE;
	mt.formattype = FORMAT_WaveFormatEx;	
	memcpy_s(mt.pbFormat, sizeof(WAVEFORMATEX), &wfxOut, sizeof(WAVEFORMATEX));

	hr = pDMO->SetOutputType(0, &mt, 0); 
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Set audio output type." << endl;
		return false;
	}

	MoFreeMediaType(&mt);

	pKinectAudioStream = new KinectAudioStream(pDMO, pNuiAudioSource);


	/*


	hr = pKinectAudioStream->StartCapture();

	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not start audio stream." << endl;
		return false;
	}

	*/

    SafeRelease(pDMO);

	cout <<"[UKinect] INFO: Audio stream has been created."<< endl;

	return true;
}


/// <summary>
/// Process recently triggered speech recognition events.
/// </summary>
void UKinect::pollAudio()
{
	if (pKinectAudioStream==NULL) return;
	audioBeamAngle = pKinectAudioStream->beamAngle;
	audioSourceAngle = pKinectAudioStream->sourceAngle;
	audioSourceConfidence = pKinectAudioStream->sourceConfidence;

}

void UKinect::audioPause(bool arg)
{
	if (pKinectAudioStream==NULL) return;
	if (speech) return;
	if (arg) 
	{
		hr = pKinectAudioStream->StopCapture();
	}else{
		hr = pKinectAudioStream->StartCapture();
	}
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not start/stop audio stream." << endl;
	} 
}

bool UKinect::audioRecordStart(const std::string& fileName)
{
	WAVEFORMATEX wfxOut = {AudioFormat, AudioChannels, AudioSamplesPerSecond, AudioAverageBytesPerSecond, AudioBlockAlign, AudioBitsPerSample, 0};
	
	if (pKinectAudioStream==NULL) return false;
	if (recording) return false;
	recording = true;
	return pKinectAudioStream->RecordStart(fileName, &wfxOut);
}

bool UKinect::audioRecordStop()
{
	if (pKinectAudioStream==NULL) return false;
	if (!recording) return false;
	recording = false;
	return pKinectAudioStream->RecordStop();
}


void UKinect::setAudioEchoCancellation()
{
	if (pKinectAudioStream==NULL) return;
	PROPVARIANT pvSysMode;
	PropVariantInit(&pvSysMode);
	pvSysMode.vt = VT_I4;
	//   SINGLE_CHANNEL_AEC = 0
	//   OPTIBEAM_ARRAY_ONLY = 2
	//   OPTIBEAM_ARRAY_AND_AEC = 4
	//   SINGLE_CHANNEL = 5
	pvSysMode.lVal = (audioEchoCancellation.as<int>() ? 4 : 2);
	hr = pPropertyStore->SetValue(MFPKEY_WMAAECMA_SYSTEM_MODE, pvSysMode);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not set audio echo cancellation (AEC) setting." << endl;
	} 
	PropVariantClear(&pvSysMode);
}

void UKinect::setAudioEchoSuppresion()
{
	if (pKinectAudioStream==NULL) return;
	PROPVARIANT pvSysMode;
	PropVariantInit(&pvSysMode);
	pvSysMode.vt = VT_I4;
	pvSysMode.lVal = audioEchoSuppresion.as<int>();
	hr = pPropertyStore->SetValue(MFPKEY_WMAAECMA_FEATR_AES, pvSysMode);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not set audio echo suppresion (AES) setting." << endl;
	}
	PropVariantClear(&pvSysMode);
}

void UKinect::setAudioNoiseSuppresion()
{
	if (pKinectAudioStream==NULL) return;
	PROPVARIANT pvSysMode;
	PropVariantInit(&pvSysMode);
	pvSysMode.vt = VT_I4;
	pvSysMode.lVal = (audioNoiseSuppresion.as<int>() ? 1 : 0);
	hr = pPropertyStore->SetValue(MFPKEY_WMAAECMA_FEATR_NS, pvSysMode);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not set audio noise suppresion (ANS) setting." << endl;
	}
	PropVariantClear(&pvSysMode);
}

void UKinect::setAudioAutomaticGainControl()
{
	if (pKinectAudioStream==NULL) return;
	PROPVARIANT pvSysMode;
	PropVariantInit(&pvSysMode);
	pvSysMode.vt = VT_BOOL;
	pvSysMode.lVal = (audioAutomaticGainControl.as<int>() ? VARIANT_TRUE : VARIANT_FALSE);
	hr = pPropertyStore->SetValue(MFPKEY_WMAAECMA_FEATR_AGC, pvSysMode);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not set automatic gain control (AGC) setting." << endl;
	}
	PropVariantClear(&pvSysMode);
}


void UKinect::getFEATURE_MODE(PROPERTYKEY in)
{
	if (pKinectAudioStream==NULL) return;
	PROPVARIANT pvSysMode;
	PropVariantInit(&pvSysMode);
	hr = pPropertyStore->GetValue(in, &pvSysMode);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not read." << endl;
	}	
	int tmp = (int)pvSysMode.lVal;
	cerr <<"[UKinect] PROPERTYKEY = " << tmp << endl;
	PropVariantClear(&pvSysMode);
}