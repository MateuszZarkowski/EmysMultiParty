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
vector<double> UKinect::accelerometer()
{
    // Get the reading
	vector<double> acc;
    Vector4 reading;

	if (NULL != sensor) 
	{
		HRESULT hr = sensor->NuiAccelerometerGetCurrentReading(&reading);

		if (SUCCEEDED(hr))
		{
			// Set the reading to viewer
			acc.push_back(static_cast<double>(reading.x));
			acc.push_back(static_cast<double>(reading.y));
			acc.push_back(static_cast<double>(reading.z));
			return acc;
		} else  cerr <<"[UKinect] ERROR: Can not read accelerometer." << endl;
	} else  cerr <<"[UKinect] ERROR: Sensor is not initialized." << endl;
	return acc;
}