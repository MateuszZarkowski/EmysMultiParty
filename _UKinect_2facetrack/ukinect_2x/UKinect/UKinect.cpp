// UKinect is a module for Urbi to handle Microsoft Kinect sensor.
// Copyright (C) 2013 Jan Kedzierski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

/*******************************************
*
*	U K i n e c t
*
*   author: Jan Kedzierski
*   date: 02.05.2015
*    ver: 2.71

hardware requirements:
- Kinect XBOX 360 or Kinect for Windows(recomended)

software requirements:
- OpenCV (tested with 2.3.1)
- Microsoft Kinect SDK (tested with 1.8)
- Microsoft Speech Platform SDK (tested with 11.0)
- Microsoft Windows SDK (tested with 6.0A)

to run use dynamic libraries (set in PATH env.):
FaceTrackData.dll	 (x86 ver.)
FaceTrackLib.dll	 (x86 ver.)
Microsoft.Kinect.dll (x86 ver.)
Microsoft.Speech.dll (x86 ver.)
KinectInteraction180_32.dll


usage:
loadModule("UKinect");
var Global.kinect= UKinect.new();
kinect.Open(0,true,true,true,true,true,true,true);
kinect.speechLoadGrammar("speech.grxml");
t: loop {
kinect.PoolData();
},
********************************************/

#include "UKinect.h"


UKinect::UKinect(const std::string& s) : urbi::UObject(s) {
  UBindFunction(UKinect, init);
};

UKinect::~UKinect() {

  if (pKinectAudioStream != NULL) {
    pKinectAudioStream->RecordStop();
    pKinectAudioStream->StopCapture();
  }

  sensor->NuiShutdown();
  SafeRelease(sensor);

  if (depthNextFrame && (depthNextFrame != INVALID_HANDLE_VALUE)) CloseHandle(depthNextFrame);
  if (colorNextFrame && (colorNextFrame != INVALID_HANDLE_VALUE)) CloseHandle(colorNextFrame);
  if (skeletonNextFrame && (skeletonNextFrame != INVALID_HANDLE_VALUE)) CloseHandle(skeletonNextFrame);
  if (interactionNextFrame && (interactionNextFrame != INVALID_HANDLE_VALUE)) CloseHandle(interactionNextFrame);

  // Clear activity watcher
  for (map<int, NuiActivityWatcher*>::iterator itr = activityWatchers.begin(); itr != activityWatchers.end(); ++itr) {
    delete itr->second;
  }
  activityWatchers.clear();


  SafeRelease(pFTResult[0]);
  SafeRelease(pFTResult[1]);
  SafeRelease(pFTResult_copy[0]);
  SafeRelease(pFTResult_copy[1]);
  SafeRelease(colorBuffer);
  SafeRelease(depthBuffer);
  SafeRelease(pFaceTracker[0]);
  SafeRelease(pFaceTracker[1]);

  SafeRelease(speechStream);
  SafeRelease(pSpeechRecognizer);
  SafeRelease(pSpeechContext);
  SafeRelease(pSpeechGrammar);
  SafeRelease(interactionStream);

  SafeRelease(pKinectAudioStream);
  SafeRelease(pNuiAudioSource);
  SafeRelease(pPropertyStore);
  CoUninitialize();
}

