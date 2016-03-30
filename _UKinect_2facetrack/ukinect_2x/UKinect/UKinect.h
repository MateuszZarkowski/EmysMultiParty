// UKinect is a module for Urbi to handle Microsoft Kinect sensor.
// Copyright (C) 2014 Jan Kedzierski

// You should have received a copy of the GNU General Public License
// UKinect is a module for Urbi to handle Microsoft Kinect sensor.
// Copyright (C) 2014 Jan Kedzierski

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



#pragma once

// For URBI use
#include <urbi/uobject.hh>
#include <urbi/uconversion.hh>
#include <urbi/uabstractclient.hh>

// urbi fake!!!!!!
//#define WINVER          0x0601

// For OpenCV functions
#include <opencv.hpp>

//include KINECT SDK headers
#include <windows.h>
#include <NuiApi.h>
#include <FaceTrackLib.h>
#include <KinectInteraction.h>

// standard headers
#include <map>
#include <vector>
#include <iterator>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <cmath>

// audio headers
// For IMediaObject and related interfaces
#include <dmo.h>
// For configuring DMO properties
#include <wmcodecdsp.h>
// For WAVEFORMATEX
#include <mmreg.h>
// For FORMAT_WaveFormatEx and such
#include <uuids.h>
// Windows Header Files
#include <Shlobj.h>

// For speech APIs
#include <sapi.h>
#include <sphelper.h>
// NOTE: To ensure that application compiles and links against correct SAPI versions (from Microsoft Speech
//       SDK), VC++ include and library paths should be configured to list appropriate paths within Microsoft
//       Speech SDK installation directory before listing the default system include and library directories,
//       which might contain a version of SAPI that is not appropriate for use together with Kinect sensor.
//		 Global Unique ID (GUID) is a unique reference number used as an identifier in computer software (in this 
//		 case to identifier Microsoft Speech Platform).
//
//		 This module was compiled with Microsoft Speech Platform SDK ver. 11.0
//
#define INITGUID
#include <guiddef.h>
DEFINE_GUID(CLSID_ExpectedRecognizer, 0x495648e7, 0xf7ab, 0x4267, 0x8e, 0x0f, 0xca, 0xfb, 0x7a, 0x33, 0xc1, 0x60);


// Project headers
#include "NuiActivityWatcher.h"
#include "Utility.h"
#include "Visualize.h"
#include "KinectAudioStream.h"

#define DEPTH_SCALE_FACTOR (255./65535.)

using namespace cv;
using namespace urbi;
using namespace std;


