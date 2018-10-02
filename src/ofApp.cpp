#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

	squareImgSize = 512;
	isPlotting = false;
	camera.setDeviceID(1);
	camera.setDesiredFrameRate(30);
	camera.initGrabber(1280, 720);

	xyplotter.setup(0, 115200);
	xyplotter.listDevices();
	if (xyplotter.isInitialized()) {
		cout << "XYPlotter initialized" << endl;
	}

	doNextStep = false;
	penDown = false;
	penIsDown = false;
	penDownPos = 90;
	penUpPos= 130;

	//img.allocate(squareImgSize, squareImgSize, OF_IMAGE_GRAYSCALE);
	//tracedImg.allocate(squareImgSize, squareImgSize, OF_IMAGE_GRAYSCALE);
	tracedImgPxl.allocate(squareImgSize, squareImgSize, OF_IMAGE_GRAYSCALE);

	//img.load("test_71.jpg");
	
	neighbours.push_back(-squareImgSize);
	neighbours.push_back(-1);
	neighbours.push_back(1);
	neighbours.push_back(squareImgSize);
	neighbours.push_back(-squareImgSize - 1);
	neighbours.push_back(-squareImgSize + 1);
	neighbours.push_back(squareImgSize - 1);
	neighbours.push_back(squareImgSize + 1);

	cvColImg.allocate(squareImgSize, squareImgSize);
	cvGreyImg.allocate(squareImgSize, squareImgSize);
	cvCannyImg.allocate(squareImgSize, squareImgSize);

	gui.setup();
	gui.add(cannyMin.setup("Canny Min", 40, 0, 400));
	gui.add(cannyMax.setup("Canny Max", 400, 0, 400));
	gui.add(step.setup("Stepsize", 25, 1, 75));
	gui.add(freeze.setup("Freeze", false));
	gui.add(debug.setup("Debug", false));
	gui.add(size.setup("Size", 200, 1, 600));
	gui.add(saveBtn.setup("Save canny image"));
	gui.add(nextStep.setup("force next step"));
	gui.add(plotBtn.setup("Plot image"));

	gui.loadFromFile("settings.xml");

	plotBtn.addListener(this, &ofApp::plotImage);
	saveBtn.addListener(this, &ofApp::saveImage);
	nextStep.addListener(this, &ofApp::forceNextStep);
}



//--------------------------------------------------------------
// If a black pixel somewhere is detected, it's most likely that it's somewhere in the middle of a line.
// We need to find the end of the line first and then trace the whole line back.
// This recursive function is looking for the end of a line and then stores the pixelIndex into startingPoints

bool ofApp::findEndOfTrace(int pixelIndex) {
	if (tracedImgPxl[pixelIndex] < 127) {		// is it a black pixel?
		tracedImgPxl[pixelIndex] = 128;			// change pixel value so we don't find it again
		bool endOfStroke = true;					// pretend it's the last black pixel of the line
		for (int j = 0; j < neighbours.size(); j++) {						// check all 8 neighbour pixels
			int _pixelIndex = pixelIndex + neighbours.at(j);				// calculate index of neighbour pixel
			if (_pixelIndex >= 0 && _pixelIndex < tracedImgPxl.size()-1) {	// make sure it's a valid index
				if (findEndOfTrace(_pixelIndex)) {		// check if there are any black neighbour pixels
					endOfStroke = false;					// there is a neighbour pixel, so it's NOT the end
				}
			}
		}
		if (endOfStroke) {								// if there is no neighbour-pixel, it's the end of the stroke
			startingPoints.push_back(pixelIndex);		// store the index
		}
		return true;		// this was a black pixel
	}
	return false;			// this is not a black pixel
}


//--------------------------------------------------------------
void ofApp::tracePixels(int pixelIndex, int depth) {
	if (tracedImgPxl[pixelIndex] == 128) {		// thats the color value we set the pixels to in the findEndOfTrace function
		if (depth % step == 0) {				// only store every n-th pixel
			blackPixels.push_back(pixelIndex);
			blackPixelTraceLength.push_back(depth/step);
		}

		tracedImgPxl[pixelIndex] = 150;					// change pixel value so we don't find it again
		for (int j = 0; j < neighbours.size(); j++) {
			int _pixelIndex = pixelIndex + neighbours.at(j);
			if (_pixelIndex >= 0 && _pixelIndex < tracedImgPxl.size()) {
				tracePixels(_pixelIndex, depth + 1);
			}
		}
	}
}


