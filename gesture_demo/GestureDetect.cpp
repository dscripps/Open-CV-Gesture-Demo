//
//  Gestures.cpp
//
//  Created by David Scripps on 1/5/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//


#include <OpenCV/OpenCV.h>
#include "GestureDetect.h"

//constructor
GestureDetect::GestureDetect(gestureSettings *set, IplImage *initialImage) {
    curSettings = set;
    background = cvCloneImage(initialImage);
    previousFrame = background;
    
    //for scaled-down images
    int smallWidth = background->width * curSettings->scale;
    int smallHeight = background->height * curSettings->scale;
    
    //initialize the intermediary images we will use for processing
    backgroundGrayBig = cvCreateImage( cvGetSize( background ), IPL_DEPTH_8U, 1);
    backgroundGray = cvCreateImage(cvSize (smallWidth, smallHeight), IPL_DEPTH_8U, 1);
    currentFrameGrayBig = cvCreateImage( cvGetSize( background ), IPL_DEPTH_8U, 1);
    currentFrameGray = cvCreateImage(cvSize (smallWidth, smallHeight), IPL_DEPTH_8U, 1);
    previousFrameGrayBig = cvCreateImage( cvGetSize( background ), IPL_DEPTH_8U, 1);
    previousFrameGray = cvCreateImage(cvSize (smallWidth, smallHeight), IPL_DEPTH_8U, 1);
    differenceBackgroundGray = cvCreateImage(cvSize (smallWidth, smallHeight), IPL_DEPTH_8U, 1);
    differenceLastFrameGray = cvCreateImage(cvSize (smallWidth, smallHeight), IPL_DEPTH_8U, 1);
    
    
    //convert background to grayscale
    cvCvtColor( background, backgroundGrayBig, CV_RGB2GRAY );
    cvResize (backgroundGrayBig, backgroundGray, CV_INTER_LINEAR);
    
    //flip around
    cvFlip (backgroundGray, backgroundGray, 1);
    
    
    //minimum sum of white pixels required to determine a person is present
    frameSize = backgroundGray->width*backgroundGray->height;
    sumRequiredForMovement = curSettings->sumRequiredForMovement * (float)frameSize/921600;
    
    //initialize our body structures
    generatedResults = new gestureResults;
    
}

//returns true if image has # of non-black pixels above a threshold
bool GestureDetect::objectMoving(IplImage* src) {
    int Cr = 0;
    int w = 0;
    int h = 0;
    
    int sum = 0;
    w = src->width, h = src->height;
    
    for (int y = 0; y < h ; y++) {
        for (int x = 0; x < w ; x++) {
            Cr= (int)((unsigned char*)(src->imageData + src->widthStep*(y)))[x];
            if(Cr > 1) {
                sum = sum + 1;
                if(sum > sumRequiredForMovement) {
                    //we have moving object, done
                    return true;
                }
            }
        }
    }
    return false;
}


//returns array of intensities of length "numslices"
//each element in array is sum of non-black pixels in that area
int* GestureDetect::horizontalIntensity (IplImage* src) {
    int Cr = 0;
    int height = src->height;
    int sum = 0;
    
    int *intensities = new int[this->curSettings->numSlices];
    
    //divide screen into vertical "slices", get sum of pixels for each slice
    int pixelsPerSlice = src->width / this->curSettings->numSlices;
    
    for(int slice = 0; slice < this->curSettings->numSlices; slice++) {
        sum = 0;
        int firstX = slice * pixelsPerSlice;
        int lastX = firstX + pixelsPerSlice;
        for(int x = firstX; x < lastX; x++) {
            for (int y = 0; y < height ; y++) {
                Cr= (int)((unsigned char*)(src->imageData + src->widthStep*(y)))[x];
                if(Cr > 1) {//non-black pixel
                    sum = sum + 1;
                }
            }
        }
        intensities[slice] = sum;
    }
    
    
    return intensities;
}

