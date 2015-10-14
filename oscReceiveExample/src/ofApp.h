#pragma once

#include "ofMain.h"
#include "ofxOsc.h"
#include "ofxGui.h"


class ofApp : public ofBaseApp {
public:
    
    void setup();
    void update();
    void draw();
    
    ofParameter<float> scale;
    ofParameter<bool> drawTrackPoints;
    ofParameter<bool> drawMesh;
    ofParameterGroup viewParams;
    
    ofParameter<string> oscPort;
    ofParameterGroup oscParameters;
    
    ofxPanel panel;
    
    map<int, ofVec2f> points;
    ofxOscReceiver receiver;

    ofEasyCam cam;
    ofMesh mesh;
};