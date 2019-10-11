//
//  ofxUrgTracker.cpp
//  example
//
//  Created by dantheman on 9/13/15.
//
//

#include "ofxUrgTracker.h"

using namespace cv;
using namespace ofxCv;

void ofxUrgFollower::setup(const cv::Point2f& track) {
    position = track;
    recent = track;
}
cv::Point2f ofxUrgFollower::getPosition(){
    return position;
}

void ofxUrgFollower::update(const cv::Point2f& track) {
    position = toCv(ofVec2f(toOf(position)).interpolate(ofVec2f(toOf(track)), .1));
    recent = track;
    all.addVertex(toOf(position).x, toOf(position).y, 0.0);
}

void ofxUrgFollower::kill() {
    float curTime = ofGetElapsedTimef();
    if(startedDying == 0) {
        startedDying = curTime;
    } else if(curTime - startedDying > dyingTime) {
        dead = true;
    }
}

float ofxUrgFollower::getLiving() const {
    if(startedDying) {
        return 1 - (ofGetElapsedTimef() - startedDying) / dyingTime;
    } else {
        return 1;
    }
}

void ofxUrgFollower::draw(float clusterSize) {
    ofPushStyle();
    if(startedDying) {
        ofSetColor(ofColor::red);
        clusterSize = getLiving() * clusterSize;
    } else {
        ofSetColor(ofColor::green);
    }
    ofDrawCircle(toOf(position), clusterSize);
    ofDrawLine(toOf(recent), toOf(position));
    ofSetColor(255);
    ofDrawBitmapString(ofToString(label) + "[" + ofToString(position.x, 0) + "," + ofToString(position.y, 0) + "]", toOf(recent));
    ofSetColor(255);
    all.draw();
    ofPopStyle();
}
