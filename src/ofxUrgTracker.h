/*
 ofxUrgTracker helps cluster and tracker data from an ofxUrg source, but it
 relies on ofxCv/OpenCv to do this so it's an optional class. If you don't
 need to cluster or track points, you can just remove the files.
 */

#pragma once

#include "ofxUrg.h"
#include "ofxCv.h"

class ofxUrgFollower : public ofxCv::PointFollower {
protected:
    cv::Point2f position, recent;
    float startedDying;
    ofPolyline all;
    float dyingTime;
public:
    ofxUrgFollower()
    :startedDying(0)
    ,dyingTime(1) {
    }
    virtual void setup(const cv::Point2f& track);
    virtual void update(const cv::Point2f& track);
    virtual void kill();
    virtual float getLiving() const;
    virtual void draw(float clusterSize = 60);
    virtual cv::Point2f getPosition();
};

template <class F>
class ofxUrgTracker : public ofxCv::PointTrackerFollower<F> {
public:
    
    ofxUrgTracker()
    :maxClusterCount(12)
    ,minClusterSize(1)
    ,maxPointDistance(50)
    ,useKmeans(true)
    ,maxStddev(60)
    ,captureOutline(false){ // 60 is good for hand/arm tracking
    }
    void setupKmeans(float maxStddev, unsigned int maxClusterCount) {
        this->maxClusterCount = maxClusterCount;
        this->maxStddev = maxStddev;
        useKmeans = true;
    }
    void setupNaive(int minClusterSize, int maxPointDistance) {
        this->maxPointDistance = maxPointDistance;
        this->minClusterSize = minClusterSize;
        useKmeans = false;
    }
    void setUseAutoRegion(bool use){
        captureOutline = true;
        useOutline = use;
    }
    void setRegion(const ofRectangle& region) {
        this->region = region;
    }
    void draw() {
        ofPushStyle();
        ofNoFill();
        vector<F>& followers = ofxCv::PointTrackerFollower<F>::followers;
        for(int i = 0; i < followers.size(); i++) {
            ((ofxUrgFollower) followers[i]).draw();
        }
        ofSetColor(255);
        if(useOutline){
            outline.draw();
        }else{
            ofRect(region);
        }

        ofPopStyle();
    }
    void update(const vector<ofPoint>& points) {
        if(captureOutline && useOutline){
            outline.clear();
            left.clear();
            right.clear();
            for(int i = 1; i < points.size(); i++){
                
                
                const ofPoint & thisPoint = points[i - 1];
                
                outline.addVertex(thisPoint);
                
            }
            outline.close();
            outline = outline.getSmoothed(8);
            int numVerts = outline.getVertices().size();
            for(int i = 1; i < numVerts; i++){
                const ofPoint & thisPoint = outline.getVertices()[i - 1];
                const ofPoint & nextPoint = outline.getVertices()[i];
                ofPoint delta		= nextPoint - thisPoint;
                ofPoint deltaNorm	= delta.normalized();
                
                ofVec3f toTheLeft	= deltaNorm.getPerpendicular(ofVec3f(0, 0, 1));
                
                ofPoint L = thisPoint + toTheLeft * 100;
                ofPoint R = thisPoint - toTheLeft * 100;
                left.addVertex(L);
                right.addVertex(R);
            }
            
            
            right.close();
            left.close();
            
            
            
            //            ofPoint center = outline.getCentroid2D();
            //            for(int i = 0; i < outline.getVertices().size(); i++){
            //                ofPoint pt = outline.getVertices()[i];
            //                outline.getVertices()[i] = pt*0.9;
            //            }
            //            outline = outline.getSmoothed(8);
            //            right = right.getSmoothed(8);
            //            left = left.getSmoothed(8);
            captureOutline = false;
        }
        if(useKmeans) {
            updateKmeans(points);
        } else {
            updateNaive(points);
        }
        ofxCv::PointTrackerFollower<F>::track(clusters);
    }
    void update(ofxUrg::Device& sick) {
        update(sick.getPoints());
    }
    unsigned int size() const {
        return clusters.size();
    }
    cv::Point2f getCluster(unsigned int i) const {
        return clusters.at(i);
    }
    const vector<cv::Point2f>& getClusters() const {
        return clusters;
    }
    void saveOutline(){
        outline.clear();
        captureOutline = true;
    }
protected:
    bool useOutline;
    bool captureOutline;
    ofPolyline outline;
    ofPolyline left;
    ofPolyline right;
    vector<cv::Point2f> clusters;
    ofRectangle region;
    unsigned int maxClusterCount;
    float maxStddev;
    bool useKmeans;
    int minClusterSize, maxPointDistance;
    
