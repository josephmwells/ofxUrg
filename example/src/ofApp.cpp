#include "ofMain.h"
#include "ofxUrg.h"
#include "ofxSyphon.h"
#include "ofxOsc.h"
#include "ofxXmlSettings.h"
#include "ofxGui.h"
#include "ofxUrgTracker.h"
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
    ofxPanel panel;
    vector<long> urgData;
    ofMesh mesh;
    ofImage img;
public:
    void setup()
    {
        ofSetFrameRate(60);
        ofSetVerticalSync(true);
        ofBackground(0);
        
        urgParameters.setName("LiDAR Parameter");
        urgParameters.add(urgPort.set("Serial Port", "/dev/tty.usbmodem14541"));
        urgParameters.add(maxRange.set("Max Range", "5600"));
        
        oscParameters.setName("OSC Paremeters");
        oscParameters.add(oscHost.set("OSC HOST", "127.0.0.1"));
        oscParameters.add(oscPort.set("OSC PORT", "7777"));
        
        panel.setup();
        panel.add(urgParameters);
        panel.add(oscParameters);
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
        
        tracker.setupKmeans(100, 18);
        tracker.setMaximumDistance(400);
        tracker.setPersistence(10);
        tracker.setRegion(trackingRegion);
        
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
                m.setAddress("/urg/data");
                ofBuffer buffer;
                
                buffer.append((char*) &urgData[0], urgData.size());
                //        }
                m.addBlobArg(buffer);
                sender.sendMessage(m);
                
            }
        }
        
        
        
        
    }
    
    void draw()
    {
        
        
        float scale = ofMap(mouseX, 0, ofGetWidth(), 0.05, .2, true);
        
        
        ofPushMatrix();
        ofTranslate(ofGetWidth()/2, ofGetHeight()/2);
        ofPushMatrix();
        ofScale(scale, scale);
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