//scan image to find body dimensions
gestureBody* GestureDetect::generateBody(IplImage* src) {
    
    //initialize body struct to return
    gestureBody *currentBody = new gestureBody;
    currentBody->leftArmStart = -1;
    currentBody->torsoStart = -1;
    currentBody->rightArmStart = -1;
    currentBody->bodyEnd = -1;
    currentBody->pixelsPerSlice = -1;
    currentBody->isBody = false;
    
    //get white pixel intensities (sum of white pixels) for each rectangular slice of src image
    int *intensities = new int[this->curSettings->numSlices];
    intensities = horizontalIntensity(src);
    
    int minSum = 200 * src->width / 1280;//need at least this many white pixels to count as part of body
    
    int longestStartPosition = -1;
    int currentStartPosition = -1;
    int longestEndPosition = -1;
    int currentEndPosition = -1;
    int longestSegmentLength = 0;
    int currentSegmentLength = 0;
    int currentTotal = 0;
    int longestTotal = 0;
    
    //get body--- the longest continuous set of segments
    for(int slice = 0; slice < this->curSettings->numSlices; slice++) {
        if(intensities[slice] > minSum) {
            currentSegmentLength = currentSegmentLength + 1;
            currentTotal = currentTotal + intensities[slice];
            if(currentStartPosition < 0) {
                //first segment in a continuation
                currentStartPosition = slice;
            }
            currentEndPosition = slice;
            if(currentSegmentLength > longestSegmentLength) {
                //this is the longest continuous segment, must be body
                longestSegmentLength = currentSegmentLength;
                if(longestStartPosition < 0) {
                    longestStartPosition = currentStartPosition;
                }
                longestEndPosition = currentEndPosition;
                longestTotal = currentTotal;
            }
        } else {
            currentSegmentLength = 0;
            currentStartPosition = -1;
            currentEndPosition = -1;
            currentTotal = 0;
        }
    }
    
    if(longestStartPosition > -1 && longestEndPosition > -1) {
        //have a body!
        
        //find the start and end of the torso
        //to do this, I will assume the segment with the most non-black pixels is likely the torso
        int brightestSlice = -1;
        int brightestValue = 0;
        for(int slice = longestStartPosition; slice <= longestEndPosition; slice++) {
            if(intensities[slice] > brightestValue) {
                //this is the brightest position thus far
                brightestSlice = slice;
                brightestValue = intensities[slice];
            }
        }
        currentBody->torsoStart = brightestSlice;
        currentBody->rightArmStart = brightestSlice;
        if(brightestSlice > -1) {
            //starting at brightest position, go left until brightness falls below threshold to get start of torso
            for(int slice = brightestSlice; slice >= longestStartPosition; slice--) {
                if(intensities[slice] > (brightestValue * this->curSettings->torsoCoefficient)) {
                    //this is bright enough to be considered part of the torso
                    currentBody->torsoStart = slice;
                } else {
                    break;
                }
            }
            //...and right to get the end of torso
            for(int slice = brightestSlice; slice <= longestEndPosition; slice++) {
                if(intensities[slice] > (brightestValue * this->curSettings->torsoCoefficient)) {
                    //this is bright enough to be considered part of the torso
                    currentBody->rightArmStart = slice;
                } else {
                    break;
                }
            }
            
        }
        
        currentBody->leftArmStart = longestStartPosition;
        currentBody->bodyEnd = longestEndPosition;
        currentBody->pixelsPerSlice = src->width / this->curSettings->numSlices;
        currentBody->isBody = (
            currentBody->leftArmStart > -1 
            && currentBody->torsoStart > -1 
            && currentBody->rightArmStart > -1
            && currentBody->bodyEnd > -1 
            && currentBody->pixelsPerSlice > -1
        );
    }    
    
    return currentBody;
}

gestures* GestureDetect::generateGestures() {
    gestureBody* currentBody = generatedResults->bodyResult;
    gestures *currentGestures = new gestures;
    currentGestures->leftArmUp = false;
    currentGestures->rightArmUp = false;
    currentGestures->isGesture = false;
    
    if(currentBody->isBody) {
        
        int torsoWidth = currentBody->rightArmStart - currentBody->torsoStart;
        if(torsoWidth <= 0) {
            return currentGestures;
        }
        int leftArmWidth = currentBody->torsoStart - currentBody->leftArmStart;
        int rightArmWidth = currentBody->bodyEnd - currentBody->rightArmStart;
        
        //left arm is "up" if it is a certain percentage of body
        float leftArmToTorsoRatio = (float)leftArmWidth / (float)torsoWidth;
        currentGestures->leftArmUp = (leftArmToTorsoRatio > this->curSettings->armToTorsoRatio);
        //ditto for right arm
        float rightArmToTorsoRatio = (float)rightArmWidth / (float)torsoWidth;
        currentGestures->rightArmUp = (rightArmToTorsoRatio > this->curSettings->armToTorsoRatio);        
        
        currentGestures->isGesture = true;
        
    }
    
    return currentGestures;
}