class UKinect : public UObject {
public:
  // The class must have a single constructor taking a string. 
  UKinect(const std::string&);
  ~UKinect();

private:
  int init();		// Urbi constructor. Throw error in case of error.
  void ClearData();	// Clear all data
  ///////////////////////////////////////////////////////////////////////////////////
  //
  //  C O M M O N   M O D U L E    F U N C T I O N
  INuiSensor * sensor;							// Kinect device
  HRESULT hr;										// Process result
  //
  double	tick;
  //
  // global bool variables to turn ON/OFF main module functions
  bool color;
  bool depth;
  bool skeleton;
  bool audio;
  bool speech;
  bool face;
  bool interaction;
  //	
  // streams functions
  bool setColorStream();
  bool setDepthStream();
  bool setSkeletonStream();
  bool setFaceStream();
  bool setInteractionStream();
  bool setAudioStream();
  bool setSpeechStream();
  //
  // polling functions
  bool pollDepth();
  bool pollColor();
  bool pollSkeleton();
  void pollSpeech();
  void pollAudio();
  bool pollFaces();
  bool pollInteraction();
  //
  // C O L O R    S E C T I O N
  //
  HANDLE colorStream;								// The identifier of the Kinect's color stream
  HANDLE colorNextFrame;							// The identifier of the Kinect's color frame ready
  //
  UBinary	colorBin;
  Mat		colorCVMat;
  //
  void	setAutoExposure();
  void	setBrightness();
  void	setExposureTime();
  void	setGain();
  void	setPowerLineFrequency();
  void	setBacklightCompensationMode();
  void	setAutoWhiteBalance();
  void	setWhiteBalance();
  void	setContrast();
  void	setHue();
  void	setSaturation();
  void	setGamma();
  void	setSharpness();
  //
  // D E P T H   S E C T I O N
  //
  HANDLE depthStream;								// The identifier of the Kinect's depth stream
  HANDLE depthNextFrame;							// The identifier of the Kinect's depth frame ready
  //
  UBinary	depthBin;
  Mat		depthCVMat;
  void	setNearMode();
  void	setEmitterOff();
  //
  // S K E L E T O N    S E C T I O N
  //
  UBinary	skeletonBin;		// binary skeleton image   
  Mat		skeletonCVMat;		// OpenCV skeleton image	
  //
  HANDLE skeletonNextFrame;						// The identifier of the Kinect's skeleton frame ready
  //
  NUI_SKELETON_FRAME skeletonFrame;				// Current skeleton frame
  DWORD	stickyIDs[TrackIDIndexCount];			// Tracked player ID index
  map<int, NuiActivityWatcher*> activityWatchers; // To track the activity of a given player over time
  //
  void	drawPosition(const NUI_SKELETON_DATA&);
  void	drawSkeleton(const NUI_SKELETON_DATA&);
  void	drawJoint(const NUI_SKELETON_DATA&, NUI_SKELETON_POSITION_INDEX);
  void	drawBone(const NUI_SKELETON_DATA&, NUI_SKELETON_POSITION_INDEX, NUI_SKELETON_POSITION_INDEX);
  void	drawOutOfFrame(const NUI_SKELETON_DATA&);
  vector<int>	UpdateTrackedSkeletons();
  void	ChooseStickySkeletons(DWORD *);
  void	ChooseClosestSkeletons(DWORD *);
  void	ChooseMostActiveSkeletons(DWORD *);
  void	FindStickyIDs(DWORD *);
  void	AssignNewStickyIDs(DWORD *);
  void	ResetActivityWatcherFlags();
  void	UpdateActivityWatchers();
  void	DeleteNonUpdateWatchers();
  void	FindMostActiveIDs(DWORD *);
  //
  // F A C E    S E C T I O N
  //
  UBinary		faceBin;
  Mat			faceCVMat;
  //
  IFTFaceTracker* pFaceTracker[2];	//				// An instance of a face tracker
  IFTResult*  pFTResult[2];							// Face tracking result interface
  IFTResult*  pFTResult_copy[2];						// Copy of Face tracking result interface
  //
  bool isFaceTracked[2];								// If face is tracking?	
  //
  IFTImage*   colorBuffer;						// Copy of color image for face tracking 
  IFTImage*   depthBuffer;						// Copy of depth image for face tracking 

  bool pollFaceUser(int);
  bool getColorConfiguration(FT_CAMERA_CONFIG*);	// get color camera configuration (color mode, resolution) for face tracking purpose
  bool getDepthConfiguration(FT_CAMERA_CONFIG*);	// get depth camera configuration (color mode, resolution) for face tracking purpose
  void setFaceTrackingPause();					// 
  void setEmotion(int, const float*, const int);
  //
  // I N T E R A C T I O N    S E C T I O N
  //
  void drawHand(DWORD, int, int, bool, bool, double);
  //
  INuiInteractionStream * interactionStream;		// The identifier of the interaction stream
  HANDLE interactionNextFrame;					// The identifier of the interaction frame ready 
  //
  UBinary		interBin;
  Mat			interCVMat;
  //
  CIneractionClient nuiIClient;					// Interaction interface
  //
  // A U D I O    S E C T I O N
  //
  KinectAudioStream*  pKinectAudioStream;			// Audio stream captured from Kinect
  INuiAudioBeam *		pNuiAudioSource;			// Audio source used to query Kinect audio beam and sound source angles.
  IPropertyStore*     pPropertyStore;				// Property store used to configure Kinect audio properties.
  bool				recording;
  //
  void setAudioEchoCancellation();
  void setAudioEchoSuppresion();
  void setAudioNoiseSuppresion();
  void setAudioAutomaticGainControl();
  void getFEATURE_MODE(PROPERTYKEY); // NOT USED with UObject IT IS ONLY FOR DEBUG USE
  //
  // S P E E C H    S E C T I O N
  //
  ISpStream*  speechStream;						// Stream given to speech recognition engine
  HANDLE speechEvent;								// Event triggered when we detect speech recognition
  //
  ISpRecognizer*          pSpeechRecognizer;		// Speech recognizer
  ISpRecoContext*         pSpeechContext;			// Speech recognizer context
  ISpRecoGrammar*         pSpeechGrammar;			// Speech grammar
  //
  string utf8_encode(const std::wstring &wstr);
  void speechSetPause();								// Set speech engine pause
  //
  // A C C E L E R O M E T E R    S E C T I O N
  //
  void readAccelerometer();
  //
  // T I L T    S E C T I O N
  //
  void setTilt();