int UKinect::init() {

  // Initialize the COM library on the current thread.
  CoUninitialize();
  hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

  if (FAILED(hr)) {
    CoUninitialize();
    cerr << "[UKinect] ERROR: Failed to initialize the COM library." << endl;
    return false;
  }

  //Bind all variables
  UBindVars(UKinect, fps, time);
  UBindVars(UKinect, colorEnabled, colorResolution, colorWidth, colorHeight, colorImage);
  UBindVars(UKinect, colorAutoExposure, colorBrightness, colorExposureTime, colorGain, colorPowerLineFrequency, colorBacklightCompensationMode);
  UBindVars(UKinect, colorAutoWhiteBalance, colorWhiteBalance, colorContrast, colorHue, colorSaturation, colorGamma, colorSharpness);

  UBindVars(UKinect, depthEnabled, depthResolution, depthWidth, depthHeight, depthImage, depthVisualization);
  UBindVars(UKinect, depthNearMode, depthEmitterOff);

  UBindVars(UKinect, skeletonEnabled, skeletonImage, skeletonVisualization);
  UBindVars(UKinect, skeletonVisualizationOnColor, skeletonTrackingMode, skeletonChooserMode, skeletonIDs, skeletonTrackedIDs, skeletonFilter);

  UBindVars(UKinect, faceEnabled, faceImage, faceVisualization, faceVisualizationOnColor, faceTrackingPause, faceVisualizationMode, faceIsTracking, faceEmotion);

  UBindVars(UKinect, interEnabled, interID, interImage, interVisualization, interVisualizationOnColor);
  UBindVars(UKinect, interLeftTracked, interLeftActive, interLeftInteractive, interLeftPressed, interLeftEvent, interLeftX, interLeftY, interLeftRawX, interLeftRawY, interLeftRawZ, interLeftPress);
  UBindVars(UKinect, interRightTracked, interRightActive, interRightInteractive, interRightPressed, interRightEvent, interRightX, interRightY, interRightRawX, interRightRawY, interRightRawZ, interRightPress);

  UBindVars(UKinect, audioEnabled, audioBeamAngle, audioSourceAngle, audioSourceConfidence, audioEchoCancellation, audioEchoSuppresion, audioNoiseSuppresion, audioAutomaticGainControl);

  UBindVars(UKinect, speechRecognizer, speechAvailableRecognizers, speechEnabled, speechResult, speechResultTag, speechIsListening, speechConfidence, speechConfidenceThreshold, speechPause);

  UBindVars(UKinect, tilt, debug);

  // Bind all functions
  UBindFunctions(UKinect, Open, Close, colorResetSettings);
  UBindFunctions(UKinect, skeletonPosition, skeletonJointPosition, skeletonPositionOnImage, skeletonJointPositionOnImage);
  UBindFunctions(UKinect, facePosition, facePositionOnImage, facePointsOnImage, faceAU, faceSU);
  UBindThreadedFunction(UKinect, speechLoadGrammar, LOCK_FUNCTION);
  UBindThreadedFunction(UKinect, speechResetGrammar, LOCK_FUNCTION);
  UBindFunctions(UKinect, speechAddPhrase);
  UBindFunctions(UKinect, accelerometer);
  UBindThreadedFunction(UKinect, audioRecordStop, LOCK_FUNCTION);
  UBindThreadedFunction(UKinect, audioRecordStart, LOCK_FUNCTION);
  UBindThreadedFunction(UKinect, audioPause, LOCK_FUNCTION);
  UBindThreadedFunction(UKinect, PollAudio, LOCK_FUNCTION);
  UBindThreadedFunction(UKinect, PollVideo, LOCK_FUNCTION);

  // CLear all module data
  ClearData();

  cout << "[UKinect] *************************************************************" << endl;
  cout << "[UKinect] **  _    _ _  ___                 _               _  " << endl;
  cout << "[UKinect] ** | |  | | |/ (_)               | |             (_)   " << endl;
  cout << "[UKinect] ** | |  | | ' / _ _ __   ___  ___| |_            _|_   " << endl;
  cout << "[UKinect] ** | |  | |  < | | '_ \\ / _ \\/ __| __|          / | \\  " << endl;
  cout << "[UKinect] ** | |__| | . \\| | | | |  __/ (__| |_          |  |  |" << endl;
  cout << "[UKinect] **  \\____/|_|\\_\\_|_| |_|\\___|\\___|\\__|           / \\   " << endl;
  cout << "[UKinect] **                                              |   | " << endl;
  cout << "[UKinect] **    author: Jan Kedzierski                    -   -" << endl;
  cout << "[UKinect] **   address: Wroclaw University of Technology" << endl;
  cout << "[UKinect] **   version: 2.71 " << endl;
  cout << "[UKinect] **      date: 02.05.2015" << endl;
  cout << "[UKinect] **" << endl;


  sensor = NULL;

  depthNextFrame = NULL;
  colorNextFrame = NULL;
  skeletonNextFrame = NULL;
  interactionNextFrame = NULL;


  pFTResult[0] = NULL;
  pFTResult_copy[0] = NULL;
  pFaceTracker[0] = NULL;

  pFTResult[1] = NULL;
  pFTResult_copy[1] = NULL;
  pFaceTracker[1] = NULL;

  colorBuffer = NULL;
  depthBuffer = NULL;

  speechStream = NULL;
  pSpeechRecognizer = NULL;
  pSpeechContext = NULL;
  pSpeechGrammar = NULL;
  interactionStream = NULL;

  pKinectAudioStream = NULL;
  pNuiAudioSource = NULL;
  pPropertyStore = NULL;


  return 0;
}

