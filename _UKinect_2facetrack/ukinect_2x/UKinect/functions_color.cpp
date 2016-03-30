/*******************************************
*
*	UKinect 
*   Author: Jan Kedzierski
*
********************************************/

#include "UKinect.h"



bool UKinect::setColorStream()
{



	DWORD t_width, t_height;
	NuiImageResolutionToSize((NUI_IMAGE_RESOLUTION)colorResolution.as<int>(), t_width, t_height);
	colorCVMat=Mat(Size(static_cast<int>(t_width), static_cast<int>(t_height)), CV_8UC3);
	colorWidth=colorCVMat.cols;
	colorHeight=colorCVMat.rows;
	colorBin.image.width = colorCVMat.cols;
	colorBin.image.height = colorCVMat.rows;
	colorBin.image.size = colorCVMat.cols * colorCVMat.rows * 3; 


	colorBuffer = FTCreateImage();
	if (!colorBuffer)
	{
		cerr <<"[UKinect] ERROR: Can not create color buffer." << endl;
		return false;
	}


	hr = colorBuffer->Allocate(static_cast<UINT>(t_width), static_cast<UINT>(t_height), FTIMAGEFORMAT_UINT8_B8G8R8X8);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not alocate depth buffer." << endl;
		return false;
	}


	// Open a color image stream to receive color frames
	hr = sensor->NuiImageStreamOpen(
		NUI_IMAGE_TYPE_COLOR,
		(NUI_IMAGE_RESOLUTION)colorResolution.as<int>(),
		0,
		2,
		colorNextFrame,
		&colorStream);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not initialize color stream." << endl;
		return false;
	}

	cout <<"[UKinect] INFO: Image stream has been created with resolution "<<t_width<<"x"<<t_height<<"." << endl;


	///////////////////////////
	// Update face stream in function enabled
	//
	if (face) {
		hr = setFaceStream();
		if (!hr)cerr <<"[UKinect] ERROR: Can not set face tracker stream." << endl;
	}

	return true;

}

bool UKinect::pollColor()
{
	if (NULL != sensor)
	{ 
		NUI_IMAGE_FRAME imageFrame;
		// Attempt to get the color frame
		hr = sensor->NuiImageStreamGetNextFrame(colorStream, 0, &imageFrame);
		if (FAILED(hr))
		{
			cerr <<"[UKinect] WARNING: Color pool." << endl;
			return false;
		}

		INuiFrameTexture * pTexture = imageFrame.pFrameTexture;
		NUI_LOCKED_RECT LockedRect;

		// Lock the frame data so the Kinect knows not to modify it while we're reading it
		pTexture->LockRect(0, &LockedRect, NULL, 0);

		// Make sure we've received valid data
		if (LockedRect.Pitch != 0)
		{
			// Copy color image to buffer if face function enabled
			if (face) memcpy(colorBuffer->GetBuffer(), PBYTE(LockedRect.pBits), min(colorBuffer->GetBufferSize(), UINT(pTexture->BufferLen())));

			// Copy colorImage 				
			Mat tmp=Mat(Size(colorWidth.as<int>(), colorHeight.as<int>()), CV_8UC4);
			tmp.data = LockedRect.pBits;
			cvtColor(tmp, colorCVMat, CV_BGRA2RGB); // <- alloc new memory	

			// Save CV image to UImage
			colorBin.image.data = colorCVMat.data;
			colorImage=colorBin;

		}

		// We're done with the texture so unlock it
		pTexture->UnlockRect(0);


		// Release the frame
		sensor->NuiImageStreamReleaseFrame(colorStream, &imageFrame);

		return true;

	} 
	cerr <<"[UKinect] ERROR: color pool error." << endl;
	return false;
}