//show lines relating to left arm, torso, and right arm (good for debugging)
void GestureDetect::drawBodyPositions(IplImage* output) {
    if(generatedResults->bodyResult->isBody) {
        int pixelsPerSlice = generatedResults->bodyResult->pixelsPerSlice;
        //whole body
        cvLine (
            output, 
            cvPoint(generatedResults->bodyResult->leftArmStart*pixelsPerSlice, 20), 
            cvPoint(
                generatedResults->bodyResult->bodyEnd*pixelsPerSlice+pixelsPerSlice,
                20
            ), 
            CV_RGB(255,255,255), 
            5
        );
        //torso
        cvLine (
            output, 
            cvPoint(generatedResults->bodyResult->torsoStart*pixelsPerSlice, 30), 
            cvPoint(generatedResults->bodyResult->rightArmStart*pixelsPerSlice+pixelsPerSlice, 30), 
            CV_RGB(255,255,255), 
            5
        );
    }
    
    
}

//draw circles for up/down of left/right arms
void GestureDetect::drawGestures(IplImage* output) {
    if(generatedResults->gestureResult->isGesture) {
        CvPoint center;
        center.x = 100;
        if(generatedResults->gestureResult->leftArmUp) {
            center.y = 100;
        } else {
            center.y = 130;
        }
        //left arm circle
        cvCircle (output, center, 10, CV_RGB(255,255,255), 3, 8, 0 );
        center.x = 130;
        if(generatedResults->gestureResult->rightArmUp) {
            center.y = 100;
        } else {
            center.y = 130;
        }
        //right arm circle
        cvCircle (output, center, 10, CV_RGB(255,255,255), 3, 8, 0 );
        cvLine (output, cvPoint(80, 115), cvPoint(150, 115), CV_RGB(255,255,255), 1);
    }
}

//generate moving body location from current camera frame
gestureResults* GestureDetect::generateResults(IplImage *curFrame, IplImage *prevFrame) {
    currentFrame = curFrame;
    previousFrame = prevFrame;//for detecting movement (no movement=skip processing)
    
    //initialize results
    generatedResults = new gestureResults;
    generatedResults->isMoving = false;
    
    //chage frames for processing
    //flip around
    cvFlip (currentFrame, currentFrame, 1);
    // convert rgb to grayscale
    cvCvtColor( currentFrame, currentFrameGrayBig, CV_RGB2GRAY );
    //shrink
    cvResize (currentFrameGrayBig, currentFrameGray, CV_INTER_LINEAR);
    
    //do same for previous frame
    //flip around
    cvFlip (previousFrame, previousFrame, 1);
    // convert rgb to grayscale
    cvCvtColor( previousFrame, previousFrameGrayBig, CV_RGB2GRAY );
    //shrink
    cvResize (previousFrameGrayBig, previousFrameGray, CV_INTER_LINEAR);
    
    // compute difference between last frame and current frame
    cvAbsDiff( previousFrameGray, currentFrameGray, differenceLastFrameGray );
    
    
    
    //highlight big changes
    cvThreshold(differenceLastFrameGray, differenceLastFrameGray, curSettings->threshold, 255, CV_THRESH_BINARY);
    
    
    if(this->objectMoving(differenceLastFrameGray)) {
        //continue processing
        // compute difference between background and current frame
        cvAbsDiff( backgroundGray, currentFrameGray, differenceBackgroundGray );
        
        //highlight big changes
        cvThreshold(differenceBackgroundGray, differenceBackgroundGray, curSettings->threshold, 255, CV_THRESH_BINARY);
        
        //generate a body
        generatedResults->bodyResult = this->generateBody(differenceBackgroundGray);
        
        //generate gestures
        generatedResults->gestureResult = this->generateGestures();
        
    }
    
    
    generatedResults->differenceImage = differenceBackgroundGray;//or whichever image we want to feed back to the screen
    
    return generatedResults;
}