void UKinect::ClearData() {

  vector<int> clear_list;


  colorResolution = 2;//NUI_IMAGE_RESOLUTION_640x480;
  depthResolution = 2;//NUI_IMAGE_RESOLUTION_640x480;


  colorWidth = 0;
  colorHeight = 0;
  colorAutoExposure = 0;
  colorBrightness = 0;
  colorExposureTime = 0;
  colorGain = 0;
  colorPowerLineFrequency = 0;
  colorBacklightCompensationMode = 0;
  colorAutoWhiteBalance = 0;
  colorWhiteBalance = 0;
  colorContrast = 0;
  colorHue = 0;
  colorSaturation = 0;
  colorGamma = 0;
  colorSharpness = 0;
  //
  depthVisualization = true;
  depthWidth = 0;
  depthHeight = 0;
  depthNearMode = 0;
  depthEmitterOff = 0;
  //
  skeletonVisualization = true;
  skeletonVisualizationOnColor = 0;
  skeletonTrackingMode = 0;
  skeletonChooserMode = 0;
  skeletonIDs = clear_list;
  skeletonTrackedIDs = clear_list;
  skeletonFilter = 1;
  //
  faceVisualization = true;
  faceVisualizationOnColor = 0;
  faceTrackingPause = 0;
  faceVisualizationMode = 0;
  faceIsTracking = vector<bool>(2,false);
  faceEmotion = vector<string>(2, "");
  //
  interVisualization = true;
  interVisualizationOnColor = 0;
  interLeftTracked = 0;
  interLeftActive = 0;
  interLeftInteractive = 0;
  interLeftPressed = 0;
  interLeftEvent = 0;
  interLeftX = 0;
  interLeftY = 0;
  interLeftRawX = 0;
  interLeftRawY = 0;
  interLeftRawZ = 0;
  interLeftPress = 0;
  interRightTracked = 0;
  interRightActive = 0;
  interRightInteractive = 0;
  interRightPressed = 0;
  interRightEvent = 0;
  interRightX = 0;
  interRightY = 0;
  interRightRawX = 0;
  interRightRawY = 0;
  interRightRawZ = 0;
  interRightPress = 0;
  interID = 0;
  //
  audioBeamAngle = 0;
  audioSourceAngle = 0;
  audioSourceConfidence = 0;
  audioEchoCancellation = 0;
  audioEchoSuppresion = 0;
  audioNoiseSuppresion = 0;
  audioAutomaticGainControl = 0;
  //
  speechRecognizer = -1;
  speechAvailableRecognizers = clear_list;
  speechResult = "";
  speechResultTag = "";
  speechIsListening = false;
  speechConfidence = 0,
    speechConfidenceThreshold = 0;
  speechPause = false;
  //
  tilt = 0;
  fps = 0;

  debug = false;

  colorCVMat = Mat(Size(640, 480), CV_8UC3, CV_RGB(0, 0, 0));
  putText(colorCVMat, "no image", Point(150, 260), 4, 2, CV_RGB(0, 0, 200), 4);

  colorBin.image.width = colorCVMat.cols;
  colorBin.image.height = colorCVMat.rows;
  colorBin.image.size = colorCVMat.cols * colorCVMat.rows * 3;
  colorBin.type = BINARY_IMAGE;
  colorBin.image.imageFormat = IMAGE_RGB;
  colorBin.image.data = colorCVMat.data;
  colorImage = colorBin;

  depthCVMat = colorCVMat.clone();
  depthBin.image.width = depthCVMat.cols;
  depthBin.image.height = depthCVMat.rows;
  depthBin.image.size = depthCVMat.cols * depthCVMat.rows * 3;
  depthBin.type = BINARY_IMAGE;
  depthBin.image.imageFormat = IMAGE_RGB;
  depthBin.image.data = depthCVMat.data;
  depthImage = depthBin;

  skeletonCVMat = colorCVMat.clone();
  skeletonBin.image.width = skeletonCVMat.cols;
  skeletonBin.image.height = skeletonCVMat.rows;
  skeletonBin.image.size = skeletonCVMat.cols * skeletonCVMat.rows * 3;
  skeletonBin.type = BINARY_IMAGE;
  skeletonBin.image.imageFormat = IMAGE_RGB;
  skeletonBin.image.data = skeletonCVMat.data;
  skeletonImage = skeletonBin;

  faceCVMat = colorCVMat.clone();
  faceBin.image.width = faceCVMat.cols;
  faceBin.image.height = faceCVMat.rows;
  faceBin.image.size = faceCVMat.cols * faceCVMat.rows * 3;
  faceBin.type = BINARY_IMAGE;
  faceBin.image.imageFormat = IMAGE_RGB;
  faceBin.image.data = faceCVMat.data;
  faceImage = faceBin;

  interCVMat = colorCVMat.clone();
  interBin.image.width = interCVMat.cols;
  interBin.image.height = interCVMat.rows;
  interBin.image.size = interCVMat.cols * interCVMat.rows * 3;
  interBin.type = BINARY_IMAGE;
  interBin.image.imageFormat = IMAGE_RGB;
  interBin.image.data = interCVMat.data;
  interImage = interBin;

  stickyIDs[FirstTrackID] = 0;
  stickyIDs[SecondTrackID] = 0;

  color = false;
  depth = false;
  skeleton = false;
  audio = false;
  speech = false;
  face = false;
  interaction = false;

  colorEnabled = false;
  depthEnabled = false;
  skeletonEnabled = false;
  audioEnabled = false;
  speechEnabled = false;
  faceEnabled = false;
  interEnabled = false;


  recording = false;
}


