/*******************************************
*
*	UKinect 
*   Author: Jan Kedzierski
*
********************************************/

#include "UKinect.h"


bool UKinect::setDepthStream()
{
	
	DWORD t_width, t_height;
	NuiImageResolutionToSize((NUI_IMAGE_RESOLUTION)depthResolution.as<int>(), t_width, t_height);
	depthCVMat=Mat(Size(static_cast<int>(t_width), static_cast<int>(t_height)), CV_8UC3);
	depthWidth=static_cast<UINT>(t_width);
	depthHeight=static_cast<UINT>(t_height);
	depthBin.image.width = static_cast<UINT>(t_width);
	depthBin.image.height = static_cast<UINT>(t_height);
	depthBin.image.size = depthBin.image.width * depthBin.image.height * 3;


	depthBuffer = FTCreateImage();
	if (!depthBuffer)
	{
		cerr <<"[UKinect] ERROR: Can not create depth buffer." << endl;
		return false;
	}
		

	hr = depthBuffer->Allocate(static_cast<UINT>(t_width), static_cast<UINT>(t_height), FTIMAGEFORMAT_UINT16_D13P3);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not alocate depth buffer." << endl;
		return false;
	}


	DWORD flags =(depthNearMode.as<int>() ? NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE : 0);

	// Open a depth stream to receive color frames
    hr = sensor->NuiImageStreamOpen(
            NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX,
            (NUI_IMAGE_RESOLUTION)depthResolution.as<int>(),
            flags,
            2,
            depthNextFrame,
            &depthStream);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not initialize depth stream." << endl;
		return false;
	}


	cout <<"[UKinect] INFO: Depth stream has been created with resolution "<<t_width<<"x"<<t_height<<"." << endl;
	
	///////////////////////////
	// Update face stream in function enabled
	//
	if (face) {
		hr = setFaceStream();
		if (!hr) cerr <<"[UKinect] WARNING: Can not set face tracker stream." << endl;
	}

	return true;
}


bool UKinect::pollDepth()
{
	if (NULL != sensor)
    { 
		NUI_IMAGE_FRAME depthFrame = {0};
		// Attempt to get the depth frame
		hr = sensor->NuiImageStreamGetNextFrame(depthStream, 0, &depthFrame);
		if (FAILED(hr))
		{
			cerr <<"[UKinect] WARNING: Depth pool." << endl;
			return false;
		}

		INuiFrameTexture * pTexture = depthFrame.pFrameTexture;

		NUI_LOCKED_RECT LockedRect;

		// Lock the frame data so the Kinect knows not to modify it while we're reading it
		pTexture->LockRect(0, &LockedRect, NULL, 0);

		// Make sure we've received valid data
		if (LockedRect.Pitch != 0)
		{
			// Copy depth image to buffer if face function enabled
			if (face) memcpy(depthBuffer->GetBuffer(), PBYTE(LockedRect.pBits), min(depthBuffer->GetBufferSize(), UINT(pTexture->BufferLen())));
			//
			// process depth image if interaction function enabled
			if (interaction){
				INuiFrameTexture* pDepthImagePixelFrame;
				BOOL nearMode = depthNearMode.as<bool>();
				sensor->NuiImageFrameGetDepthImagePixelFrameTexture(depthStream, &depthFrame, &nearMode, &pDepthImagePixelFrame);
				INuiFrameTexture *pdTexture = pDepthImagePixelFrame;
				NUI_LOCKED_RECT dLockedRect;
				// Lock the frame data so the Kinect knows not to modify it while we're reading it
				pdTexture->LockRect(0, &dLockedRect, NULL, 0);
				hr = interactionStream->ProcessDepth(dLockedRect.size,PBYTE(dLockedRect.pBits),depthFrame.liTimeStamp);
				if( FAILED( hr ) )
				{
					cerr <<"[UKinect] ERROR: Process depth failed (for interaction purpose)." << endl;
					return false;
				}
			
				// We're done with the texture so unlock it
				pdTexture->UnlockRect(0);
			}

			if (depthVisualization){
				DWORD t_width, t_height;
				NuiImageResolutionToSize((NUI_IMAGE_RESOLUTION)depthResolution.as<int>(), t_width, t_height);

				USHORT * pBuff = (USHORT*) LockedRect.pBits; 
				BYTE * buf = new BYTE[(t_width * t_height)*3];

				for(int i=0; i< (t_width * t_height); i++)  
				{  
					BYTE index = pBuff[i]&0x07;  
					USHORT realDepth = (pBuff[i]&0xFFF8)>>3;  
					BYTE scale = 255 - (BYTE)(256*realDepth/0x0fff);  
					buf[3*i] = buf[3*i+1] = buf[3*i+2] = 0;  
					switch( index )  
					{  
					case 0:  
						buf[3*i]=scale/2;  
						buf[3*i+1]=scale/2;  
						buf[3*i+2]=scale/2;  
						break;  
					case 1:  
						buf[3*i]=scale;  
						break;  
					case 2:  
						buf[3*i+1]=scale;  
						break;  
					case 3:  
						buf[3*i+2]=scale;  
						break;  
					case 4:  
						buf[3*i]=scale;  
						buf[3*i+1]=scale;  
						break;  
					case 5:  
						buf[3*i]=scale;  
						buf[3*i+2]=scale;  
						break;  
					case 6:  
						buf[3*i+1]=scale;  
						buf[3*i+2]=scale;  
						break;  
					case 7:  
						buf[3*i]=255-scale/2;  
						buf[3*i+1]=255-scale/2;  
						buf[3*i+2]=255-scale/2;  
						break;  
					}  
				}  
				Mat tmp = Mat(Size(static_cast<int>(t_width), static_cast<int>(t_height)), CV_8UC3, buf);
			
				depthCVMat = tmp.clone();
								
				// Save CV image to UImage
				depthBin.image.data = depthCVMat.data;
				depthImage=depthBin;

				delete [] buf;
			
				// We're done with the texture so unlock it
				pTexture->UnlockRect(0);
			}

		}

		// Release the frame
		sensor->NuiImageStreamReleaseFrame(depthStream, &depthFrame);


		return true;

	} 
	cerr <<"[UKinect] ERROR: Depth pool error." << endl;
	return false;
}


void UKinect::setNearMode()
{

	if (NULL == sensor) {
		cerr <<"[UKinect] ERROR: Sensor is not initialized." << endl;
		return;
	}

    if (INVALID_HANDLE_VALUE != depthStream)
    {
		hr = sensor->NuiImageStreamSetImageFrameFlags(depthStream, (depthNearMode.as<int>() ? NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE : 0));
		if (FAILED(hr)){
			cerr <<"[UKinect] ERROR: Can not set near mode or using not supported Kinect XBOX360 device." << endl;		
		}
		return;
	}
	cerr <<"[UKinect] ERROR: Can not set near mode." << endl;
}


void UKinect::setEmitterOff()
{

	if (NULL == sensor) {
		cerr <<"[UKinect] ERROR: Sensor is not initialized." << endl;
		return;
	}

	if (INVALID_HANDLE_VALUE != depthStream)
    {
		hr = sensor->NuiSetForceInfraredEmitterOff(depthEmitterOff.as<int>() ? TRUE : FALSE);
		if (SUCCEEDED(hr)) return;
	}
    cerr <<"[UKinect] ERROR: Can not set emitter off." << endl;
}

