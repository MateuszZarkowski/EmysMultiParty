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
bool UKinect::setFaceStream() {

  pFaceTracker[0] = FTCreateFaceTracker();
  pFaceTracker[1] = FTCreateFaceTracker();
  if (!pFaceTracker[0] || !pFaceTracker[1]) {
    cerr << "[UKinect] ERROR: Can not create face tracker." << endl;
    return false;
  }


  FT_CAMERA_CONFIG colorConfig;
  FT_CAMERA_CONFIG depthConfig;
  hr = getColorConfiguration(&colorConfig);
  if (FAILED(hr)) {
    cerr << "[UKinect] ERROR: Can not get color configuration (face tracker)." << endl;
    return false;
  }
  hr = getDepthConfiguration(&depthConfig);
  if (FAILED(hr)) {
    cerr << "[UKinect] ERROR: Can not get depth configuration (face tracker)." << endl;
    return false;
  }

  for (int i = 0; i < 2; i++) {
    // Initialize the face tracker
    hr = pFaceTracker[i]->Initialize(&colorConfig, &depthConfig, NULL, NULL);
    if (FAILED(hr)) {
      cerr << "[UKinect] ERROR: Can not initialize face tracker "<< i << endl;
      return false;
    }

    if (!pFTResult[i]) {
      // Create a face tracking result interface
      hr = pFaceTracker[i]->CreateFTResult(&pFTResult[i]);
      if (FAILED(hr)) {
        cerr << "[UKinect] ERROR: Can not create face tracking result interface " << i << endl;
        return false;
      }
      hr = pFaceTracker[i]->CreateFTResult(&pFTResult_copy[i]);
      if (FAILED(hr)) {
        cerr << "[UKinect] ERROR: Can not create copy of face tracking result interface " << i << endl;
        return false;
      }
    }
  }


  faceBin.image.width = colorConfig.Width;
  faceBin.image.height = colorConfig.Height;
  faceBin.image.size = colorConfig.Width * colorConfig.Height * 3;

  cout << "[UKinect] INFO: Face stream has been created" << endl;
  return true;
}

bool UKinect::pollFaces() {
  // use skeletonTrackedIDs to get head and shoulders
  if (skeletonTrackedIDs.as< vector<int>>().size() != 2) return false;  //TrackIDIndexCount = 2

  isFaceTracked[0] = pollFaceUser(0);
  isFaceTracked[1] = pollFaceUser(1);
  

  vector<bool> tmp;
  tmp.push_back(isFaceTracked[0]);
  tmp.push_back(isFaceTracked[1]);
  faceIsTracking = tmp;

  if (!isFaceTracked[0] && !isFaceTracked[1] && faceVisualization) {
    faceCVMat = Mat(Size(static_cast<int>(colorBuffer->GetWidth()), static_cast<int>(colorBuffer->GetHeight())), CV_8UC3, CV_RGB(0, 0, 0));
    putText(faceCVMat, "no face", Point(160, 260), 4, 2, CV_RGB(0, 0, 200), 4);
    faceBin.image.data = faceCVMat.data;
    faceImage = faceBin;
  }

  return isFaceTracked[0] || isFaceTracked[1];

}