bool UKinect::Open(int ID, bool in_color, bool in_depth, bool in_skeleton, bool in_face, bool in_interaction, bool in_audio, bool in_speech) {
  cout << "[UKinect] *************************************************************" << endl;
  cout << "[UKinect] INFO: Sensor loading..." << endl;


  int sensorCount = 0;
  hr = NuiGetSensorCount(&sensorCount);
  if (FAILED(hr)) {
    cerr << "[UKinect] ERROR: Get sensors count error." << endl;
    return false;
  }

  if (sensorCount == 0) {
    cerr << "[UKinect] ERROR: Can not find any sensors." << endl;
    return false;
  }

  cout << "[UKinect] INFO: No of connected sensors: " << sensorCount << endl;

  // Create the sensor so we can check status, if we can't create it, move on to the next
  hr = NuiCreateSensorByIndex(ID, &sensor);
  if (FAILED(hr)) {
    cerr << "[UKinect] ERROR: Sensor ID" << ID << " has error." << endl;
    return false;
  }


  // Get the status of the sensor, and if connected, then we can initialize it
  hr = sensor->NuiStatus();
  if (S_OK == hr) cout << "[UKinect] INFO: Sensor ID " << ID << " has been initialized." << endl;

  if (NULL == sensor || FAILED(hr)) {
    switch (hr) {

    case S_NUI_INITIALIZING:
      cerr << "[UKinect] ERROR: The device is connected, but still initializing." << endl;
      break;
    case E_NUI_NOTCONNECTED:
      cerr << "[UKinect] ERROR: The device is not connected." << endl;
      break;
    case E_NUI_NOTGENUINE:
      cerr << "[UKinect] ERROR: The device is not a valid Kinect." << endl;
      break;
    case E_NUI_NOTSUPPORTED:
      cerr << "[UKinect] ERROR: The device is an unsupported model." << endl;
      break;
    case E_NUI_INSUFFICIENTBANDWIDTH:
      cerr << "[UKinect] ERROR: The device is connected to a hub without the necessary bandwidth requirements." << endl;
      break;
    case E_NUI_NOTPOWERED:
      cerr << "[UKinect] ERROR: The device is connected, but unpowered." << endl;
      break;
    case E_NUI_NOTREADY:
      cerr << "[UKinect] ERROR: There was some other unspecified error." << endl;
      break;
    }
    return false;
  }

  //NOTE: initalization is needed for DEBUG!
  DWORD flags = 0;
  if (in_color || in_face) flags |= NUI_INITIALIZE_FLAG_USES_COLOR;
  if (in_depth || in_face || in_interaction) flags |= NUI_INITIALIZE_FLAG_USES_DEPTH | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX;
  if (in_skeleton || in_face || in_interaction) flags |= NUI_INITIALIZE_FLAG_USES_SKELETON;
  if (in_audio || in_speech) flags |= NUI_INITIALIZE_FLAG_USES_AUDIO;
  // Initialize the Kinect and specify that we'll be using color
  hr = sensor->NuiInitialize(flags);
  if (S_OK == hr) {
    cout << "[UKinect] INFO: Device initialized." << endl;
    if (in_color || in_face) {
      colorNextFrame = CreateEvent(NULL, TRUE, FALSE, NULL);
      if (!setColorStream()) return false;
      color = in_color;
      colorEnabled = true;
    }
    if (in_depth || in_face || in_interaction) {
      depthNextFrame = CreateEvent(NULL, TRUE, FALSE, NULL);
      if (!setDepthStream()) return false;
      depth = in_depth;
      depthEnabled = true;
    }
    if (in_skeleton || in_interaction) {
      skeletonNextFrame = CreateEvent(NULL, TRUE, FALSE, NULL);
      if (!setSkeletonStream()) return false;
      skeleton = in_skeleton;
      skeletonEnabled = true;
    }
    if (in_face) {
      if (!setFaceStream()) return false;
      face = in_face;
      color = in_face;
      depth = in_face;
      faceEnabled = true;
    }
    if (in_interaction) {
      interactionNextFrame = CreateEvent(NULL, TRUE, FALSE, NULL);
      if (!setInteractionStream()) return false;
      interaction = in_interaction;
      depth = in_interaction;
      skeleton = in_interaction;
      interEnabled = true;
    }
    if (in_audio || in_speech) {
      if (!setAudioStream()) return false;
      audio = in_audio;
      audioEnabled = true;
    }
    if (in_speech) {
      if (!setSpeechStream()) return false;
      speech = in_speech;
      audio = in_speech;
      speechEnabled = true;
    }

    if (color) {
      INuiColorCameraSettings* cameraSettings;
      // The device support camera settings
      if (S_OK == sensor->NuiGetColorCameraSettings(&cameraSettings)) {

        int itmp;
        double dtmp;

        cameraSettings->GetAutoExposure(&itmp);
        colorAutoExposure = itmp;
        colorAutoExposure.rangemax = 1;
        colorAutoExposure.rangemin = 0;

        cameraSettings->GetBrightness(&dtmp);
        colorBrightness = dtmp;
        cameraSettings->GetMaxBrightness(&dtmp);
        colorBrightness.rangemax = dtmp;
        cameraSettings->GetMinBrightness(&dtmp);
        colorBrightness.rangemin = dtmp;

        cameraSettings->GetExposureTime(&dtmp);
        colorExposureTime = dtmp;
        cameraSettings->GetMaxExposureTime(&dtmp);
        colorExposureTime.rangemax = dtmp;
        cameraSettings->GetMinExposureTime(&dtmp);
        colorExposureTime.rangemin = dtmp;

        cameraSettings->GetGain(&dtmp);
        colorGain = dtmp;
        cameraSettings->GetMaxGain(&dtmp);
        colorGain.rangemax = dtmp;
        cameraSettings->GetMinGain(&dtmp);
        colorGain.rangemin = dtmp;

        NUI_POWER_LINE_FREQUENCY ntmp1;
        cameraSettings->GetPowerLineFrequency(&ntmp1);
        colorPowerLineFrequency = itmp;
        colorPowerLineFrequency.rangemax = 2;
        colorPowerLineFrequency.rangemin = 0;

        NUI_BACKLIGHT_COMPENSATION_MODE ntmp2;
        cameraSettings->GetBacklightCompensationMode(&ntmp2);
        colorBacklightCompensationMode = itmp;
        colorBacklightCompensationMode.rangemax = 4;
        colorBacklightCompensationMode.rangemin = 0;

        cameraSettings->GetAutoWhiteBalance(&itmp);
        colorAutoWhiteBalance = itmp;
        colorAutoWhiteBalance.rangemax = 1;
        colorAutoWhiteBalance.rangemin = 0;

        LONG ltmp;
        cameraSettings->GetWhiteBalance(&ltmp);
        colorWhiteBalance = ltmp;
        cameraSettings->GetMaxWhiteBalance(&ltmp);
        colorWhiteBalance.rangemax = ltmp;
        cameraSettings->GetMinWhiteBalance(&ltmp);
        colorWhiteBalance.rangemin = ltmp;

        cameraSettings->GetContrast(&dtmp);
        colorContrast = dtmp;
        cameraSettings->GetMaxContrast(&dtmp);
        colorContrast.rangemax = dtmp;
        cameraSettings->GetMinContrast(&dtmp);
        colorContrast.rangemin = dtmp;

        cameraSettings->GetHue(&dtmp);
        colorHue = dtmp;
        cameraSettings->GetMaxHue(&dtmp);
        colorHue.rangemax = dtmp;
        cameraSettings->GetMinHue(&dtmp);
        colorHue.rangemin = dtmp;

        cameraSettings->GetSaturation(&dtmp);
        colorSaturation = dtmp;
        cameraSettings->GetMaxSaturation(&dtmp);
        colorSaturation.rangemax = dtmp;
        cameraSettings->GetMinSaturation(&dtmp);
        colorSaturation.rangemin = dtmp;

        cameraSettings->GetGamma(&dtmp);
        colorGamma = dtmp;
        cameraSettings->GetMaxGamma(&dtmp);
        colorGamma.rangemax = dtmp;
        cameraSettings->GetMinGamma(&dtmp);
        colorGamma.rangemin = dtmp;

        cameraSettings->GetSharpness(&dtmp);
        colorSharpness = dtmp;
        cameraSettings->GetMaxSharpness(&dtmp);
        colorSharpness.rangemax = dtmp;
        cameraSettings->GetMinSharpness(&dtmp);
        colorSharpness.rangemin = dtmp;

        cameraSettings->Release();

      } else 	cerr << "[UKinect] ERROR: Can not read camera settings or using not supported Kinect XBOX360 device." << endl;


      UNotifyChange(colorResolution, &UKinect::setColorStream);
      UNotifyChange(depthResolution, &UKinect::setDepthStream);
      UNotifyChange(colorAutoExposure, &UKinect::setAutoExposure);
      UNotifyChange(colorBrightness, &UKinect::setBrightness);
      UNotifyChange(colorExposureTime, &UKinect::setExposureTime);
      UNotifyChange(colorGain, &UKinect::setGain);
      UNotifyChange(colorPowerLineFrequency, &UKinect::setPowerLineFrequency);
      UNotifyChange(colorBacklightCompensationMode, &UKinect::setBacklightCompensationMode);
      UNotifyChange(colorAutoWhiteBalance, &UKinect::setAutoWhiteBalance);
      UNotifyChange(colorWhiteBalance, &UKinect::setWhiteBalance);
      UNotifyChange(colorContrast, &UKinect::setContrast);
      UNotifyChange(colorHue, &UKinect::setHue);
      UNotifyChange(colorSaturation, &UKinect::setSaturation);
      UNotifyChange(colorGamma, &UKinect::setGamma);
      UNotifyChange(colorSharpness, &UKinect::setSharpness);
    }


    ///////////////////////////////////////////////////////
    if (depth) {
      depthEmitterOff = 0;
      depthEmitterOff.rangemax = 1;
      depthEmitterOff.rangemin = 0;
      depthNearMode = 0;
      depthNearMode.rangemax = 1;
      depthNearMode.rangemin = 0;
      UNotifyChange(depthNearMode, &UKinect::setNearMode);
      UNotifyChange(depthEmitterOff, &UKinect::setEmitterOff);
    }
    ///////////////////////////////////////////////////////

    if (skeleton) {
      skeletonVisualizationOnColor = 1;
      skeletonVisualizationOnColor.rangemax = 1;
      skeletonVisualizationOnColor.rangemin = 0;

      skeletonTrackingMode = 0;
      skeletonTrackingMode.rangemax = 1;
      skeletonTrackingMode.rangemin = 0;

      skeletonChooserMode = 0;
      skeletonChooserMode.rangemax = 6;
      skeletonChooserMode.rangemin = 0;

      skeletonFilter = 1;
      skeletonFilter.rangemax = 3;
      skeletonFilter.rangemin = 0;

      UNotifyChange(skeletonTrackingMode, &UKinect::setSkeletonStream);
      UNotifyChange(skeletonChooserMode, &UKinect::setSkeletonStream);
    }
    ////////////////////////////////////////////////////////
    if (face) {
      faceVisualizationOnColor = 1;
      faceVisualizationOnColor.rangemax = 1;
      faceVisualizationOnColor.rangemin = 0;

      faceTrackingPause = 0;
      faceTrackingPause.rangemax = 1;
      faceTrackingPause.rangemin = 0;

      faceVisualizationMode = 0;
      faceVisualizationMode.rangemax = 1;
      faceVisualizationMode.rangemin = 0;

      UNotifyChange(faceTrackingPause, &UKinect::setFaceTrackingPause);
    }
    ////////////////////////////////////////////////////////
    if (interaction) {
      interVisualizationOnColor = 1;
      interVisualizationOnColor.rangemax = 1;
      interVisualizationOnColor.rangemin = 0;
    }
    ////////////////////////////////////////////////////////
    if (audio) {

      audioBeamAngle = 0;
      audioBeamAngle.rangemax = 50;
      audioBeamAngle.rangemin = -50;
      audioSourceAngle = 0;
      audioSourceAngle.rangemax = 50;
      audioSourceAngle.rangemin = -50;
      audioSourceConfidence = 0;
      audioSourceConfidence.rangemax = 1;
      audioSourceConfidence.rangemin = 0;

      audioEchoCancellation = 0;
      audioEchoCancellation.rangemax = 1;
      audioEchoCancellation.rangemin = 0;
      audioEchoSuppresion = 0;
      audioEchoSuppresion.rangemax = 2;
      audioEchoSuppresion.rangemin = 0;
      audioNoiseSuppresion = 1;
      audioNoiseSuppresion.rangemax = 1;
      audioNoiseSuppresion.rangemin = 0;
      audioAutomaticGainControl = 0;
      audioAutomaticGainControl.rangemax = 1;
      audioAutomaticGainControl.rangemin = 0;

      UNotifyChange(audioEchoCancellation, &UKinect::setAudioEchoCancellation);
      UNotifyChange(audioEchoSuppresion, &UKinect::setAudioEchoSuppresion);
      UNotifyChange(audioNoiseSuppresion, &UKinect::setAudioNoiseSuppresion);
      UNotifyChange(audioAutomaticGainControl, &UKinect::setAudioAutomaticGainControl);

      hr = pKinectAudioStream->StartCapture();

      if (FAILED(hr)) {
        cerr << "[UKinect] ERROR: Can not start audio stream." << endl;
        return false;
      }
    }

    ////////////////////////////////////////////////////////
    if (speech) {
      speechConfidence.rangemax = 1;
      speechConfidence.rangemin = -1;
      speechConfidenceThreshold.rangemax = 1;
      speechConfidenceThreshold.rangemin = -1;
      speechPause.rangemax = 1;
      speechPause.rangemin = 0;

      UNotifyThreadedChange(speechPause, &UKinect::speechSetPause, LOCK_FUNCTION);
    }
    ////////////////////////////////////////////////////////
    tilt = 0;
    tilt.rangemax = NUI_CAMERA_ELEVATION_MAXIMUM;
    tilt.rangemin = NUI_CAMERA_ELEVATION_MINIMUM;
    UNotifyThreadedChange(tilt, &UKinect::setTilt, LOCK_FUNCTION);

    cout << "[UKinect] INFO: Sensor ready." << endl;
    cout << "[UKinect] *************************************************************" << endl;
    return true;
  } else cerr << "[UKinect] ERROR: Device initializing error." << endl;
  return false;
}

