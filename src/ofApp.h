#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxGui.h"


class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		bool findEndOfTrace(int pixelIndex);
		void tracePixels(int pixelIndex, int depth = 0);
		void plotImage();
		void saveImage();
		void forceNextStep();

		vector<int> blackPixels;	// all black pixels
		vector<int> blackPixelTraceLength;
		vector<int> startingPoints;
		vector<int> neighbours;	// list with all neighbor pixels' positionis
	

		//ofImage img;
		//ofImage tracedImg;
		ofPixels tracedImgPxl;
		ofVideoGrabber camera;

		ofxCvColorImage cvColImg;		// camera image
		ofxCvGrayscaleImage cvGreyImg;	// cropped, resized and monochrome camera picture
		ofxCvGrayscaleImage cvCannyImg;	// edge detected image with same size as cvGreyImg

		int squareImgSize; 

		ofxIntSlider cannyMin, cannyMax, step;
		ofxFloatSlider size;
		ofxToggle freeze, debug;
		ofxButton saveBtn, plotBtn, nextStep;
		ofxPanel gui;

		bool isPlotting;
		int plotIterator;
		int px, py;

		ofSerial xyplotter;
		bool doNextStep;
		bool penDown, penIsDown;
		int penDownPos, penUpPos;
		stringstream plotterMsg;
};