    void updateNaive(const vector<ofPoint>& points) {
        if(points.size() > 0) {
            vector< vector<glm::vec2> > all;
            for(int i = 0; i < points.size(); i++) {
                const glm::vec2& cur = glm::vec2(points[i].x, points[i].y);
                if(region.inside(cur)) {
                    if(all.empty()) {
                        all.push_back(vector<glm::vec2>());
                    } else {
                        glm::vec2& prev = all.back().back();
                        float distance = glm::distance(cur, prev);
                        if(distance > maxPointDistance) {
                            all.push_back(vector<glm::vec2>());
                        }
                    }
                    all.back().push_back(cur);
                }
            }
            clusters.clear();
            for(int i = 0; i < all.size(); i++) {
                if(all[i].size() >= minClusterSize) {
                    vector<cv::Point2f> allCv = ofxCv::toCv(all[i]);
                    
                    cv::Mat curMat(allCv);
                    cv::Scalar curMean, curStddev;
                    cv::meanStdDev(curMat, curMean, curStddev);
                    clusters.push_back(cv::Point2f(curMean[0], curMean[1]));
                    
                    /*
                     cv::Point2f enclosingCenter;
                     float radius;
                     cv::minEnclosingCircle(allCv, enclosingCenter, radius);
                     clusters.push_back(enclosingCenter);
                     */
                }
            }
        }
    }
    
    void updateKmeans(const vector<ofPoint>& points) {
        // build samples vector for all points within the bounds
        vector<cv::Point2f> samples;
        if(useOutline){
            ofPoint center = outline.getCentroid2D();
            for(int i = 0; i < points.size(); i++) {
                if(right.inside(points[i])) {
                    samples.push_back(ofxCv::toCv(ofVec2f(points[i].x, points[i].y)));
                }
            }
        }else{
            ofPoint center = region.getCenter();
            for(int i = 0; i < points.size(); i++) {
                if(region.inside(ofVec2f(points[i].x, points[i].y))) {
                    samples.push_back(ofxCv::toCv(ofVec2f(points[i].x, points[i].y)));
                }
            }
        }
        
        cv::Mat samplesMat = cv::Mat(samples).reshape(1);
        clusters.clear();
        for(int clusterCount = 1; clusterCount < maxClusterCount; clusterCount++) {
            if(samples.size() > clusterCount) {
                cv::Mat labelsMat, centersMat;
                float compactness = cv::kmeans(samplesMat, clusterCount, labelsMat, cv::TermCriteria(), 8, cv::KMEANS_PP_CENTERS, centersMat);
                vector<cv::Point2f> centers = centersMat.reshape(2);
                vector<int> labels = labelsMat;
                vector<cv::Point2f> centered(samples.size());
                vector<int> clusterCount(centers.size());
                for(int i = 0; i < samples.size(); i++) {
                    centered[i] = centers[labels[i]];
                    clusterCount[labels[i]]++;
                }
                cv::Mat centeredMat(centered);
                centeredMat -= cv::Mat(samples);
                cv::Scalar curMean, curStddev;
                cv::meanStdDev(centeredMat, curMean, curStddev);
                float totalDev = ofVec2f(curStddev[0], curStddev[1]).length();
                if(totalDev < maxStddev) {
                    for(int i = centers.size() - 1; i >= 0; i--) {
                        if(clusterCount[i] < minClusterSize) {
                            centers.erase(centers.begin() + i);
                        }
                    }
                    clusters = centers;
                    break;
                }
            }
        }
    }
};
