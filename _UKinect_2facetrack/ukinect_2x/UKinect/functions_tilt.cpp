/*******************************************
*
*	UKinect 
*   Author: Jan Kedzierski
*
********************************************/

#include "UKinect.h"


/// <summary>
/// Get accelerometer reading
/// </summary>
void UKinect::setTilt()
{
	if (NULL != sensor) 
	{
		sensor->NuiCameraElevationSetAngle(static_cast<LONG>(tilt.as<int>()));
		if (FAILED(hr)) cerr <<"[UKinect] ERROR: Can not set tilt angle." << endl;
	} else  cerr <<"[UKinect] ERROR: Sensor is not initialized." << endl;
}