bool UKinect::Close() {

  if (pKinectAudioStream != NULL) {
    if (recording) pKinectAudioStream->RecordStop();
    pKinectAudioStream->StopCapture();
  }


  sensor->NuiShutdown();
  SafeRelease(sensor);


  if (depthNextFrame && (depthNextFrame != INVALID_HANDLE_VALUE)) CloseHandle(depthNextFrame);
  if (colorNextFrame && (colorNextFrame != INVALID_HANDLE_VALUE)) CloseHandle(colorNextFrame);
  if (skeletonNextFrame && (skeletonNextFrame != INVALID_HANDLE_VALUE)) CloseHandle(skeletonNextFrame);
  if (interactionNextFrame && (interactionNextFrame != INVALID_HANDLE_VALUE)) CloseHandle(interactionNextFrame);


  SafeRelease(pFTResult[0]);
  SafeRelease(pFTResult[1]);
  SafeRelease(pFTResult_copy[0]);
  SafeRelease(pFTResult_copy[1]);
  SafeRelease(pFaceTracker[0]);
  SafeRelease(pFaceTracker[1]);

  SafeRelease(colorBuffer);
  SafeRelease(depthBuffer);

  SafeRelease(speechStream);
  SafeRelease(pSpeechRecognizer);
  SafeRelease(pSpeechContext);
  SafeRelease(pSpeechGrammar);
  SafeRelease(interactionStream);

  SafeRelease(pKinectAudioStream);
  SafeRelease(pNuiAudioSource);
  SafeRelease(pPropertyStore);

  colorResolution.unnotify();
  depthResolution.unnotify();
  colorAutoExposure.unnotify();
  colorBrightness.unnotify();
  colorExposureTime.unnotify();
  colorGain.unnotify();
  colorPowerLineFrequency.unnotify();
  colorBacklightCompensationMode.unnotify();
  colorAutoWhiteBalance.unnotify();
  colorWhiteBalance.unnotify();
  colorContrast.unnotify();
  colorHue.unnotify();
  colorSaturation.unnotify();
  colorGamma.unnotify();
  colorSharpness.unnotify();

  depthNearMode.unnotify();
  depthEmitterOff.unnotify();

  skeletonTrackingMode.unnotify();
  skeletonChooserMode.unnotify();

  faceTrackingPause.unnotify();

  audioEchoCancellation.unnotify();
  audioEchoSuppresion.unnotify();
  audioNoiseSuppresion.unnotify();
  audioAutomaticGainControl.unnotify();

  speechPause.unnotify();

  tilt.unnotify();


  ClearData();


  cout << "[UKinect] INFO: Sensor closed successful." << endl;
  cout << "[UKinect] *************************************************************" << endl;
  return true;
}


