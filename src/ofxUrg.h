#pragma once

#include "Urg_Driver.h"
#include "ofMain.h"
#define OFX_URG_BEGIN_NAMESPACE namespace ofx { namespace Urg {
#define OFX_URG_END_NAMESPACE } }

OFX_URG_BEGIN_NAMESPACE

static const string DEFAULT_HOST = "192.168.0.10";
static const int DEFAULT_PORT = 10940;

enum ConnectionType
{
    SERIAL,
    ETHERNET
};
enum Mode
{
    DISTANCE,
    DISTANCE_INTENSITY,
    MULTIECHO,
    MULTIECHO_INTENSITY
};

typedef vector<long> Frame;

class Device : public ofThread
{
    
public:
    Device()
    :device_or_ip_name(DEFAULT_HOST)
    ,baudrate_or_port_number(DEFAULT_PORT)
    ,connect_type(ETHERNET)
    ,mode(DISTANCE)
    ,active(false)
    ,is_frame_new(false)
    {}
    
    virtual ~Device()
    {
        if (isThreadRunning()) ofThread::waitForThread();
        if (urg.is_open()) urg.close();
    }
    
    void setMode(const Mode& mode) { this->mode = mode; }
    
    void setRange(int lower, int upper){
        this->upper = upper;
        this->lower = lower;
    }
    
    bool setupSerial(const string& device_name = "", int baudrate = 115200)
    {
        connect_type = SERIAL;
        
        if (!device_name.empty())
        {
            device_or_ip_name = device_name;
        }
        else
        {
            device_or_ip_name = "/dev/tty.usbmodemfa131";
        }
        baudrate_or_port_number = baudrate;
        
        return open();
    }
    
    bool setupEthernet(const string& ip = DEFAULT_HOST, int port = DEFAULT_PORT)
    {
        connect_type = ETHERNET;
        device_or_ip_name = ip;
        baudrate_or_port_number = port;
        
        return open();
    }
    
    bool open()
    {
        if (!urg.open(device_or_ip_name.c_str(), baudrate_or_port_number,
                      (connect_type==SERIAL) ? qrk::Urg_driver::Serial : qrk::Urg_driver::Ethernet))
        {
            ofLogError("Urg connection failed", "%s", urg.what());
            return false;
        }
        
        urg.set_scanning_parameter(minStep(), maxStep());
        urg.set_sensor_time_stamp(ofGetElapsedTimef());
        return true;
    }
    
    void close() { urg.close(); }
    
    bool start()
    {
        stop();
        
        qrk::Lidar::measurement_type_t measurement_type;
        switch (mode) {
            case DISTANCE:
                measurement_type = qrk::Lidar::Distance;
                break;
            case DISTANCE_INTENSITY:
                measurement_type = qrk::Lidar::Distance_intensity;
                break;
            case MULTIECHO:
                measurement_type = qrk::Lidar::Multiecho;
                break;
            case MULTIECHO_INTENSITY:
                measurement_type = qrk::Lidar::Multiecho_intensity;
                break;
        }
        
        active = urg.start_measurement(measurement_type, qrk::Urg_driver::Infinity_times, 0);
        startThread();
        return active;
    }
    void stop()
    {
        waitForThread();
        stopThread();
        urg.stop_measurement();
    }
    
    void update()
    {
        is_frame_new = false;
        
        ofMutex mutex;
        mutex.lock();
        if (data != data_buffer)
        {
            data = data_buffer;
            is_frame_new = true;
        }
        
        if (mode == DISTANCE_INTENSITY || mode == MULTIECHO_INTENSITY)
        {
            if (intensity != intensity_buffer)
            {
                intensity = intensity_buffer;
                is_frame_new = true;
            }
        }
        mutex.unlock();
    }
    
    vector<ofVec2f> getNormalizedPoints() const {
        vector<ofVec2f> points;
        points.resize(data.size());
        ofMutex mutex;
        mutex.lock();
        for(int i = 0; i < points.size(); i++) {
            float r = data[i];
            float theta = urg.index2rad(i);
            float x = ofMap(r * cos(theta), lower, upper, 0, 1000, true);
            float y = ofMap(r * sin(theta), lower, upper, 0, 1000, true);
            points[i] = ofVec2f(x,y);
        }
        mutex.unlock();
        return points;
    }
    
    vector<ofPoint> getPoints() const {
        vector<ofPoint> points;
        points.resize(data.size());
        ofMutex mutex;
        mutex.lock();
        for(int i = 0; i < points.size(); i++) {
            float r = data[i];
            if(r == 0 || r < minDistance()){
//                cout<<r<<endl;
                r = 5600;
            }
            float theta = urg.index2rad(i);
            float x = r * cos(theta);
            float y = r * sin(theta);
            points[i] = ofPoint(x,y);
        }
        mutex.unlock();
        return points;
    }
    
