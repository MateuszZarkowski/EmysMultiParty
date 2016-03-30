/*******************************************
*
*	UKinect
*   Author: Jan Kedzierski
*
********************************************/

#include "UKinect.h"


bool UKinect::setSkeletonStream() {
  if (NULL != sensor) {

    // Enable tracking skeleton
    DWORD flags = (skeletonTrackingMode.as<int>() ? NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT : 0) | NUI_SKELETON_TRACKING_FLAG_ENABLE_IN_NEAR_RANGE
      | (skeletonChooserMode.as<int>() != ChooserModeDefault ? NUI_SKELETON_TRACKING_FLAG_TITLE_SETS_TRACKED_SKELETONS : 0);

    // Open a skeleton stream 
    hr = sensor->NuiSkeletonTrackingEnable(skeletonNextFrame, flags);
    if (FAILED(hr)) {
      cerr << "[UKinect] ERROR: Can not enable skeleton stream." << endl;
      return false;
    }

    if (S_OK == hr) {

      string t_tracking_mode;
      switch (skeletonTrackingMode.as<int>()) {
      case 0:
        t_tracking_mode = "\"default (full skeleton)\"";
        break;
      case 1:
        t_tracking_mode = "\"seated (upper body)\"";
        break;
      }
      string t_chooser_mode;
      switch (skeletonChooserMode.as<int>()) {
      case ChooserModeDefault:
        t_chooser_mode = "\"default\"";
        break;
      case ChooserModeClosest1:
        t_chooser_mode = "\"closest 1\"";
        break;
      case ChooserModeClosest2:
        t_chooser_mode = "\"closest 2\"";
        break;
      case ChooserModeSticky1:
        t_chooser_mode = "\"sticky 1\"";
        break;
      case ChooserModeSticky2:
        t_chooser_mode = "\"sticky 2\"";
        break;
      case ChooserModeActive1:
        t_chooser_mode = "\"most active 1\"";
        break;
      case ChooserModeActive2:
        t_chooser_mode = "\"most active 2\"";
        break;
      }

      cout << "[UKinect] INFO: Skeleton stream has been created with tracking mode \n                " << t_tracking_mode << " and chooser mode " << t_chooser_mode << "." << endl;
      return true;
    }
  }
  cerr << "[UKinect] ERROR: Skeleton stream initializing error." << endl;
  return false;
}


