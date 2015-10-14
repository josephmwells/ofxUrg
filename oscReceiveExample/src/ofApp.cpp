#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    viewParams.setName("View");
    viewParams.add(scale.set("Zoom", 0.05, 0.05, 1));
    viewParams.add(drawTrackPoints.set("Draw Tracker Points", true));
    viewParams.add(drawMesh.set("Draw Point Cloud", true));
    oscParameters.setName("OSC");
    oscParameters.add(oscPort.set("OSC Port", "7777"));
    panel.setup();
    panel.add(viewParams);
    panel.add(oscParameters);
    panel.loadFromFile("settimgs.xml");
    
    receiver.setup(ofToInt(oscPort));
    
}

//--------------------------------------------------------------
void ofApp::update(){
    
    // check for waiting messages
    while(receiver.hasWaitingMessages()){
        // get the next message
        ofxOscMessage m;
        receiver.getNextMessage(&m);
        
        if(m.getAddress() == "/urg/raw/data" && drawMesh){
            mesh.clear();
            for(int i = 0; i < m.getNumArgs(); i+=2){
                float r = m.getArgAsInt32(i);
                float theta = m.getArgAsFloat(i+1);
                float x = r * cos(theta);
                float y = r * sin(theta);
                mesh.addVertex(ofVec3f(x,y));
            }
        }
        
        if(m.getAddress() == "/urg/tracker/data" && drawTrackPoints){
            cout<<"tracker data"<<endl;
            for(int i = 0; i < m.getNumArgs(); i+=3){
                points[m.getArgAsInt32(i)] = ofVec2f(m.getArgAsInt32(i+1), m.getArgAsInt32(i+2));
            }
        }
    }
}


//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(0, 0, 0);
    ofPushMatrix();
    {
        ofTranslate(ofGetWidth()/2, ofGetHeight()/2);
        ofPushMatrix();
        {
            ofScale(scale, scale);
            
            ofPushStyle();
            ofSetCircleResolution(60);
            ofPushMatrix();
            {
                ofNoFill();
                for(int i = 0; i < 10; i++) {
                    float radius = ofMap(i, -1, 10 - 1, 0, 5600);
                    ofSetColor(64);
                    ofCircle(0, 0, radius);
                    ofVec2f textPosition = ofVec2f(radius, 0).rotate(45);
                    ofSetColor(255);
                    ofDrawBitmapString(ofToString(radius, 2) + "mm", textPosition);
                }
            }
            ofPopMatrix();
            if(drawTrackPoints){
                for(map<int, ofVec2f>::iterator iter = points.begin(); iter!=points.end(); iter++){
                    ofPushStyle();
                    ofNoFill();
                    ofSetColor(255, 0, 255);
                    ofCircle(iter->second, 50);
                    ofPopStyle();
                }
            }
            ofSetColor(255, 255, 0);
            if(drawMesh) mesh.drawVertices();
        }
        ofPopMatrix();
    }
    ofPopMatrix();
    
    panel.draw();
}