bool UKinect::pollFaceUser(int user) {
  if (user != 0 && user != 1) return false;

  FT_SENSOR_DATA sensorData;
  sensorData.pVideoFrame = colorBuffer;
  sensorData.pDepthFrame = depthBuffer;
  sensorData.ZoomFactor = 1.0f;       // Not used must be 1.0
  sensorData.ViewOffset.x = 0; // Not used must be (0,0)
  sensorData.ViewOffset.y = 0; // Not used must be (0,0)

  IFTFaceTracker* _pFaceTracker = pFaceTracker[user];	//				// An instance of a face tracker
  IFTResult*  _pFTResult = pFTResult[user];							// Face tracking result interface
  IFTResult*  _pFTResult_copy = pFTResult_copy[user];						// Copy of Face tracking result interface
  bool _isFaceTracked = isFaceTracked[user];
  //                        red          yellow
  int color = (user == 0) ? 0x00FF0000 : 0x00FFFF00;

  int trackedID = skeletonTrackedIDs.as< vector<int>>()[user];

  FT_VECTOR3D headHint[2];
  if (trackedID != 0) {
    vector<double> shoulders = skeletonJointPosition(trackedID, NUI_SKELETON_POSITION_SHOULDER_CENTER);
    if (shoulders.size() == 0) return false;
    headHint[0] = FT_VECTOR3D(shoulders[0], shoulders[1], shoulders[2]);

    vector<double> head = skeletonJointPosition(trackedID, NUI_SKELETON_POSITION_HEAD);
    if (head.size() == 0) return false;
    headHint[1] = FT_VECTOR3D(head[0], head[1], head[2]);
  } else return false;


  // Check if we are already tracking a face
  if (!_isFaceTracked) {
    // Initiate face tracking.
    // This call is more expensive and searches over the input RGB frame for a face.
    // However if hint != null id limits only to head region
    hr = _pFaceTracker->StartTracking(&sensorData, NULL, headHint, _pFTResult);
  } else {
    // Continue tracking. It uses a previously known face position.
    // This call is less expensive than StartTracking()
    hr = _pFaceTracker->ContinueTracking(&sensorData, headHint, _pFTResult);
  }

  // exit on fail
  if (FAILED(hr) || FAILED(_pFTResult->GetStatus())) {
    _pFTResult->Reset();
    return false;
  }

  _pFTResult->CopyTo(_pFTResult_copy);

  if (faceVisualization) {
    FLOAT* pSU = NULL;
    UINT numSU;
    BOOL suConverged;
    hr = _pFaceTracker->GetShapeUnits(NULL, &pSU, &numSU, &suConverged);
    if (FAILED(hr)) {
      cerr << "[UKinect] ERROR: Can not get SU units." << endl;
      throw;
    }

    POINT viewOffset = { 0, 0 };
    FT_CAMERA_CONFIG colorConfig;
    getColorConfiguration(&colorConfig);

    IFTModel* ftModel;
    HRESULT hr = _pFaceTracker->GetFaceModel(&ftModel);
    if (FAILED(hr)) {
      cerr << "[UKinect] ERROR: Can not get Face Model." << endl;
      throw;
    }

    Mat tmp;
    if (!faceVisualizationOnColor.as<int>()) {
      tmp = Mat(Size(static_cast<int>(colorBuffer->GetWidth()), static_cast<int>(colorBuffer->GetHeight())), CV_8UC4, CV_RGB(0, 0, 0));
      memcpy(colorBuffer->GetBuffer(), tmp.data, min(colorBuffer->GetBufferSize(), UINT(colorBuffer->GetBufferSize())));
    }
    if (faceVisualizationMode.as<int>())
      hr = VisualizeFacetracker(colorBuffer, _pFTResult, color);
    else
      hr = VisualizeFaceModel(colorBuffer, ftModel, &colorConfig, pSU, 1.0, viewOffset, _pFTResult, color);

    if (FAILED(hr)) {
      cerr << "[UKinect] ERROR: Cannot visualize Face Model." << endl;
      throw;
    }
    tmp = Mat(Size(static_cast<int>(colorBuffer->GetWidth()), static_cast<int>(colorBuffer->GetHeight())), CV_8UC4, colorBuffer->GetBuffer());
    cvtColor(tmp, faceCVMat, CV_BGRA2RGB); // <- alloc new memory
    // Save CV image to UImage
    faceBin.image.data = faceCVMat.data;
    faceImage = faceBin;

    ftModel->Release();
  }

  return true;

}