bool UKinect::pollSkeleton() {
  if (NULL != sensor) {
    //
    // Attempt to get the color frame
    hr = sensor->NuiSkeletonGetNextFrame(0, &skeletonFrame);
    if (FAILED(hr)) {
      cerr << "[UKinect] WARNING: Skeleton pool." << endl;
      return false;
    }
    //
    // smooth out the skeleton data
    if (skeletonFilter.as<int>() == 0) {
      //sensor->NuiTransformSmooth(&skeletonFrame, NULL);
    } else if (skeletonFilter.as<int>() == 1) {
      //const NUI_TRANSFORM_SMOOTH_PARAMETERS DefaultParams = {0.5f, 0.5f, 0.5f, 0.05f, 0.04f};
      sensor->NuiTransformSmooth(&skeletonFrame, NULL);
    } else if (skeletonFilter.as<int>() == 2) {
      const NUI_TRANSFORM_SMOOTH_PARAMETERS SomewhatLatentParams = { 0.5f, 0.1f, 0.5f, 0.1f, 0.1f };
      sensor->NuiTransformSmooth(&skeletonFrame, &SomewhatLatentParams);
    } else {
      const NUI_TRANSFORM_SMOOTH_PARAMETERS VerySmoothParams = { 0.7f, 0.3f, 1.0f, 1.0f, 1.0f };
      sensor->NuiTransformSmooth(&skeletonFrame, &VerySmoothParams);
    }
    //
    // process skeleton frame if interaction function enabled
    if (interaction) {
      Vector4 v;
      sensor->NuiAccelerometerGetCurrentReading(&v);
      hr = interactionStream->ProcessSkeleton(NUI_SKELETON_COUNT,
        skeletonFrame.SkeletonData,
        &v,
        skeletonFrame.liTimeStamp);
      if (FAILED(hr)) {
        cerr << "[UKinect] ERROR: Process skeleton failed (for interaction purpose)." << endl;
        return false;
      }
    }


    vector<int> skelIDs;

    // these are used in face tracking
    vector<int> skelTrackedIDs = UpdateTrackedSkeletons();   // << use this to set tracked

    for (int i = 0; i < NUI_SKELETON_COUNT; ++i) {
      NUI_SKELETON_TRACKING_STATE trackingState = skeletonFrame.SkeletonData[i].eTrackingState;
      if (NUI_SKELETON_POSITION_ONLY == trackingState) skelIDs.push_back((int)skeletonFrame.SkeletonData[i].dwTrackingID);
    }

    // Save vectors to UVars
    skeletonIDs = skelIDs;
    skeletonTrackedIDs = skelTrackedIDs;

    if (skeletonVisualization) {

      DWORD t_width, t_height;
      NuiImageResolutionToSize((NUI_IMAGE_RESOLUTION)colorResolution.as<int>(), t_width, t_height);

      if
        ((color) && (skeletonVisualizationOnColor.as<int>())) skeletonCVMat = colorCVMat.clone(); // use color image as a background if color function enabled
      else
        skeletonCVMat = Mat(Size(static_cast<int>(t_width), static_cast<int>(t_height)), CV_8UC3, CV_RGB(0, 0, 0));

      for (int i = 0; i < NUI_SKELETON_COUNT; ++i) {
        NUI_SKELETON_TRACKING_STATE trackingState = skeletonFrame.SkeletonData[i].eTrackingState;

        if (NUI_SKELETON_TRACKED == trackingState) {
          // We're tracking the skeleton, draw it
          drawSkeleton(skeletonFrame.SkeletonData[i]);
          drawPosition(skeletonFrame.SkeletonData[i]);
        } else if (NUI_SKELETON_POSITION_ONLY == trackingState) {
          // we've only received the center point of the skeleton, draw that
          drawPosition(skeletonFrame.SkeletonData[i]);
        }

        drawOutOfFrame(skeletonFrame.SkeletonData[i]);
      }

      // Save CV image to UImage
      skeletonBin.image.width = skeletonCVMat.cols;
      skeletonBin.image.height = skeletonCVMat.rows;
      skeletonBin.image.size = skeletonCVMat.cols * skeletonCVMat.rows * 3;
      skeletonBin.image.data = skeletonCVMat.data;
      skeletonImage = skeletonBin;
    }

    return true;

  }
  cerr << "[UKinect] ERROR: Skeleton pool error." << endl;
  return false;
}



/// <summary>
/// Draw skeleton.
/// </summary>
/// <param name="skeletonData">Skeleton coordinates</param>
/// <param name="imageRect">The rect which the color or depth stream image is streched to fit</param>
void UKinect::drawSkeleton(const NUI_SKELETON_DATA& skeletonData) {
  // Torso
  drawBone(skeletonData, NUI_SKELETON_POSITION_HEAD, NUI_SKELETON_POSITION_SHOULDER_CENTER);
  drawBone(skeletonData, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_LEFT);
  drawBone(skeletonData, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_RIGHT);
  drawBone(skeletonData, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SPINE);
  drawBone(skeletonData, NUI_SKELETON_POSITION_SPINE, NUI_SKELETON_POSITION_HIP_CENTER);
  drawBone(skeletonData, NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_LEFT);
  drawBone(skeletonData, NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_RIGHT);

  // Left arm
  drawBone(skeletonData, NUI_SKELETON_POSITION_SHOULDER_LEFT, NUI_SKELETON_POSITION_ELBOW_LEFT);
  drawBone(skeletonData, NUI_SKELETON_POSITION_ELBOW_LEFT, NUI_SKELETON_POSITION_WRIST_LEFT);
  drawBone(skeletonData, NUI_SKELETON_POSITION_WRIST_LEFT, NUI_SKELETON_POSITION_HAND_LEFT);

  // Right arm
  drawBone(skeletonData, NUI_SKELETON_POSITION_SHOULDER_RIGHT, NUI_SKELETON_POSITION_ELBOW_RIGHT);
  drawBone(skeletonData, NUI_SKELETON_POSITION_ELBOW_RIGHT, NUI_SKELETON_POSITION_WRIST_RIGHT);
  drawBone(skeletonData, NUI_SKELETON_POSITION_WRIST_RIGHT, NUI_SKELETON_POSITION_HAND_RIGHT);

  // Left leg
  drawBone(skeletonData, NUI_SKELETON_POSITION_HIP_LEFT, NUI_SKELETON_POSITION_KNEE_LEFT);
  drawBone(skeletonData, NUI_SKELETON_POSITION_KNEE_LEFT, NUI_SKELETON_POSITION_ANKLE_LEFT);
  drawBone(skeletonData, NUI_SKELETON_POSITION_ANKLE_LEFT, NUI_SKELETON_POSITION_FOOT_LEFT);

  // Right leg
  drawBone(skeletonData, NUI_SKELETON_POSITION_HIP_RIGHT, NUI_SKELETON_POSITION_KNEE_RIGHT);
  drawBone(skeletonData, NUI_SKELETON_POSITION_KNEE_RIGHT, NUI_SKELETON_POSITION_ANKLE_RIGHT);
  drawBone(skeletonData, NUI_SKELETON_POSITION_ANKLE_RIGHT, NUI_SKELETON_POSITION_FOOT_RIGHT);

  // Draw joints
  for (int i = 0; i < NUI_SKELETON_POSITION_COUNT; i++) {
    drawJoint(skeletonData, (NUI_SKELETON_POSITION_INDEX)i);
  }

}

