// George Terzakis 2016
//
// University of Portsmouth
//
// Code based on PTAM by Klein and Murray (Copyright 2008 Isis Innovation Limited)

//
// This header declares the data structures to do with keyframes:
// structs KeyFrame, Level, Measurement, Candidate.
//
// A KeyFrame contains an image pyramid stored as array of Level;
// A KeyFrame also has associated map-point mesurements stored as a vector of Measurment;
// Each individual Level contains an image, corner points, and special corner points
// which are promoted to Candidate status (the mapmaker tries to make new map points from those.)
//
// KeyFrames are stored in the Map class and manipulated by the MapMaker.
// However, the tracker also stores its current frame as a half-populated
// KeyFrame struct.

#ifndef __KEYFRAME_H
#define __KEYFRAME_H

#include "GCVD/SE3.h"

#include "OpenCV.h"

#include <map>
#include <memory>
#include <set>
#include <vector>

using namespace RigidTransforms;

struct MapPoint;
class SmallBlurryImage;

#define LEVELS 4

// Candidate: a feature in an image which could be made into a map point
struct Candidate {
    cv::Point2i irLevelPos;
    cv::Vec2f v2RootPos;
    double dSTScore;
};

// Measurement: A 2D image measurement of a map point. Each keyframe stores a bunch of these.
struct KFMeasurement {
    int nLevel;    // Which image level?
    bool bSubPix;  // Has this measurement been refined to sub-pixel level?
    cv::Vec<float, 2>
        v2RootPos;  // Position of the measurement, REFERED TO PYRAMID LEVEL ZERO
    enum {
        SRC_TRACKER,  // .... dunnow yet... :(
        SRC_REFIND,   // relfound
        SRC_ROOT,     // Found with feature detection
        SRC_TRAIL,    // Found in the second (base) initialization image
        SRC_EPIPOLAR  // found with epipolar search

    } Source;  // Where has this measurement come frome?
};

// Each keyframe is made of LEVELS pyramid levels, stored in struct Level.
// This contains image data and corner points.
struct Level {
    inline Level() { bImplaneCornersCached = false; };

    cv::Mat_<uchar> im;                 // The pyramid level pixels
    std::vector<cv::Point2i> vCorners;  // All FAST corners on this level
    std::vector<int>
        vCornerRowLUT;  // Row-index into the FAST corners, speeds up access
    std::vector<cv::Point2i> vMaxCorners;  // The maximal FAST corners

    Level& operator=(const Level& rhs);

    std::vector<Candidate>
        vCandidates;  // Potential locations of new map points

    bool
        bImplaneCornersCached;  // Also keep image-plane (z=1) positions of FAST corners to speed up epipolar search
    std::vector<cv::Vec2f>
        vImplaneCorners;  // Corner points un-projected into z=1-plane coordinates
};

// The actual KeyFrame struct. The map contains of a bunch of these. However, the tracker uses this
// struct as well: every incoming frame is turned into a keyframe before tracking; most of these
// are then simply discarded, but sometimes they're then just added to the map.
struct KeyFrame {
   public:
    // Need the nickname for shared_ptr
    typedef std::shared_ptr<KeyFrame> Ptr;

    inline KeyFrame() {
        pSBI = NULL;
        bFixed =
            false;  // The tracker and mamaker explicitly fix the KF. So I dont think this will hurt being here...
    }
    SE3<>
        se3CfromW;  // The coordinate frame of this key-frame as a Camera-From-World transformation
    bool
        bFixed;  // Is the coordinate frame of this keyframe fixed? (only true for first KF!)
    Level aLevels
        [LEVELS];  // Images, corners, etc lives in this array of pyramid levels
    std::map<std::shared_ptr<MapPoint>, KFMeasurement>
        mMeasurements;  // All the measurements associated with the keyframe

    void MakeKeyFrame_Lite(
        cv::Mat_<uchar>&
            im);  // This takes an image and calculates pyramid levels etc to fill the
        // keyframe data structures with everything that's needed by the tracker..
    void
    MakeKeyFrame_Rest();  // ... while this calculates the rest of the data which the mapmaker needs.

    double dSceneDepthMean;  // Hacky heuristics to improve epipolar search.
    double dSceneDepthSigma;

    SmallBlurryImage* pSBI;  // The relocaliser uses this
};

typedef std::map<std::shared_ptr<MapPoint>, KFMeasurement>::iterator
    meas_it;  // For convenience, and to work around an emacs paren-matching bug

#endif