bool UKinect::getColorConfiguration(FT_CAMERA_CONFIG* colorConfig) {
  if (!colorConfig) {
    return false;
  }

  UINT width = colorBuffer ? colorBuffer->GetWidth() : 0;
  UINT height = colorBuffer ? colorBuffer->GetHeight() : 0;
  FLOAT focalLength = 0.f;

  if (width == 640 && height == 480) {
    focalLength = NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS;
  } else if (width == 1280 && height == 960) {
    focalLength = NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS * 2.f;
  }

  if (focalLength == 0.f) {
    return false;
  }


  colorConfig->FocalLength = focalLength;
  colorConfig->Width = width;
  colorConfig->Height = height;

  return true;
}

bool UKinect::getDepthConfiguration(FT_CAMERA_CONFIG* depthConfig) {
  if (!depthConfig) {
    return false;
  }

  UINT width = depthBuffer ? depthBuffer->GetWidth() : 0;
  UINT height = depthBuffer ? depthBuffer->GetHeight() : 0;
  FLOAT focalLength = 0.f;

  if (width == 80 && height == 60) {
    focalLength = NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS / 4.f;
  } else if (width == 320 && height == 240) {
    focalLength = NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS;
  } else if (width == 640 && height == 480) {
    focalLength = NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS * 2.f;
  }

  if (focalLength == 0.f) {
    return false;
  }

  depthConfig->FocalLength = focalLength;
  depthConfig->Width = width;
  depthConfig->Height = height;

  return true;
}

void UKinect::setFaceTrackingPause() {
  if (pFaceTracker[0] == NULL && pFaceTracker[1] == NULL) return;
  face = !faceTrackingPause.as<bool>();
  isFaceTracked[0] = false;
  isFaceTracked[1] = false;
  faceIsTracking = vector<bool>(2, false);
}

void UKinect::setEmotion(int user, const float* AU, const int numberAU) {
  if (user != 0 && user != 1) return;
  string _faceEmotion;

  if (numberAU >= 6) {
    //if (AU[3] > 0.1f && AU[5] > 0.05f)
    if (AU[3] > 0.3f) {   // If the eyebrows are lowered, draw angry eyes
      _faceEmotion = "angry";
    } else if (AU[3] < -0.1f && AU[2] > 0.1f && AU[4] > 0.1f) {   // If eyebrow up and mouth stretched, draw fearful eyes
      _faceEmotion = "fear";
    } else if (AU[1] > 0.1f && AU[3] < -0.1f) {   // if eyebrow up and mouth open, draw big surprised eyes
      _faceEmotion = "surprise";
    } else if ((AU[2] - AU[4]) > 0.1f && AU[4] < 0) {   // If lips are stretched, assume smile and draw smily eyes
      _faceEmotion = "joy";
    } else if ((AU[2] - AU[4]) < 0.1f &&  AU[5] < -0.3f) {   // If lips low and eyebrow slanted up draw sad eyes
      _faceEmotion = "sad";
    } else // by default, just draw the default eyes
    {
      _faceEmotion = "neutral";
    }
  }//TODO: ??? else ?


  vector<string> tmp = faceEmotion.as<vector<string>>();
  tmp[user] = _faceEmotion;
  faceEmotion = tmp;
}




vector<double> UKinect::facePosition(int user) {
  vector<double> position_list;
  if (user != 0 && user != 1) return position_list;

  IFTResult *_pFTResult_copy = pFTResult_copy[user];
  bool _isFaceTracked = isFaceTracked[user];

  if (_isFaceTracked) {
    float rot[3], trans[3], scale;
    hr = _pFTResult_copy->Get3DPose(&scale, rot, trans);
    if (FAILED(hr)) {
      cerr << "[UKinect] ERROR: Can not get face position." << endl;
      return position_list;
    }
    position_list.push_back(static_cast<double>(trans[0]));
    position_list.push_back(static_cast<double>(trans[1]));
    position_list.push_back(static_cast<double>(trans[2]));
    position_list.push_back(static_cast<double>(rot[0]));
    position_list.push_back(static_cast<double>(rot[1]));
    position_list.push_back(static_cast<double>(rot[2]));
  }
  return position_list;
}


