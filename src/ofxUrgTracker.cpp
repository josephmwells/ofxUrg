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
    position = toCv(toOf(position).interpolate(toOf(track), .1));
    recent = track;
    all.addVertex(toOf(position));
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
    ofCircle(toOf(position), clusterSize);
    ofLine(toOf(recent), toOf(position));
    ofSetColor(255);
    ofDrawBitmapString(ofToString(label), toOf(recent));
    all.draw();
    ofPopStyle();
}