//--------------------------------------------------------------
void ofApp::saveImage() {
	ofImage s_img;
	s_img.allocate(squareImgSize, squareImgSize, OF_IMAGE_GRAYSCALE);
	s_img.setFromPixels(cvCannyImg.getPixels());
	string i = std::to_string(ofGetFrameNum());
	s_img.saveImage("test_" + i + ".jpg");
}


//--------------------------------------------------------------
void ofApp::forceNextStep() {
	doNextStep = true;
}


//--------------------------------------------------------------
void ofApp::plotImage() {
	isPlotting = true;
	if (!isPlotting) return;

	cout << "starting to plot" << endl;

	if (!xyplotter.isInitialized()) {
		xyplotter.setup(0, 115200);
	}

	stringstream ss;
	ss << "G28\n";
	ss << "M3 -5\n";
	xyplotter.writeBytes(ss);
	cout << "going home" << endl;

	freeze = true;
	// there are many useless points from the noise. so:
	// 1) remove single plot points
	vector<int> toBeDeleted;
	for (int j = 0; j < blackPixels.size() - 1; j++) {
		if (blackPixelTraceLength.at(j) == 0 && blackPixelTraceLength.at(j + 1) == 0) {
			toBeDeleted.push_back(j);
		}
	}
	for (int i = blackPixels.size(); i >= 0; i--) {
		for (int j = 0; j < toBeDeleted.size(); j++) {
			if (i == toBeDeleted.at(j)) {
				blackPixels.erase(blackPixels.begin() + i);
				blackPixelTraceLength.erase(blackPixelTraceLength.begin() + i);
			}
		}
	}
	// 2) remove single starting points
	toBeDeleted.clear();
	for (int i = 0; i < startingPoints.size(); i++) {
		bool addToKill = true;
		for (int j = 0; j < blackPixels.size(); j++) {
			if (startingPoints.at(i) == blackPixels.at(j)) {
				addToKill = false;
			}
		}
		if (addToKill) toBeDeleted.push_back(i);
	}
	for (int i = startingPoints.size(); i >= 0; i--) {
		for (int j = 0; j < toBeDeleted.size(); j++) {
			if (i == toBeDeleted.at(j)) {
				startingPoints.erase(startingPoints.begin() + i);
			}
		}
	}

	plotIterator = 0;
}


//--------------------------------------------------------------
void ofApp::update(){
	// Get Camera Updates
	if (!freeze) {
		camera.update();
		cvColImg = camera.getPixels();
		cvColImg.resize(squareImgSize * camera.getWidth() / camera.getHeight(), squareImgSize);
		cvColImg.setROI((cvColImg.width - squareImgSize) / 2, 0, squareImgSize, squareImgSize);
		cvColImg.convertToGrayscalePlanarImage(cvGreyImg, 0);
	}


	// actual tracing
	if (!isPlotting) {
		cvCanny(cvGreyImg.getCvImage(), cvCannyImg.getCvImage(), cannyMin, cannyMax, 3);
		cvCannyImg.invert();
		cvCannyImg.flagImageChanged();

		// 1) copy img to tracedImgPxl
		for (int i = 0; i < tracedImgPxl.size(); i++) {
			//tracedImgPxl[i] = (int)img.getPixels()[i];
			tracedImgPxl[i] = cvCannyImg.getPixels()[i];
		}

		// 2) find all the ends of the lines
		startingPoints.clear();
		for (int i = 0; i < tracedImgPxl.size(); i++) {
			findEndOfTrace(i);
		}

		// 3) now take the ends and trace back
		blackPixels.clear();
		blackPixelTraceLength.clear();
		for (int i = 0; i < startingPoints.size(); i++) {
			tracePixels(startingPoints.at(i));
		}
		//tracedImg.setFromPixels(tracedImgPxl);
	}


	// Check if there is a "OK"-message from the plotter to make the next step
	// (every time the plotter arrived at it's position, it sends "OK")
	if (xyplotter.available()) {
		int b = xyplotter.readByte();
		plotterMsg << char(b);
		if (plotterMsg.str() == "OK\r\n") {
			plotterMsg.str("");
			if (isPlotting) {
				cout << "nextstep" << endl;
				doNextStep = true;
			}
		}
	}

	// Plot!
	if (isPlotting && doNextStep) {
		doNextStep = false;

		stringstream gcode;
		if (!penIsDown && penDown) {
			gcode << "M1 " << penDownPos << "\n";
			plotIterator++;
			penIsDown = true;
		}
		else if (penIsDown && !penDown) {
			gcode << "M1 " << penUpPos << "\n";
			plotIterator++;
			penIsDown = false;
		}
		else if (plotIterator < blackPixels.size()-1	) {
			px = size -( float(blackPixels.at(plotIterator) % squareImgSize) / squareImgSize * size);
			py = float(blackPixels.at(plotIterator) / squareImgSize) / squareImgSize * size;

			gcode << "G1 X" << px << " Y" << py << "\n";

			if (blackPixelTraceLength.at(plotIterator) == 0) {
				penDown = true;
			}
			else if (blackPixelTraceLength.at(plotIterator + 1) == 0) {
				penDown = false;
			}
			else {
				plotIterator++;
			}
		}
		else if (plotIterator == blackPixels.size()-1) {
			cout << "finished plotting" << endl;
			gcode << "M1 " << penUpPos << "\n";
			penIsDown = false;
			isPlotting = false;
		}
		cout << gcode.str();
		xyplotter.writeBytes(gcode);
	}


	// display framerate in window title
	stringstream fr;
	fr << ofGetFrameRate();
	ofSetWindowTitle(fr.str());
}