vector<int> UKinect::facePositionOnImage(int user) {
  vector<int> rect_list;
  if (user != 0 && user != 1) return rect_list;

  IFTResult *_pFTResult_copy = pFTResult_copy[user];
  bool _isFaceTracked = isFaceTracked[user];


  if (_isFaceTracked) {

    RECT rectFace;
    hr = _pFTResult_copy->GetFaceRect(&rectFace);
    if (FAILED(hr)) {
      cerr << "[UKinect] ERROR: Can not get face position on image." << endl;
      return rect_list;
    }
    rect_list.push_back(static_cast<int>(rectFace.left + (rectFace.right - rectFace.left) / 2));
    rect_list.push_back(static_cast<int>(rectFace.top + (rectFace.bottom - rectFace.top) / 2));
    rect_list.push_back(static_cast<int>(rectFace.right - rectFace.left));
    rect_list.push_back(static_cast<int>(rectFace.bottom - rectFace.top));
  }
  return rect_list;
}


vector<vector<int>> UKinect::facePointsOnImage(int user) {
  vector<vector<int>> points_list;
  if (user != 0 && user != 1) return points_list;

  IFTResult *_pFTResult_copy = pFTResult_copy[user];
  bool _isFaceTracked = isFaceTracked[user];

  
  if (_isFaceTracked) {
    FT_VECTOR2D* pPts2D;
    UINT pts2DCount;
    hr = _pFTResult_copy->Get2DShapePoints(&pPts2D, &pts2DCount);
    if (FAILED(hr)) {
      cerr << "[UKinect] ERROR: Can not get face points on image." << endl;
      return points_list;
    }
    for (UINT ipt = 0; ipt < pts2DCount; ++ipt) {
      vector<int> tmp_list;
      tmp_list.push_back(static_cast<int>(pPts2D[ipt].x));
      tmp_list.push_back(static_cast<int>(pPts2D[ipt].y));
      points_list.push_back(tmp_list);
    }
  }
  return points_list;
}



vector<double> UKinect::faceAU(int user) {
  vector<double> au_list;
  if (user != 0 && user != 1) return au_list;

  IFTResult *_pFTResult_copy = pFTResult_copy[user];
  bool _isFaceTracked = isFaceTracked[user];


  if (_isFaceTracked) {
    FLOAT* pAU = NULL;
    UINT numAU;
    hr = _pFTResult_copy->GetAUCoefficients(&pAU, &numAU);
    setEmotion(user, pAU, numAU);
    if (FAILED(hr)) {
      cerr << "[UKinect] ERROR: Can not get AU coefficients." << endl;
      return au_list;
    }
    for (UINT ipt = 0; ipt < numAU; ++ipt) {
      au_list.push_back(static_cast<double>(pAU[ipt]));
    }
  } else  cerr << "[UKinect] NO TRACK." << endl;
  return au_list;
}


vector<double> UKinect::faceSU(int user) {
  vector<double> su_list;
  if (user != 0 && user != 1) return su_list;

  IFTFaceTracker *_pFaceTracker = pFaceTracker[user];
  bool _isFaceTracked = isFaceTracked[user];


  if (_isFaceTracked) {
    FLOAT* pSU = NULL;
    UINT numSU;
    BOOL suConverged;
    hr = _pFaceTracker->GetShapeUnits(NULL, &pSU, &numSU, &suConverged);
    if (FAILED(hr)) {
      cerr << "[UKinect] ERROR: Can not get SU units." << endl;
      return su_list;
    }
    for (UINT ipt = 0; ipt < numSU; ++ipt) {
      su_list.push_back(static_cast<double>(pSU[ipt]));
    }
    ///////////////////////////////////////////////////////////////////
  }
  return su_list;
}