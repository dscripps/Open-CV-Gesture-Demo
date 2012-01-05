//
//  gesture_demo.cpp
//
//  Created by David Scripps on 1/5/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <OpenCV/OpenCV.h>
#include "GestureDetect.h"

//entry point
int main (int argc, char * const argv[]) 
{
    //settings
    gestureSettings *mainSettings = new gestureSettings;
    //threshold for filtering differences
    mainSettings->threshold = 80;
    //number of "slices" to find gestures
    mainSettings->numSlices = 20;
    //body segments greater than this are assumed to be the torso
    mainSettings->torsoCoefficient = 0.4f;
    //arms must be at least this much of torso
    mainSettings->armToTorsoRatio = 0.4f;
    //scale to shrink when processing
    mainSettings->scale = 0.5;
    mainSettings->sumRequiredForMovement = 1000;//min value to determine something is moving in image
    
    
    // use first camera attached to computer
    CvCapture *capture;
    capture = cvCaptureFromCAM( 0 );
    assert( capture );
    
    CvMemStorage* storage = cvCreateMemStorage(0);
    assert (storage);
    
    
    // image data structures
    IplImage *background;
    IplImage *currentFrame;
    IplImage *previousFrame;
    IplImage *display;
   
    //load the background image first
    background =  cvQueryFrame( capture );
    if( !background ) return -1;
    previousFrame = background;
    
    GestureDetect gestureDetect (mainSettings, background);
    int key = 0;
    while ( key != 'q' ) {
        
        // load image from camera
        currentFrame = cvQueryFrame( capture );	
        
        //all the magic is inside gestureDetect
        gestureResults *generatedReults = gestureDetect.generateResults(currentFrame, previousFrame);
        display = generatedReults->differenceImage;

        //draw body positions
        gestureDetect.drawBodyPositions(display);
        //show circles for each gesture
        gestureDetect.drawGestures(display);
        
        //use this frame as the previous one for next time
        cvCopy(currentFrame, previousFrame);
        
        //output to selected window
        cvNamedWindow( "display", 1 );
        cvShowImage( "display", display );
         
        // quit if user press 'q' and wait a bit between images
        key = cvWaitKey( 50 );

    }
    
    
    // release camera and clean up resources when "q" is pressed
    cvReleaseCapture( &capture );
    cvDestroyWindow( "video" );
    
    return 0;
}











