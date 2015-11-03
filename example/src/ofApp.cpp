#include "ofMain.h"
#include "ofxUrg.h"
#include "ofxSyphon.h"
#include "ofxOsc.h"
#include "ofxXmlSettings.h"
#include "ofxGui.h"
#include "ofxUrgTracker.h"
#include "ofxTiming.h"
#include "ofxCv.h"
class ofApp : public ofBaseApp {
    
    
    ofxUrg::Device urg;
    ofxUrgTracker<ofxUrgFollower> tracker;
    ofRectangle trackingRegion;
    ofEasyCam cam;
    ofxSyphonServer server;
    ofxOscSender sender;
    ofxXmlSettings settings;
    ofParameter<string> oscPort;
    ofParameter<string> oscHost;
    ofParameterGroup oscParameters;
    
    ofParameter<string> maxRange;
    ofParameter<string> urgPort;
    ofParameterGroup urgParameters;
    
    ofParameter<float> zoom;
    ofParameter<ofVec2f> searchRegionCenterPt;
    ofParameter<float> searchRegionSize;
    ofRectangle searchRegion;
    ofParameter<bool> useAutoRegion;
    ofParameterGroup viewParameters;
    
    ofxPanel panel;
    vector<long> urgData;
    ofMesh mesh;
    ofImage img;
    
    map<unsigned int, Hysteresis*> timers;
    ofParameter<int> maxStddev;
    ofParameter<int> maxClusterCount;
    int preCount;
    int preSdev;
    
public:
    void setup()
    {
        ofSetFrameRate(60);
        ofSetVerticalSync(true);
        ofBackground(0);
        
        urgParameters.setName("LiDAR Parameter");
        urgParameters.add(urgPort.set("Serial Port", "/dev/tty.usbmodem14121"));
        urgParameters.add(maxRange.set("Max Range", "5600"));
        urgParameters.add(maxClusterCount.set("maxClusterCount", 12, 1, 400));
        urgParameters.add(maxStddev.set("maxStddev", 60, 1, 400));
        
        
        oscParameters.setName("OSC Paremeters");
        oscParameters.add(oscHost.set("OSC HOST", "127.0.0.1"));
        oscParameters.add(oscPort.set("OSC PORT", "7777"));
        
        viewParameters.add(searchRegionCenterPt.set("Search Region Center", ofVec2f(-1500, -1500), ofVec2f(-ofToInt(maxRange), -ofToInt(maxRange)), ofVec2f(ofToInt(maxRange), ofToInt(maxRange))));
        viewParameters.add(useAutoRegion.set("Use Auto Region", false));
        viewParameters.add(searchRegionSize.set("Search Region Size", 1500, 0, ofToInt(maxRange)*2));
        viewParameters.add(zoom.set("Zoom", 0.05, 0.05, .2));
        
        searchRegionCenterPt.addListener(this, &ofApp::updatedSearchCenterPoint);
        searchRegionSize.addListener(this, &ofApp::updatedSearchSize);
        useAutoRegion.addListener(this, &ofApp::updateAutoRegion);
        searchRegion.set(searchRegionCenterPt.get(), searchRegionSize, searchRegionSize);
        
        
        panel.setup();
        panel.add(urgParameters);
        panel.add(oscParameters);
        panel.add(viewParameters);
        panel.setSize(350, 350);
        panel.setWidthElements(350);
        //
        //        panel.saveToFile("settings.xml");
        panel.loadFromFile("settings.xml");
        
        
        int _max = ofToInt(maxRange);
        urg.setMode(ofxUrg::DISTANCE);
        urg.setupSerial(urgPort);
        urg.setRange(-_max, _max);
        
        ofLogNotice("Product", urg.productType());
        ofLogNotice("Serial", urg.serialId());
        ofLogNotice("Status", urg.status());
        ofLogNotice("State", urg.state());
        ofLogNotice("Firmware version", urg.firmwareVersion());
        
        urg.start();
        
        
        sender.setup(oscHost.get(), ofToInt(oscPort.get()));
        
        mesh = ofMesh();
        
        trackingRegion.set(-_max/2, -_max/2, _max, _max);
        
        tracker.setupKmeans(maxStddev, maxClusterCount);
        tracker.setMaximumDistance(400);
        tracker.setPersistence(10);
        tracker.setRegion(trackingRegion);
        tracker.setRegion(searchRegion);
        
    }
    void updatedSearchCenterPoint(ofVec2f & center){
        searchRegion.set(searchRegionCenterPt.get(), searchRegionSize, searchRegionSize);
        tracker.setRegion(searchRegion);
    }
    void updatedSearchSize(float & size){
        searchRegion.set(searchRegionCenterPt.get(), size, size);
        tracker.setRegion(searchRegion);
    }
    void updateAutoRegion(bool & input){
        
        tracker.setUseAutoRegion(input);
        
    }
    
    
    void exit(){
        panel.saveToFile("settings.xml");
    }
    
    void update()
    {
        urg.update();
        if(urg.isFrameNew()){
            
            tracker.update(urg);
            urgData = urg.getData();
            if(urgData.size() > 0){
                ofxOscMessage m;
                m.setAddress("/urg/raw/data");
                for(int i = 0; i < urgData.size(); i++){
                    m.addIntArg(urgData[i]);
                    m.addFloatArg(urg.index2rad(i));
                }
                sender.sendMessage(m);
            }
            
            vector<ofxUrgFollower> followers = tracker.getFollowers();
            if(followers.size() > 0){
                ofxOscMessage m;
                m.setAddress("/urg/tracker/data");
                for(int i = 0; i < followers.size(); i++){
                    ofVec2f pt = ofxCv::toOf(followers[i].getPosition());
                    m.addIntArg(i);
                    m.addFloatArg(pt.x);
                    m.addFloatArg(pt.y);
                    m.addFloatArg(followers[i].getLiving());
                }
                sender.sendMessage(m);
            }
        }
        
        if(maxStddev != preSdev || maxClusterCount != preCount){
            tracker.setupKmeans(maxStddev, maxClusterCount);
            preCount = maxClusterCount;
            preSdev = maxStddev;
        }
        
    }
    void keyPressed(int key){
        
    }
    
    void draw()
    {
        
        ofPushMatrix();
        ofTranslate(ofGetWidth()/2, ofGetHeight()/2);
        ofPushMatrix();
        ofScale(zoom, zoom);
        urg.draw(10, 5600);
        tracker.draw();
        ofPopMatrix();
        ofPopMatrix();
        
        ofDrawBitmapString(ofToString(ofGetFrameRate(), 0), 20, 20);
        
        //        server.publishScreen();
        panel.draw();
    }
};

//========================================================================
int main( ){
    ofSetupOpenGL(1280,720,OF_WINDOW);			// <-------- setup the GL context
    
    // this kicks off the running of my app
    // can be OF_WINDOW or OF_FULLSCREEN
    // pass in width and height too:
    ofRunApp(new ofApp());
    
}