void UKinect::colorResetSettings()
{
	if (NULL == sensor) return;

	INuiColorCameraSettings* cameraSettings;
	// The device support camera settings
	if (S_OK == sensor->NuiGetColorCameraSettings(&cameraSettings))
	{
		cameraSettings->ResetCameraSettingsToDefault();
		int itmp;
		double dtmp;

		cameraSettings->GetAutoExposure(&itmp);
		colorAutoExposure=itmp;
		colorAutoExposure.rangemax=1;
		colorAutoExposure.rangemin=0;

		cameraSettings->GetBrightness(&dtmp);
		colorBrightness=dtmp;
		cameraSettings->GetMaxBrightness(&dtmp);
		colorBrightness.rangemax=dtmp;
		cameraSettings->GetMinBrightness(&dtmp);
		colorBrightness.rangemin=dtmp;

		cameraSettings->GetExposureTime(&dtmp);
		colorExposureTime=dtmp;
		cameraSettings->GetMaxExposureTime(&dtmp);
		colorExposureTime.rangemax=dtmp;
		cameraSettings->GetMinExposureTime(&dtmp);
		colorExposureTime.rangemin=dtmp;

		cameraSettings->GetGain(&dtmp);
		colorGain=dtmp;
		cameraSettings->GetMaxGain(&dtmp);
		colorGain.rangemax=dtmp;
		cameraSettings->GetMinGain(&dtmp);
		colorGain.rangemin=dtmp;

		NUI_POWER_LINE_FREQUENCY ntmp1;
		cameraSettings->GetPowerLineFrequency(&ntmp1);
		colorPowerLineFrequency=itmp;
		colorPowerLineFrequency.rangemax=2;
		colorPowerLineFrequency.rangemin=0;

		NUI_BACKLIGHT_COMPENSATION_MODE ntmp2;
		cameraSettings->GetBacklightCompensationMode(&ntmp2);
		colorBacklightCompensationMode=itmp;
		colorBacklightCompensationMode.rangemax=4;
		colorBacklightCompensationMode.rangemin=0;

		cameraSettings->GetAutoWhiteBalance(&itmp);
		colorAutoWhiteBalance=itmp;
		colorAutoWhiteBalance.rangemax=1;
		colorAutoWhiteBalance.rangemin=0;

		LONG ltmp;
		cameraSettings->GetWhiteBalance(&ltmp);
		colorWhiteBalance=ltmp;
		cameraSettings->GetMaxWhiteBalance(&ltmp);
		colorWhiteBalance.rangemax=ltmp;
		cameraSettings->GetMinWhiteBalance(&ltmp);
		colorWhiteBalance.rangemin=ltmp;

		cameraSettings->GetContrast(&dtmp);
		colorContrast=dtmp;
		cameraSettings->GetMaxContrast(&dtmp);
		colorContrast.rangemax=dtmp;
		cameraSettings->GetMinContrast(&dtmp);
		colorContrast.rangemin=dtmp;

		cameraSettings->GetHue(&dtmp);
		colorHue=dtmp;
		cameraSettings->GetMaxHue(&dtmp);
		colorHue.rangemax=dtmp;
		cameraSettings->GetMinHue(&dtmp);
		colorHue.rangemin=dtmp;

		cameraSettings->GetSaturation(&dtmp);
		colorSaturation=dtmp;
		cameraSettings->GetMaxSaturation(&dtmp);
		colorSaturation.rangemax=dtmp;
		cameraSettings->GetMinSaturation(&dtmp);
		colorSaturation.rangemin=dtmp;

		cameraSettings->GetGamma(&dtmp);
		colorGamma=dtmp;
		cameraSettings->GetMaxGamma(&dtmp);
		colorGamma.rangemax=dtmp;
		cameraSettings->GetMinGamma(&dtmp);
		colorGamma.rangemin=dtmp;

		cameraSettings->GetSharpness(&dtmp);
		colorSharpness=dtmp;
		cameraSettings->GetMaxSharpness(&dtmp);
		colorSharpness.rangemax=dtmp;
		cameraSettings->GetMinSharpness(&dtmp);
		colorSharpness.rangemin=dtmp;

		cameraSettings->Release();

	} else cerr <<"[UKinect] ERROR: Can not read camera settings or using not supported Kinect XBOX360 device." << endl;
}