/// <summary>
/// Draw a circle to indicate a skeleton of which only position info is available
/// </summary>
/// <param name="skeletonData">Skeleton coordinates</param>
/// <param name="imageRect">The rect which the color or depth stream image is streched to fit</param>
void UKinect::drawPosition(const NUI_SKELETON_DATA& skeletonData) {
  LONG xd, yd, x, y;
  USHORT depth;
  NuiTransformSkeletonToDepthImage(skeletonData.Position, &xd, &yd, &depth, (NUI_IMAGE_RESOLUTION)depthResolution.as<int>());
  NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution((NUI_IMAGE_RESOLUTION)colorResolution.as<int>(), (NUI_IMAGE_RESOLUTION)depthResolution.as<int>(), NULL, xd, yd, depth, &x, &y);
  circle(skeletonCVMat, Point(static_cast<int>(x), static_cast<int>(y)), 10, CV_RGB(0, 50, 200), 2);
  putText(skeletonCVMat, lexical_cast<string>(skeletonData.dwTrackingID), Point(x, y), 1, 2, CV_RGB(255, 255, 255), 2);
}

/// <summary>
/// Draw a bone between 2 tracked joint.
/// <summary>
/// <param name="skeletonData">Skeleton coordinates</param>
/// <param name="imageRect">The rect which the color or depth image is streched to fit</param>
/// <param name="joint0">Index for the first joint</param>
/// <param name="joint1">Index for the second joint</param>
void UKinect::drawBone(const NUI_SKELETON_DATA& skeletonData, NUI_SKELETON_POSITION_INDEX joint0, NUI_SKELETON_POSITION_INDEX joint1) {
  NUI_SKELETON_POSITION_TRACKING_STATE state0 = skeletonData.eSkeletonPositionTrackingState[joint0];
  NUI_SKELETON_POSITION_TRACKING_STATE state1 = skeletonData.eSkeletonPositionTrackingState[joint1];

  // Any is not tracked
  if (NUI_SKELETON_POSITION_NOT_TRACKED == state0 || NUI_SKELETON_POSITION_NOT_TRACKED == state1) {
    return;
  }

  // Both are inferred
  if (NUI_SKELETON_POSITION_INFERRED == state0 && NUI_SKELETON_POSITION_INFERRED == state1) {
    return;
  }

  LONG x1, y1, x2, y2;
  LONG x1d, y1d, x2d, y2d;
  USHORT depth;

  NuiTransformSkeletonToDepthImage(skeletonData.SkeletonPositions[joint0], &x1d, &y1d, &depth, (NUI_IMAGE_RESOLUTION)depthResolution.as<int>());
  NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution((NUI_IMAGE_RESOLUTION)colorResolution.as<int>(), (NUI_IMAGE_RESOLUTION)depthResolution.as<int>(), NULL, x1d, y1d, depth, &x1, &y1);
  NuiTransformSkeletonToDepthImage(skeletonData.SkeletonPositions[joint1], &x2d, &y2d, &depth, (NUI_IMAGE_RESOLUTION)depthResolution.as<int>());
  NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution((NUI_IMAGE_RESOLUTION)colorResolution.as<int>(), (NUI_IMAGE_RESOLUTION)depthResolution.as<int>(), NULL, x2d, y2d, depth, &x2, &y2);

  // We assume all drawn bones are inferred unless BOTH joints are tracked
  if (NUI_SKELETON_POSITION_TRACKED == state0 && NUI_SKELETON_POSITION_TRACKED == state1) {
    line(skeletonCVMat, Point(x1, y1), Point(x2, y2), CV_RGB(0, 0, 255), 4);
  } else {
    line(skeletonCVMat, Point(x1, y1), Point(x2, y2), CV_RGB(160, 160, 180), 4);
  }
}

