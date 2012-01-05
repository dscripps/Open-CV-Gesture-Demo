//
//  Gestures.h
//
//  Created by David Scripps on 1/5/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <OpenCV/OpenCV.h>

#ifndef gesture_demo_Gestures_h
#define gesture_demo_Gestures_h


struct gestureSettings {
    //threshold for filtering differences
    int threshold;
    //number of "slices" to find gestures
    int numSlices;
    //body segments greater than this are assumed to be the torso
    float torsoCoefficient;
    //arms must be at least this much of torso
    float armToTorsoRatio;
    //scale to shrink when processing
    float scale;
    float sumRequiredForMovement;
};

struct gestureBody {
    int leftArmStart;//x slice position of left arm in image
    int torsoStart;//x slice position of torso
    int rightArmStart;//x slice position of start of right arm
    int bodyEnd;
    int pixelsPerSlice;//pixel width of 1 slice
    bool isBody;//true if struct is indeed a body
};

//supported gestures
struct gestures {
    bool leftArmUp;
    bool rightArmUp;
    bool isGesture;//true if struct is indeed a gesture
};

//results from compiling differences in images
struct gestureResults {
    bool isMoving;
    gestureBody* bodyResult;
    gestures* gestureResult;
    IplImage *differenceImage;//to send back to caller for display
};


class GestureDetect {
    gestureSettings *curSettings;
    
    
    // image data structures
    IplImage *background;
    IplImage *currentFrame;
    IplImage *previousFrame;
    IplImage *display;
    IplImage *smallImage;
    
    // grayscale buffers
    IplImage *backgroundGrayBig;
    IplImage *backgroundGray;
    IplImage *currentFrameGrayBig;
    IplImage *currentFrameGray;
    IplImage *previousFrameGrayBig;
    IplImage *previousFrameGray;
    IplImage *differenceBackgroundGray;
    IplImage *differenceLastFrameGray;
    
    //other stuff
    int frameSize;
    float sumRequiredForMovement;
    
    //results of processing
    gestureResults* generatedResults;
    
    
    //private functions
    bool objectMoving(IplImage* src);
    int* horizontalIntensity (IplImage* src);
    gestureBody* generateBody(IplImage* src);
    gestures* generateGestures();
    
    public:
        void drawBodyPositions(IplImage* output);
        void drawGestures(IplImage* output);
        gestureResults* generateResults(IplImage *curFrame, IplImage *prevFrame);
        GestureDetect(gestureSettings *set, IplImage *initialImage);

};

#endif