void UKinect::setAutoExposure()
{
	if (NULL == sensor) {
		cerr <<"[UKinect] ERROR: Sensor is not initialized." << endl;
		return;
	}

	INuiColorCameraSettings* cameraSettings;
	// The device support camera settings
	if (S_OK == sensor->NuiGetColorCameraSettings(&cameraSettings))
	{
		cameraSettings->SetAutoExposure(colorAutoExposure.as<int>());
		cameraSettings->Release();
	} else cerr <<"[UKinect] ERROR: Can not read camera settings or using not supported Kinect XBOX360 device." << endl;
}


void UKinect::setBrightness()
{
	if (NULL == sensor) {
		cerr <<"[UKinect] ERROR: Sensor is not initialized." << endl;
		return;
	}

	INuiColorCameraSettings* cameraSettings;
	// The device support camera settings
	if (S_OK == sensor->NuiGetColorCameraSettings(&cameraSettings))
	{
		cameraSettings->SetBrightness(colorBrightness.as<double>());
		cameraSettings->Release();
	} else cerr <<"[UKinect] ERROR: Can not read camera settings or using not supported Kinect XBOX360 device." << endl;
}

void UKinect::setExposureTime()
{
	if (NULL == sensor) {
		cerr <<"[UKinect] ERROR: Sensor is not initialized." << endl;
		return;
	}

	INuiColorCameraSettings* cameraSettings;
	// The device support camera settings
	if (S_OK == sensor->NuiGetColorCameraSettings(&cameraSettings))
	{
		cameraSettings->SetExposureTime(colorExposureTime.as<double>());
		cameraSettings->Release();
	} else cerr <<"[UKinect] ERROR: Can not read camera settings or using not supported Kinect XBOX360 device." << endl;
}


void UKinect::setGain()
{
	if (NULL == sensor) {
		cerr <<"[UKinect] ERROR: Sensor is not initialized." << endl;
		return;
	}

	INuiColorCameraSettings* cameraSettings;
	// The device support camera settings
	if (S_OK == sensor->NuiGetColorCameraSettings(&cameraSettings))
	{
		cameraSettings->SetGain(colorGain.as<double>());
		cameraSettings->Release();
	} else cerr <<"[UKinect] ERROR: Can not read camera settings or using not supported Kinect XBOX360 device." << endl;
}


void UKinect::setPowerLineFrequency()
{
	if (NULL == sensor) {
		cerr <<"[UKinect] ERROR: Sensor is not initialized." << endl;
		return;
	}

	INuiColorCameraSettings* cameraSettings;
	// The device support camera settings
	if (S_OK == sensor->NuiGetColorCameraSettings(&cameraSettings))
	{
		cameraSettings->SetPowerLineFrequency((NUI_POWER_LINE_FREQUENCY)colorPowerLineFrequency.as<int>());
		cameraSettings->Release();
	} else cerr <<"[UKinect] ERROR: Can not read camera settings or using not supported Kinect XBOX360 device." << endl;

}

void UKinect::setBacklightCompensationMode()
{
	if (NULL == sensor) {
		cerr <<"[UKinect] ERROR: Sensor is not initialized." << endl;
		return;
	}

	INuiColorCameraSettings* cameraSettings;
	// The device support camera settings
	if (S_OK == sensor->NuiGetColorCameraSettings(&cameraSettings))
	{
		cameraSettings->SetBacklightCompensationMode((NUI_BACKLIGHT_COMPENSATION_MODE)colorBacklightCompensationMode.as<int>());
		cameraSettings->Release();
	} else cerr <<"[UKinect] ERROR: Can not read camera settings or using not supported Kinect XBOX360 device." << endl;
}