/// <summary>
/// Draw a joint of the skeleton
/// </summary>
/// <param name="skeletonData">Skeleton coordinates</param>
/// <param name="imageRect">The rect which the color or depth image is streched to fit</param>
/// <param name="joint">Index for the joint to be drawn</param>
void UKinect::drawJoint(const NUI_SKELETON_DATA& skeletonData, NUI_SKELETON_POSITION_INDEX joint) {
  NUI_SKELETON_POSITION_TRACKING_STATE state = skeletonData.eSkeletonPositionTrackingState[joint];

  // Not tracked
  if (NUI_SKELETON_POSITION_NOT_TRACKED == state) {
    return;
  }

  LONG x, y, xd, yd;
  USHORT depth;
  NuiTransformSkeletonToDepthImage(skeletonData.SkeletonPositions[joint], &xd, &yd, &depth, (NUI_IMAGE_RESOLUTION)depthResolution.as<int>());
  NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution((NUI_IMAGE_RESOLUTION)colorResolution.as<int>(), (NUI_IMAGE_RESOLUTION)depthResolution.as<int>(), NULL, xd, yd, depth, &x, &y);

  if (NUI_SKELETON_POSITION_TRACKED == state) {
    circle(skeletonCVMat, Point(static_cast<int>(x), static_cast<int>(y)), 7, CV_RGB(0, 200, 0), -1);
  } else {
    circle(skeletonCVMat, Point(static_cast<int>(x), static_cast<int>(y)), 6, CV_RGB(0, 200, 0), 2);
  }

  // draw "L" and "R" informations near shoulders
  if (joint == 4) putText(skeletonCVMat, lexical_cast<string>("L"), Point(static_cast<int>(x), static_cast<int>(y) - 10), 1, 2, CV_RGB(255, 255, 255), 2);
  if (joint == 8) putText(skeletonCVMat, lexical_cast<string>("R"), Point(static_cast<int>(x) - 13, static_cast<int>(y) - 10), 1, 2, CV_RGB(255, 255, 255), 2);
}

/// <summary>
/// Draw a circle to indicate a skeleton of which only position info is available
/// </summary>
/// <param name="skeletonData">Skeleton coordinates</param>
/// <param name="imageRect">The rect which the color or depth stream image is streched to fit</param>
void UKinect::drawOutOfFrame(const NUI_SKELETON_DATA& skeletonData) {
  if (skeletonData.dwQualityFlags&NUI_SKELETON_QUALITY_CLIPPED_RIGHT)
    rectangle(skeletonCVMat, Point(skeletonCVMat.cols - 5, 0), Point(skeletonCVMat.cols - 1, skeletonCVMat.rows - 1), CV_RGB(0, 0, 200), -1);

  if (skeletonData.dwQualityFlags&NUI_SKELETON_QUALITY_CLIPPED_LEFT)
    rectangle(skeletonCVMat, Point(0, 0), Point(4, skeletonCVMat.rows - 1), CV_RGB(0, 0, 200), -1);

  if (skeletonData.dwQualityFlags&NUI_SKELETON_QUALITY_CLIPPED_TOP)
    rectangle(skeletonCVMat, Point(0, 0), Point(skeletonCVMat.cols - 1, 4), CV_RGB(0, 0, 200), -1);

  if (skeletonData.dwQualityFlags&NUI_SKELETON_QUALITY_CLIPPED_BOTTOM)
    rectangle(skeletonCVMat, Point(0, skeletonCVMat.rows - 5), Point(skeletonCVMat.cols - 1, skeletonCVMat.rows - 1), CV_RGB(0, 0, 200), -1);
}


vector<double> UKinect::skeletonPosition(DWORD ID) {
  vector<double> position;

  if (&skeletonFrame != NULL) {
    for (int i = 0; i < NUI_SKELETON_COUNT; i++) {
      const NUI_SKELETON_DATA & skeletonData = skeletonFrame.SkeletonData[i];

      if ((skeletonData.eTrackingState > NUI_SKELETON_NOT_TRACKED) && (skeletonData.dwTrackingID == ID)) {
        position.push_back(static_cast<double>(skeletonData.Position.x));
        position.push_back(static_cast<double>(skeletonData.Position.y));
        position.push_back(static_cast<double>(skeletonData.Position.z));
      }
    }
  }

  return position;
}



