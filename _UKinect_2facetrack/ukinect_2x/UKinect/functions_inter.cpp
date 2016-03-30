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
bool UKinect::setInteractionStream()
{

	hr = NuiCreateInteractionStream(sensor,(INuiInteractionClient *)&nuiIClient,&interactionStream);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not initialize interaction stream." << endl;
		return false;
	}

	hr = interactionStream->Enable(interactionNextFrame);
	if (FAILED(hr))
	{
		cerr <<"[UKinect] ERROR: Can not enable interaction stream." << endl;
		return false;
	}

	cout <<"[UKinect] INFO: Interaction stream has been created." << endl;
	return true;
}

bool UKinect::pollInteraction()
{
	NUI_USER_INFO user;
	NUI_INTERACTION_FRAME Interaction_Frame;
	hr = interactionStream->GetNextFrame( 0, &Interaction_Frame );
	if (FAILED(hr))
	{
		cerr << hex << hr <<endl;
		cerr <<"[UKinect] WARNING: Interaction pool." << endl;
		return false;
	}

	for (int i = 0 ; i < NUI_SKELETON_COUNT; ++i)
	{
		user = Interaction_Frame.UserInfos[i];
		if (user.SkeletonTrackingId != 0) break;
	}

	NUI_HANDPOINTER_INFO handLeft = user.HandPointerInfos[0];
	NUI_HANDPOINTER_INFO handRight = user.HandPointerInfos[1];
	NUI_HANDPOINTER_STATE stateLeft  = (NUI_HANDPOINTER_STATE)handLeft.State;
	NUI_HANDPOINTER_STATE stateRight  = (NUI_HANDPOINTER_STATE)handRight.State;

	interID=user.SkeletonTrackingId;

	interLeftTracked= (bool)(stateLeft & NUI_HANDPOINTER_STATE_TRACKED);
	interLeftActive= (bool)(stateLeft & NUI_HANDPOINTER_STATE_PRESSED);
	interLeftInteractive= (bool)(stateLeft & NUI_HANDPOINTER_STATE_ACTIVE); 
	interLeftPressed= (bool)(stateLeft & NUI_HANDPOINTER_STATE_INTERACTIVE);

	interRightTracked=(bool)(stateRight & NUI_HANDPOINTER_STATE_TRACKED);
	interRightActive=(bool)(stateRight & NUI_HANDPOINTER_STATE_PRESSED);
	interRightInteractive=(bool)(stateRight & NUI_HANDPOINTER_STATE_ACTIVE);
	interRightPressed= (bool)(stateRight & NUI_HANDPOINTER_STATE_INTERACTIVE); 

	if (stateLeft!= NUI_HANDPOINTER_STATE_NOT_TRACKED)
	{
		interLeftX = handLeft.X;
		interLeftY = -handLeft.Y;
		interLeftRawX = handLeft.RawX;
		interLeftRawY = -handLeft.RawY;
		interLeftRawZ = handLeft.RawZ;
		interLeftPress = handLeft.PressExtent;
		if (handLeft.HandEventType>0) interLeftEvent = handLeft.HandEventType;
		//interLeftEvent = handLeft.HandEventType;
	}

	if (stateLeft!= NUI_HANDPOINTER_STATE_NOT_TRACKED)
	{
		interRightX = handRight.X;
		interRightY = -handRight.Y;
		interRightRawX = handRight.RawX;
		interRightRawY = -handRight.RawY;
		interRightRawZ = handRight.RawZ;
		interRightPress = handRight.PressExtent;
		if (handRight.HandEventType>0) interRightEvent = handRight.HandEventType;
		//interRightEvent = handRight.HandEventType;
	}

	if (interVisualization) {

		DWORD t_width, t_height;
		NuiImageResolutionToSize((NUI_IMAGE_RESOLUTION)colorResolution.as<int>(), t_width, t_height);

		if 
			((color)&&(interVisualizationOnColor.as<int>())) interCVMat=colorCVMat.clone(); // use color image as a background if color function enabled
		else
			interCVMat=Mat(Size(static_cast<int>(t_width), static_cast<int>(t_height)), CV_8UC3, CV_RGB( 0, 0, 0 ));

		if (stateLeft!= NUI_HANDPOINTER_STATE_NOT_TRACKED)
		{
			drawHand(user.SkeletonTrackingId,11,interRightEvent.as<int>(),interRightPressed.as<bool>(), interRightInteractive.as<bool>(), interRightPress.as<double>());
		}

		if (stateLeft!= NUI_HANDPOINTER_STATE_NOT_TRACKED)
		{
			drawHand(user.SkeletonTrackingId,7,interLeftEvent.as<int>(),interLeftPressed.as<bool>(),interLeftInteractive.as<bool>(),interLeftPress.as<double>());
		}

		interBin.image.width = interCVMat.cols;
		interBin.image.height = interCVMat.rows;
		interBin.image.size = interCVMat.cols * skeletonCVMat.rows * 3;
		interBin.image.data = interCVMat.data;
		interImage=interBin;

	}

	return true;
}


void UKinect::drawHand(DWORD ID, int joint, int _catch, bool pressed, bool active, double press)
{
	LONG x, y, xd, yd;
	USHORT depth;
	Vector4 vPoint = {0,0,0,0};

	vector<double> tmp = skeletonJointPosition(ID, joint);
	if (tmp.size()==0) return;

	vPoint.x = tmp[0];
	vPoint.y = tmp[1];
	vPoint.z = tmp[2];

	NuiTransformSkeletonToDepthImage(vPoint, &xd, &yd, &depth, (NUI_IMAGE_RESOLUTION)depthResolution.as<int>());
	hr = NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution((NUI_IMAGE_RESOLUTION)colorResolution.as<int>(), (NUI_IMAGE_RESOLUTION)depthResolution.as<int>(), NULL, xd, yd, depth, &x, &y);
	if (FAILED(hr)) return;

	if (press>2) press=2; 
	press=press*125;
	
	circle(interCVMat,Point(static_cast<int>(x),static_cast<int>(y)),10,CV_RGB( static_cast<int>(press),static_cast<int>(press),static_cast<int>(press)),-1);

	if (1 == _catch)
	{
		if (pressed){
			circle(interCVMat,Point(static_cast<int>(x),static_cast<int>(y)),15,CV_RGB( 0, 0, 255),-1);
		} else if (active) {
			circle(interCVMat,Point(static_cast<int>(x),static_cast<int>(y)),15,CV_RGB( 0, 255 ,0),-1);
		} else {
			circle(interCVMat,Point(static_cast<int>(x),static_cast<int>(y)),15,CV_RGB( 255, 0, 0),-1);
		}
	}
	else
	{
		if (pressed){
			circle(interCVMat,Point(static_cast<int>(x),static_cast<int>(y)),15,CV_RGB( 0, 0, 255),3);
		} else if (active) {
			circle(interCVMat,Point(static_cast<int>(x),static_cast<int>(y)),15,CV_RGB( 0, 255, 0),3);
		} else {
			circle(interCVMat,Point(static_cast<int>(x),static_cast<int>(y)),15,CV_RGB( 255, 0, 0),3);
		}
	}
}