    void draw(int gridDivisions, float gridSize) const {
        ofPushMatrix();
        ofPushStyle();
        ofSetCircleResolution(60);
        ofPushMatrix();
        ofNoFill();
        for(int i = 0; i < gridDivisions; i++) {
            float radius = ofMap(i, -1, gridDivisions - 1, 0, gridSize);
            ofSetColor(64);
            ofDrawCircle(0, 0, radius);
            ofVec2f textPosition = ofVec2f(radius, 0).rotate(45);
            ofSetColor(255);
            ofDrawBitmapString(ofToString(radius, 2) + "mm", textPosition);
        }
        ofPopMatrix();
        
        ofSetColor(255);
        getPointCloud().draw();
        ofPopStyle();
        ofPopMatrix();
    }

    
    void drawDebug(float width = ofGetWindowWidth(), float height = ofGetWindowHeight()) const
    {
        ofMutex mutex;
        mutex.lock();
        for (int i=0; i<data.size(); i++)
        {
            float x = ofMap(i, 0, data.size(), 0, width, true);
            float y = ofMap(data[i], 0, 1500, 0, height, true);
            ofDrawLine(x, 0, x, y);
        }
        mutex.unlock();
    }
    void drawPointCloud() const
    {
        ofPushMatrix();
        ofPushStyle();
        ofMesh m = getPointCloud();
        m.draw();
        ofPopStyle();
        ofPopMatrix();
    }
    
    ofMesh getPointCloud() const{
        ofMesh m;
        m.setMode(OF_PRIMITIVE_POINTS);
        ofMutex mutex;
        mutex.lock();
        for (int i=0; i<data.size(); i++)
        {
            
            float r = data[i];
            float theta = urg.index2rad(i);
            float x = r * cos(theta);
            float y = r * sin(theta);
            m.addVertex(ofVec3f(x,y));
        }
        mutex.unlock();
        return m;
    }
    
    ofMesh getDebugPointCloud() const{
        ofMesh m;
        m.setMode(OF_PRIMITIVE_POINTS);
        ofMutex mutex;
        mutex.lock();
        for (int i=0; i<data.size(); i++)
        {
            
            float r = data[i];
            float theta = urg.index2rad(i);
            float x = r * cos(theta);
            float y = r * sin(theta);
            m.addVertex(ofVec3f(x,y));
            if(i %2 == 0){
                m.addColor(ofColor(255, 0, 255));
            }else{
                m.addColor(ofColor(255, 255, 0));
            }
        }
        mutex.unlock();
        return m;
    }

    
    
    void drawDebugPolar() const
    {
        ofPushMatrix();
        ofPushStyle();
        ofMesh m = getDebugPointCloud();

        m.draw();
        ofPopStyle();
        ofPopMatrix();
    }
    bool isFrameNew() const { return is_frame_new; }
    
    string  productType(void)           const { return urg.product_type(); }
    string  firmwareVersion(void)       const { return urg.firmware_version(); }
    string  serialId(void)              const { return urg.serial_id(); }
    string  status(void)                const { return urg.status(); }
    string  state(void)                 const { return urg.state(); }
    
    int     minStep(void)               const { return urg.min_step(); }
    int     maxStep(void)               const { return urg.max_step(); }
    long    minDistance(void)           const { return urg.min_distance(); }
    long    maxDistance(void)           const { return urg.max_distance(); }
    long    scanUsec(void)              const { return urg.scan_usec(); }
    int     maxDataSize(void)           const { return urg.max_data_size(); }
    int     maxEchoSize(void)           const { return urg.max_echo_size(); }
    
    double  index2rad(int index)        const { return urg.index2rad(index); }
    double  index2deg(int index)        const { return urg.index2deg(index); }
    int     rad2index(double radian)    const { return urg.rad2index(radian); }
    int     deg2index(double degree)    const { return urg.deg2index(degree); }
    int     rad2step(double radian)     const { return urg.rad2step(radian); }
    int     deg2step(double degree)     const { return urg.deg2step(degree); }
    double  step2rad(int step)          const { return urg.step2rad(step); }
    double  step2deg(int step)          const { return urg.step2deg(step); }
    int     step2index(int step)        const { return urg.step2index(step); }
    
    const Frame& getData() const { return data; }
    long getData(int index) const { return data.at(index); }
    const vector<unsigned short>& getIntensity() const { return intensity; }
    unsigned short getIntensity(int index) const { return intensity.at(index); }
    
    vector<ofVec2f> firstPoints;
    vector<ofVec2f> secondPoints;
    int upper;
    int lower;
    ofFbo mTexture;
    ofTexture & getTexture(){
        return mTexture.getTexture();
    }
    
protected:
    
    void threadedFunction()
    {
        while (isThreadRunning())
        {
            if (lock())
            {
                if (active)
                {
                    if (mode == DISTANCE)
                    {
                        if (!urg.get_distance(data_buffer, &timestamp))
                        ofLogError("urg get distance", "%s", urg.what());
                    }
                    if (mode == DISTANCE_INTENSITY)
                    {
                        if (!urg.get_distance_intensity(data_buffer, intensity_buffer, &timestamp))
                            ofLogError("urg get distance intensity", "%s", urg.what());
                    }
                }
            }
            unlock();
        }
    }
    string device_or_ip_name;
    int baudrate_or_port_number;
    ConnectionType connect_type;
    Mode mode;
    qrk::Urg_driver urg;
    
    bool active;
    bool is_frame_new;
    vector<long> data, data_buffer;
    vector<unsigned short> intensity, intensity_buffer;
    long timestamp;
};

OFX_URG_END_NAMESPACE

namespace ofxUrg = ofx::Urg;