vector<double> UKinect::skeletonJointPosition(DWORD ID, int joint) {
  vector<double> position;

  if (&skeletonFrame != NULL) {
    for (int i = 0; i < NUI_SKELETON_COUNT; i++) {
      const NUI_SKELETON_DATA & skeletonData = skeletonFrame.SkeletonData[i];

      if ((skeletonData.eTrackingState == NUI_SKELETON_TRACKED) && (skeletonData.dwTrackingID == ID) && (skeletonData.eSkeletonPositionTrackingState[joint] >= NUI_SKELETON_POSITION_INFERRED)) {
        position.push_back(static_cast<double>(skeletonData.SkeletonPositions[joint].x));
        position.push_back(static_cast<double>(skeletonData.SkeletonPositions[joint].y));
        position.push_back(static_cast<double>(skeletonData.SkeletonPositions[joint].z));
      }
    }
  }

  return position;
}

vector<double> UKinect::skeletonPositionOnImage(DWORD ID) {
  vector<double> position;

  if (&skeletonFrame != NULL) {
    for (int i = 0; i < NUI_SKELETON_COUNT; i++) {
      const NUI_SKELETON_DATA & skeletonData = skeletonFrame.SkeletonData[i];

      if ((skeletonData.dwTrackingID == ID) && (skeletonData.eTrackingState>NUI_SKELETON_NOT_TRACKED)) {
        LONG x, y, xd, yd;
        USHORT depth;
        NuiTransformSkeletonToDepthImage(skeletonData.Position, &xd, &yd, &depth, (NUI_IMAGE_RESOLUTION)depthResolution.as<int>());
        NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution((NUI_IMAGE_RESOLUTION)colorResolution.as<int>(), (NUI_IMAGE_RESOLUTION)depthResolution.as<int>(), NULL, xd, yd, depth, &x, &y);
        position.push_back(x);
        position.push_back(y);
        position.push_back(static_cast<double>(depth));
      }
    }
  }

  return position;
}


vector<double> UKinect::skeletonJointPositionOnImage(DWORD ID, int joint) {
  vector<double> position;

  if (&skeletonFrame != NULL) {
    for (int i = 0; i < NUI_SKELETON_COUNT; i++) {
      const NUI_SKELETON_DATA & skeletonData = skeletonFrame.SkeletonData[i];

      if ((skeletonData.eTrackingState == NUI_SKELETON_TRACKED) && (skeletonData.dwTrackingID == ID) && (skeletonData.eSkeletonPositionTrackingState[joint] >= NUI_SKELETON_POSITION_INFERRED)) {
        LONG x, y, xd, yd;
        USHORT depth;
        NuiTransformSkeletonToDepthImage(skeletonData.SkeletonPositions[joint], &xd, &yd, &depth, (NUI_IMAGE_RESOLUTION)depthResolution.as<int>());
        NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution((NUI_IMAGE_RESOLUTION)colorResolution.as<int>(), (NUI_IMAGE_RESOLUTION)depthResolution.as<int>(), NULL, xd, yd, depth, &x, &y);
        position.push_back(x);
        position.push_back(y);
        position.push_back(static_cast<double>(depth));
      }
    }
  }

  return position;
}



/// <summary>
/// Update tracked skeletons according to current chooser mode
/// </summary>
vector<int> UKinect::UpdateTrackedSkeletons() {
  //NOTE: verify if zero can be used. -1 is more explicit.
  DWORD trackIDs[TrackIDIndexCount] = { 0 };

  if (ChooserModeClosest1 == skeletonChooserMode.as<int>() || ChooserModeClosest2 == skeletonChooserMode.as<int>()) {
    ChooseClosestSkeletons(trackIDs);
  } else if (ChooserModeSticky1 == skeletonChooserMode.as<int>() || ChooserModeSticky2 == skeletonChooserMode.as<int>()) {
    ChooseStickySkeletons(trackIDs);
  } else if (ChooserModeActive1 == skeletonChooserMode.as<int>() || ChooserModeActive2 == skeletonChooserMode.as<int>()) {
    ChooseMostActiveSkeletons(trackIDs);
  }

  if (ChooserModeClosest1 == skeletonChooserMode.as<int>() || ChooserModeSticky1 == skeletonChooserMode.as<int>() || ChooserModeActive1 == skeletonChooserMode.as<int>()) {
    // Track only one player ID. The second ID is not used
    trackIDs[SecondTrackID] = 0;
  }

  sensor->NuiSkeletonSetTrackedSkeletons(trackIDs);

  vector<int> res;
  for (int i = 0; i < TrackIDIndexCount; i++)
    res.push_back((int)trackIDs[i]);
  return res;
}