//--------------------------------------------------------------
void ofApp::draw(){
	ofBackground(255);
	ofSetColor(255);

	//cvGreyImg.draw(0, 0);
	//cvCannyImg.draw(squareImgSize, 0);

	gui.draw();

	if (blackPixels.size() > 0) {
		for (int i = 0; i < blackPixels.size() - 1; i++) {
			if (blackPixelTraceLength.at(i + 1) > 0) {
				int x = float(blackPixels.at(i) % squareImgSize) / squareImgSize * ofGetHeight();
				int y = float(blackPixels.at(i) / squareImgSize) / squareImgSize * ofGetHeight();
				int nx = float(blackPixels.at(i + 1) % squareImgSize) / squareImgSize * ofGetHeight();
				int ny = float(blackPixels.at(i + 1) / squareImgSize) / squareImgSize * ofGetHeight();
				ofNoFill();
				ofSetColor(0, 0, 0, 200);
				ofLine(x, y, nx, ny);
			}
		}
	}

	if (debug) {
		for (int i = 0; i < startingPoints.size(); i++) {
			int x = float(startingPoints.at(i) % squareImgSize) / squareImgSize * ofGetHeight();
			int y = float(startingPoints.at(i) / squareImgSize) / squareImgSize * ofGetHeight();
			ofNoFill();
			ofSetColor(0, 255, 0);
			ofEllipse(x, y, 13, 13);
		}
		for (int i = 0; i < blackPixels.size() - 1; i++) {
			int x = float(blackPixels.at(i) % squareImgSize) / squareImgSize * ofGetHeight();
			int y = float(blackPixels.at(i) / squareImgSize) / squareImgSize * ofGetHeight();
			ofFill();
			ofSetColor(255, 0, 0, 55);
			ofEllipse(x, y, 12, 12);
		}
	}

	if (isPlotting) {
		int x = float(blackPixels.at(plotIterator) % squareImgSize) / squareImgSize * ofGetHeight();
		int y = float(blackPixels.at(plotIterator) / squareImgSize) / squareImgSize * ofGetHeight();
		ofFill();
		ofSetColor(0, 0, 255);
		ofEllipse(x, y, 20, 20);
	}

/*
	std::stringstream ss;
	ss << "x: " << std::to_string(mouseX / size) << endl
		<< "y: " << std::to_string(mouseY / size) << endl
		<< "index: " << std::to_string(mouseX / size + mouseY / size * squareImgSize);
	ofSetColor(0);
	ofDrawBitmapString(ss.str(), mouseX + 30, mouseY + 10);
*/
}