bool UKinect::PollVideo(bool pwait) {
  // to measure all processing time
  double t = (double)cvGetTickCount();

  // Compute fps - algorithm efficency
  fps = ((double)cvGetTickFrequency() * 1000000) / (t - tick);
  tick = t;

  if (NULL == sensor) {
    cerr << "[UKinect] ERROR: Device not initialized." << endl;
    return false;
  }

  DWORD mSec = 0;
  if (pwait) mSec = 100;

  if (color && (WAIT_OBJECT_0 == WaitForSingleObject(colorNextFrame, mSec))) {
    pollColor();
  }

  if (depth && (WAIT_OBJECT_0 == WaitForSingleObject(depthNextFrame, mSec))) {
    pollDepth();
  }

  if (skeleton && (WAIT_OBJECT_0 == WaitForSingleObject(skeletonNextFrame, 0))) {
    pollSkeleton();
  }

  if (interaction && (WAIT_OBJECT_0 == WaitForSingleObject(interactionNextFrame, 0))) {
    pollInteraction();
  }



  if (face) pollFaces();




  t = (double)cvGetTickCount() - t;
  time = t / ((double)cvGetTickFrequency()*1000.);

  return true;
}



bool UKinect::PollAudio(bool pwait) {
  int t_wait = 0;
  if (pwait) t_wait = INFINITE;

  if (NULL == sensor) {
    cerr << "[UKinect] ERROR: Device not initialized." << endl;
    return false;
  }

  if (audio) pollAudio();

  if (speech && (WAIT_OBJECT_0 == WaitForSingleObject(speechEvent, t_wait))) {
    pollSpeech();
  }

  return true;
}




UStart(UKinect);