/// <summary>
/// Find closest skeleton to set tracked
/// </summary>
/// <param name="trackIDs">Array of skeleton tracking IDs</param>
void UKinect::ChooseClosestSkeletons(DWORD trackIDs[TrackIDIndexCount]) {
  ZeroMemory(trackIDs, TrackIDIndexCount * sizeof(DWORD));

  // Initial depth array with max posible value
  USHORT nearestDepth[TrackIDIndexCount] = { NUI_IMAGE_DEPTH_MAXIMUM, NUI_IMAGE_DEPTH_MAXIMUM };

  for (int i = 0; i < NUI_SKELETON_COUNT; i++) {
    if (NUI_SKELETON_NOT_TRACKED != skeletonFrame.SkeletonData[i].eTrackingState) {
      LONG   x, y;
      USHORT depth;

      // Transform skeleton coordinates to depth image
      NuiTransformSkeletonToDepthImage(skeletonFrame.SkeletonData[i].Position, &x, &y, &depth);

      // Compare depth to peviously found item
      if (depth < nearestDepth[FirstTrackID]) {
        // Move depth and track ID in first place to second place and assign with the new closer one
        nearestDepth[SecondTrackID] = nearestDepth[FirstTrackID];
        nearestDepth[FirstTrackID] = depth;

        trackIDs[SecondTrackID] = trackIDs[FirstTrackID];
        trackIDs[FirstTrackID] = skeletonFrame.SkeletonData[i].dwTrackingID;
      } else if (depth < nearestDepth[SecondTrackID]) {
        // Replace old depth and track ID in second place with the newly found closer one
        nearestDepth[SecondTrackID] = depth;
        trackIDs[SecondTrackID] = skeletonFrame.SkeletonData[i].dwTrackingID;
      }
    }
  }
}

/// <summary>
/// Find sticky skeletons to set tracked
/// </summary>
/// <param name="trackIDs">Array of skeleton tracking IDs</param>
void UKinect::ChooseStickySkeletons(DWORD trackIDs[TrackIDIndexCount]) {
  ZeroMemory(trackIDs, TrackIDIndexCount * sizeof(DWORD));

  FindStickyIDs(trackIDs);
  AssignNewStickyIDs(trackIDs);

  // Update stored sticky IDs
  stickyIDs[FirstTrackID] = trackIDs[FirstTrackID];
  stickyIDs[SecondTrackID] = trackIDs[SecondTrackID];
}

/// <summary>
/// Verify if stored tracked IDs are found in new skeleton frame
/// </summary>
/// <param name="trackIDs">Array of skeleton tracking IDs</param>
void UKinect::FindStickyIDs(DWORD trackIDs[TrackIDIndexCount]) {
  for (int i = 0; i < TrackIDIndexCount; i++) {
    for (int j = 0; j < NUI_SKELETON_COUNT; j++) {
      if (NUI_SKELETON_NOT_TRACKED != skeletonFrame.SkeletonData[j].eTrackingState) {
        DWORD trackID = skeletonFrame.SkeletonData[j].dwTrackingID;
        if (trackID == stickyIDs[i]) {
          trackIDs[i] = trackID;
          break;
        }
      }
    }
  }
}

/// <summary>
/// Assign a new ID if old sticky is not found in new skeleton frame
/// </summary>
/// <param name="trackIDs">Array of skeleton tracking IDs</param>
void UKinect::AssignNewStickyIDs(DWORD trackIDs[TrackIDIndexCount]) {
  for (int i = 0; i < NUI_SKELETON_COUNT; i++) {
    if (trackIDs[FirstTrackID] && trackIDs[SecondTrackID]) {
      break;
    }

    if (NUI_SKELETON_NOT_TRACKED != skeletonFrame.SkeletonData[i].eTrackingState) {
      DWORD trackID = skeletonFrame.SkeletonData[i].dwTrackingID;

      if (!trackIDs[FirstTrackID] && trackID != trackIDs[SecondTrackID]) {
        trackIDs[FirstTrackID] = trackID;
      } else if (!trackIDs[SecondTrackID] && trackID != trackIDs[FirstTrackID]) {
        trackIDs[SecondTrackID] = trackID;
      }
    }
  }
}