void UKinect::setAutoWhiteBalance()
{
	if (NULL == sensor) {
		cerr <<"[UKinect] ERROR: Sensor is not initialized." << endl;
		return;
	}

	INuiColorCameraSettings* cameraSettings;
	// The device support camera settings
	if (S_OK == sensor->NuiGetColorCameraSettings(&cameraSettings))
	{
		cameraSettings->SetAutoWhiteBalance(colorAutoWhiteBalance.as<int>());
		cameraSettings->Release();
	} else cerr <<"[UKinect] ERROR: Can not read camera settings or using not supported Kinect XBOX360 device." << endl;
}

void UKinect::setWhiteBalance()
{
	if (NULL == sensor) {
		cerr <<"[UKinect] ERROR: Sensor is not initialized." << endl;
		return;
	}

	INuiColorCameraSettings* cameraSettings;
	// The device support camera settings
	if (S_OK == sensor->NuiGetColorCameraSettings(&cameraSettings))
	{
		cameraSettings->SetWhiteBalance(colorWhiteBalance.as<long>());
		cameraSettings->Release();
	} else cerr <<"[UKinect] ERROR: Can not read camera settings or using not supported Kinect XBOX360 device." << endl;
}


void UKinect::setContrast()
{
	if (NULL == sensor) {
		cerr <<"[UKinect] ERROR: Sensor is not initialized." << endl;
		return;
	}

	INuiColorCameraSettings* cameraSettings;
	// The device support camera settings
	if (S_OK == sensor->NuiGetColorCameraSettings(&cameraSettings))
	{
		cameraSettings->SetContrast(colorContrast.as<double>());
		cameraSettings->Release();
	} else cerr <<"[UKinect] ERROR: Can not read camera settings or using not supported Kinect XBOX360 device." << endl;
}

void UKinect::setHue()
{
	if (NULL == sensor) {
		cerr <<"[UKinect] ERROR: Sensor is not initialized." << endl;
		return;
	}

	INuiColorCameraSettings* cameraSettings;
	// The device support camera settings
	if (S_OK == sensor->NuiGetColorCameraSettings(&cameraSettings))
	{
		cameraSettings->SetHue(colorHue.as<double>());
		cameraSettings->Release();
	} else cerr <<"[UKinect] ERROR: Can not read camera settings or using not supported Kinect XBOX360 device." << endl;
}

void UKinect::setSaturation()
{
	if (NULL == sensor) {
		cerr <<"[UKinect] ERROR: Sensor is not initialized." << endl;
		return;
	}

	INuiColorCameraSettings* cameraSettings;
	// The device support camera settings
	if (S_OK == sensor->NuiGetColorCameraSettings(&cameraSettings))
	{
		cameraSettings->SetSaturation(colorSaturation.as<double>());
		cameraSettings->Release();
	} else cerr <<"[UKinect] ERROR: Can not read camera settings or using not supported Kinect XBOX360 device." << endl;
}

void UKinect::setGamma()
{
	if (NULL == sensor) {
		cerr <<"[UKinect] ERROR: Sensor is not initialized." << endl;
		return;
	}

	INuiColorCameraSettings* cameraSettings;
	// The device support camera settings
	if (S_OK == sensor->NuiGetColorCameraSettings(&cameraSettings))
	{
		cameraSettings->SetGamma(colorGamma.as<double>());
		cameraSettings->Release();
	} else cerr <<"[UKinect] ERROR: Can not read camera settings or using not supported Kinect XBOX360 device." << endl;
}

void UKinect::setSharpness()
{
	if (NULL == sensor) {
		cerr <<"[UKinect] ERROR: Sensor is not initialized." << endl;
		return;
	}

	INuiColorCameraSettings* cameraSettings;
	// The device support camera settings
	if (S_OK == sensor->NuiGetColorCameraSettings(&cameraSettings))
	{
		cameraSettings->SetSharpness(colorSharpness.as<double>());
		cameraSettings->Release();
	} else cerr <<"[UKinect] ERROR: Can not read camera settings or using not supported Kinect XBOX360 device." << endl;
}