  //////////////////////////////////
  //
  // U R B I    I n t e r f c e
  //
  // Bounded functions
  // see README file for details.
  //
  bool Open(int, bool, bool, bool, bool, bool, bool, bool);		// Open funcytion to connect to Kinect device
  bool UKinect::Close();									// Release device
  bool PollAudio(bool);									// Pool data function
  bool PollVideo(bool);									// Pool data function
  void colorResetSettings();								// Reset all color camera settings
  vector<double> accelerometer();							// Gets accelerometer data
  vector<double> skeletonPosition(DWORD);					// Gets tracked skeleton position x-y-z	
  vector<double> skeletonPositionOnImage(DWORD); 			// Gets tracked skeleton position on image x-y
  vector<double> skeletonJointPosition(DWORD, int);		// Gets joint position x-y-z
  vector<double> skeletonJointPositionOnImage(DWORD, int);// Gets joint position on image x-y
  vector<double> facePosition(int);							// Gets face position x-y-z-roll-pitch-yaw
  vector<int> facePositionOnImage(int);						// Gets face position on image x-y
  vector<vector<int>> facePointsOnImage(int);				// Gets 2D face points on image
  vector<double> faceAU(int);								// Gets 6 AUs
  vector<double> faceSU(int);								// Gets 11 SUs
  void audioPause(bool);									// Set audio stream paused
  bool audioRecordStart(const string&);					// Start recording audio stream to wav file
  bool audioRecordStop();									// Stop recordng audio
  bool speechResetGrammar();								// Reset grammar
  bool speechLoadGrammar(const string&);					// Load Grammar rules to speech recognition engine
  bool speechAddPhrase(const string&, const string&);	// Add single phrase
  // Bounded variables
  // see README file for details.
  //
  // color
  UVar colorEnabled;
  UVar colorImage, colorResolution, colorWidth, colorHeight;
  UVar colorAutoExposure, colorBrightness, colorExposureTime, colorGain, colorPowerLineFrequency, colorBacklightCompensationMode;
  UVar colorAutoWhiteBalance, colorWhiteBalance, colorContrast, colorHue, colorSaturation, colorGamma, colorSharpness;
  // depth
  UVar depthEnabled;
  UVar depthImage, depthVisualization, depthResolution, depthWidth, depthHeight;
  UVar depthNearMode, depthEmitterOff;
  // skeleton
  UVar skeletonEnabled;
  UVar skeletonImage, skeletonVisualizationOnColor, skeletonVisualization;
  UVar skeletonTrackingMode, skeletonChooserMode, skeletonIDs, skeletonTrackedIDs, skeletonFilter;
  // face
  UVar faceEnabled;
  UVar faceImage, faceVisualizationOnColor, faceVisualization;
  UVar faceTrackingPause, faceVisualizationMode, faceIsTracking, faceEmotion;
  // interaction
  UVar interEnabled;
  UVar interID, interImage, interVisualization, interVisualizationOnColor;
  UVar interLeftTracked, interLeftActive, interLeftInteractive, interLeftPressed, interLeftEvent, interLeftX, interLeftY, interLeftRawX, interLeftRawY, interLeftRawZ, interLeftPress;
  UVar interRightTracked, interRightActive, interRightInteractive, interRightPressed, interRightEvent, interRightX, interRightY, interRightRawX, interRightRawY, interRightRawZ, interRightPress;
  // audio
  UVar audioEnabled;
  UVar audioBeamAngle, audioSourceAngle, audioSourceConfidence, audioEchoCancellation, audioEchoSuppresion, audioNoiseSuppresion, audioAutomaticGainControl;
  // speech
  UVar speechEnabled;
  UVar speechRecognizer, speechAvailableRecognizers, speechResult, speechResultTag, speechIsListening, speechConfidence, speechConfidenceThreshold, speechPause;
  // kinect tilt angle
  UVar tilt;
  // common
  UVar	fps, time;
  // debug
  UVar debug;
};