/// <summary>
/// Find most active skeletons to set tracked
/// </summary>
/// <param name="trackIDs">Array of skeleton tracking IDs</param>
void UKinect::ChooseMostActiveSkeletons(DWORD trackIDs[TrackIDIndexCount]) {
  ZeroMemory(trackIDs, TrackIDIndexCount * sizeof(DWORD));

  // Clear update flag for all active watchers. Delete not updated active watchers later because IDs are not tracked in this pass.
  ResetActivityWatcherFlags();

  // Update watchers's activity levels and update flags
  UpdateActivityWatchers();

  // Delete not updated activity watchers because we've lost the track ID in this pass
  DeleteNonUpdateWatchers();

  // Find out highest activity level IDs
  FindMostActiveIDs(trackIDs);
}

//// <summary>
/// Reset update flags for all activity watchers for further process
/// </summary>
void UKinect::ResetActivityWatcherFlags() {
  for (map<int, NuiActivityWatcher*>::iterator itr = activityWatchers.begin(); itr != activityWatchers.end(); ++itr) {
    itr->second->SetUpdateFlag(false);
  }
}

/// <summary>
/// Update activity watcher items and their activity levels
/// </summary>
void UKinect::UpdateActivityWatchers() {
  for (int i = 0; i < NUI_SKELETON_COUNT; i++) {
    if (NUI_SKELETON_NOT_TRACKED != skeletonFrame.SkeletonData[i].eTrackingState) {
      DWORD id = skeletonFrame.SkeletonData[i].dwTrackingID;
      map<int, NuiActivityWatcher*>::iterator  itr = activityWatchers.find(id);

      if (activityWatchers.end() != itr) {
        // Activity watcher related to this ID is found. Update its activity level
        itr->second->UpdateActivity(skeletonFrame.SkeletonData[i]);
        itr->second->SetUpdateFlag(true);
      } else {
        // No activity watcher related to this ID is found. Create a new one for it
        NuiActivityWatcher* pWatcher = new NuiActivityWatcher(skeletonFrame.SkeletonData[i]);
        pWatcher->SetUpdateFlag(true);

        activityWatchers.insert(std::make_pair(id, pWatcher));
      }
    }
  }
}

/// <summary>
/// Delete non-update activity watchers which has lost track to skeleton from stored collection
/// </summary>
void UKinect::DeleteNonUpdateWatchers() {
  for (map<int, NuiActivityWatcher*>::iterator itr = activityWatchers.begin(); itr != activityWatchers.end();) {
    if (itr->second->GetUpdateFlag()) {
      ++itr;
    } else {
      delete itr->second;
      itr = activityWatchers.erase(itr);
    }
  }
}

/// <summary>
/// Find most active IDs
/// </summary>
/// <param name="trackIDs">Array of skeleton tracking IDs</param>
void UKinect::FindMostActiveIDs(DWORD trackIDs[TrackIDIndexCount]) {
  // Initialize activity levels with negtive valus so they can replaced by any found activity levels
  FLOAT activityLevels[TrackIDIndexCount] = { -1.0f, -1.0f };

  // Run through activity watchers
  for (map<int, NuiActivityWatcher*>::iterator itr = activityWatchers.begin(); itr != activityWatchers.end(); ++itr) {
    // Get calculated activity level
    FLOAT level = itr->second->GetActivityLevel();

    // Compare to previously found activity levels
    if (level > activityLevels[FirstTrackID]) {
      // Move first track ID and activity level to second place. Assign newly found higher activity level and ID to first place
      activityLevels[SecondTrackID] = activityLevels[FirstTrackID];
      activityLevels[FirstTrackID] = level;

      trackIDs[SecondTrackID] = trackIDs[FirstTrackID];
      trackIDs[FirstTrackID] = itr->first;
    } else if (level > activityLevels[SecondTrackID]) {
      // Replace the previous one
      activityLevels[SecondTrackID] = level;
      trackIDs[SecondTrackID] = itr->first;
    }
  }
}
