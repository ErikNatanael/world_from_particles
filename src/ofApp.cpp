#include "ofApp.h"
#include "ofxCv.h"
#include "ofxJSON.h"

#include "Country.h"

//--------------------------------------------------------------
void ofApp::setup(){

    cvScale = 4;
    rect = Rect(0, 0, ofGetWidth()*2*cvScale, ofGetHeight()*2*cvScale);
    // ofLog() << "w: " << ofGetWidth()*2*cvScale << ", h: " << ofGetHeight()*2*cvScale;
    subdiv = Subdiv2D(rect);
    cvOffset = glm::vec2(ofGetWidth(), ofGetHeight()); // coordinates have to be positive for the triangulation
    
    
    //-------------------------
    // We can change translate mode.
    // Choose OFX_GEO_JSON_MERCATROE mode or OFX_GEO_JSON_EQUIRECTANGULAR mode. Default is OFX_GEO_JSON_MERCATROE.
    //-------------------------
    
    geojson.setMode(OFX_GEO_JSON_MERCATROE);
    
    //-------------------------
    // Set scale and translate
    //-------------------------

    geojson.setScale(250.0);
    geojson.setTranslate(0, 0);
    
    //-------------------------
    // Load GeoJSON file from project's data path.
    // If it's loaded ofxGeoJSON makes ofMesh's vector array in this transcation.
    //-------------------------
    
    if (geojson.load("countries.geo.json")) {
        ofLog(OF_LOG_NOTICE, "Succeed to load geojson..");
    } else {
        ofLog(OF_LOG_NOTICE, "Failed to load geojson..");
    };

    // Generate triangles from all of the countries
    countryNames = geojson.getFeatureNames();
    for(auto& c : countryNames) {
        generateTrianglesFromCountry(c);
        pointsPerCountry.insert({c, 0});
        countries.insert({c, Country()});
    }

    fbo.allocate(3840, 2160, GL_RGBA32F);
    renderFbo.allocate(3840, 2160, GL_RGB32F);

    //set the camera
    cam.setAutoDistance(true);
    cam.setNearClip(0.01);
    cam.setFarClip(100000);
    cam.setFov(45);
    cam.setDistance(500);

    ofSetBackgroundAuto(false);
    ofBackground(255);

    fbo.begin();
    // ofBackground(255, 255, 255);
    ofClear(255);
    fbo.end();
    renderFbo.begin();
        ofClear(255);
        ofBackground(255, 255);
    renderFbo.end();

    renderDirectory = ofFilePath::getCurrentWorkingDirectory() + "/data/render/";
    cout << renderDirectory << endl;


    for(auto& c : countryNames) {
        drawTriangles(c);
    }

    // client.connect("130.237.222.185:1189", true);
    ofxLibwebsockets::ClientOptions options = ofxLibwebsockets::defaultClientOptions();
    options.host = "130.237.222.185";
    options.port = 1189;
    options.bUseSSL = false;
    client.connect(options);
    client.addListener(this);
}

//--------------------------------------------------------------
void ofApp::update(){
    cam.lookAt(ofVec3f(0,0,0));
}

//--------------------------------------------------------------
void ofApp::draw(){
    float now = ofGetElapsedTimef();
    if(lastFrameNow == 0) {
        lastFrameNow = now;
    }
    float dt = now - lastFrameNow;
    

    fbo.begin();
    // ofBackground(255, 255, 255, 255);
    ofClear(255);
    cam.begin();
    
    
    //-------------------------
    // draw mashes
    //-------------------------
    // geojson.draw();

    // for(int i=0; i < 500; i++) {
    //     ofSetColor(255, 0, 0, 100);
    //     int index = ofRandom(countryNames.size());
    //     // float size = ofRandom(1);
    //     drawPointInCountry(countryNames[index], 1.0);

    //     drawPointInCountry("Sweden", 1.0);
    //     drawPointInCountry("Ireland", 1.0);
    //     drawPointInCountry("United States of America", 1.0);
    // }

    // Set colour based on time of day
    float time = fmod((ofGetElapsedTimef() * 0.01f), 1.0f);

    ofFloatColor c = ofFloatColor(time, 0.0, 1.0-time, 0.5);
    ofSetColor(c);
        // ofSetColor(255, 0, 0, 1);

    for (std::pair<std::string, int> element : pointsPerCountry) {
        
        // drawPointInCountry(element.first, 1.0, element.second);
        addPointToCountry(element.first, 1.0, element.second, c);
        element.second = 0;
    }

    for (std::pair<std::string, Country> element : countries) {
        
        element.second.draw();
    }
    
    cam.end();
    fbo.end();

    ofSetColor(255);
    renderFbo.begin();
        ofClear(255);
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        ofSetColor(255, 255);
        fbo.draw(0, 0);
    renderFbo.end();

    renderFbo.draw(0, 0, ofGetWidth(), ofGetHeight());

    renderCounter += dt;
    if(renderCounter >= renderInterval) {
        renderFrameToFile();
        renderCounter = 0;
    }

    lastFrameNow = now;
}

void ofApp::drawPointInCountry(string country, float size, int numPoints) {
    // Method:
    // 1. Convert the meshes to triangles
    // 2. Pick a random triangle (potentially weighted by the relative area of the triangles)
    // 3. Pick a random point within the triangle using
    //  P = (1 - sqrt(r1)) * A + (sqrt(r1) * (1 - r2)) * B + (r2 * sqrt(r1)) * C
    // where r1 and r2 are uniform random numbers between 0 and 1 and A, B and C are vertices of a triangle
    auto it = countryTriangles.find(country);
    if(it != countryTriangles.end()) {
        auto& triangles = it->second;
        for(int i = 0; i < numPoints; i++) {
            int index = ofRandom(triangles.size());
            auto& p = triangles[index].points;
            float r1 = ofRandom(1.0);
            float r2 = ofRandom(1.0);
            float sqrtr1 = sqrt(r1);
            float min1sqrtr1 = 1-sqrtr1;
            float secondTerm = sqrtr1 * (1-r2);
            float thirdTerm = r2 * sqrtr1;
            float x = min1sqrtr1 * p[0].x + secondTerm * p[1].x + thirdTerm * p[2].x;
            float y = min1sqrtr1 * p[0].y + secondTerm * p[1].y + thirdTerm * p[2].y;
            glm::vec2 point = glm::vec2(x, y);

            float dist2ToPoints = glm::length2(point - glm::vec2(p[0].x, p[0].y));
            dist2ToPoints += glm::length2(point - glm::vec2(p[1].x, p[1].y));
            dist2ToPoints += glm::length2(point - glm::vec2(p[2].x, p[2].y));
            dist2ToPoints = sqrt(dist2ToPoints) - 1.0;
            dist2ToPoints *= 0.1;
            size = max(size + dist2ToPoints, 0.0f);
            size *= 0.1;
            ofDrawEllipse(x, y, size, size);
        }
    }
}

void ofApp::addPointToCountry(string country, float size, int numPoints, ofColor col) {
    // Method:
    // 1. Convert the meshes to triangles
    // 2. Pick a random triangle (potentially weighted by the relative area of the triangles)
    // 3. Pick a random point within the triangle using
    //  P = (1 - sqrt(r1)) * A + (sqrt(r1) * (1 - r2)) * B + (r2 * sqrt(r1)) * C
    // where r1 and r2 are uniform random numbers between 0 and 1 and A, B and C are vertices of a triangle
    auto it = countryTriangles.find(country);
    if(it != countryTriangles.end()) {
        auto& triangles = it->second;
        for(int i = 0; i < numPoints; i++) {
            int index = ofRandom(triangles.size());
            auto& p = triangles[index].points;
            float r1 = ofRandom(1.0);
            float r2 = ofRandom(1.0);
            float sqrtr1 = sqrt(r1);
            float min1sqrtr1 = 1-sqrtr1;
            float secondTerm = sqrtr1 * (1-r2);
            float thirdTerm = r2 * sqrtr1;
            float x = min1sqrtr1 * p[0].x + secondTerm * p[1].x + thirdTerm * p[2].x;
            float y = min1sqrtr1 * p[0].y + secondTerm * p[1].y + thirdTerm * p[2].y;
            glm::vec2 point = glm::vec2(x, y);

            auto it2 = countries.find(country);
            if(it2 != countries.end()) {
                it2->second.addPoint(point, col);
            }
        }
    }
}

void ofApp::drawTriangles(string country) {
    auto it = countryTriangles.find(country);
    if(it != countryTriangles.end()) {
        auto& triangles = it->second;
        for(size_t i = 0; i < triangles.size(); i++) {
            auto& pt = triangles[i].points;
            ofSetColor(255, 0, 0, 255);
            ofDrawLine(pt[0], pt[1]);
            ofDrawLine(pt[1].x, pt[1].y, pt[2].x, pt[2].y);
            ofDrawLine(pt[2].x, pt[2].y, pt[0].x, pt[0].y);
        }
    }    
}

void ofApp::generateTrianglesFromCountry(string country) {
    vector< ofPtr<ofMesh> > meshes = geojson.getFeature(country);
    vector<Triangle> triangles;
    
    for (int i = 0; i<meshes.size(); i++) {
        // ofLog(OF_LOG_NOTICE) << "new polygon, i: " << i;
        int size = meshes[i].get()->getNumVertices();
        subdiv = Subdiv2D(rect);
        subdiv.initDelaunay(rect);
        vector<Point> polygon;
        for (int j = 0; j < size; j++) {
            glm::vec3 point = meshes[i].get()->getVertex(j);
            // ofLog(OF_LOG_NOTICE) << cvRound((point.x + cvOffset.x) * cvScale) << ", " << cvRound((point.y + cvOffset.y) * cvScale);
            subdiv.insert(Point2f(cvRound((point.x + cvOffset.x) * cvScale), cvRound((point.y + cvOffset.y) * cvScale)));
            polygon.push_back(Point((point.x + cvOffset.x) * cvScale, (point.y + cvOffset.y) * cvScale));
        }
        // generate triangles
        vector<Vec6f> triangleList;
        subdiv.getTriangleList(triangleList);
        vector<Point> pt(3);
    
        for( size_t i = 0; i < triangleList.size(); i++ )
        {
            Vec6f t = triangleList[i];
            pt[0] = Point(cvRound(t[0]), cvRound(t[1]));
            pt[1] = Point(cvRound(t[2]), cvRound(t[3]));
            pt[2] = Point(cvRound(t[4]), cvRound(t[5]));

            // TODO: Reject triangles whose center is outside of the polygon
            Point centerOfTriangle(
                (pt[0].x + pt[1].x + pt[2].x) / 3,
                (pt[0].y + pt[1].y + pt[2].y) / 3
            );

            // Only add the triangle if it is inside the polygon contour
            if(pointPolygonTest( polygon, centerOfTriangle, false ) >= 0) {
                triangles.push_back(Triangle(
                    glm::vec2((pt[0].x/cvScale - cvOffset.x), (pt[0].y/cvScale - cvOffset.y)), 
                    glm::vec2((pt[1].x/cvScale - cvOffset.x), (pt[1].y/cvScale - cvOffset.y)), 
                    glm::vec2((pt[2].x/cvScale - cvOffset.x), (pt[2].y/cvScale - cvOffset.y))));
                // auto& p = triangles[triangles.size()-1].points;
                // ofLog() << p[0].x << ", " << p[0].y << " | " << p[1].x << ", " << p[1].y << " | " << p[2].x << ", " << p[2].y;
            }
            
            // // Draw rectangles completely inside the image.
            // if ( rect.contains(pt[0]) && rect.contains(pt[1]) && rect.contains(pt[2]))
            // {
            //     ofSetColor(255, 0, 0, 255);
            //     ofDrawLine(toOf(pt[0]), toOf(pt[1]));
            //     ofDrawLine(pt[1].x, pt[1].y, pt[2].x, pt[2].y);
            //     ofDrawLine(pt[2].x, pt[2].y, pt[0].x, pt[0].y);
            // }
        }
    }

    countryTriangles.insert({country, triangles});
}

void ofApp::renderFrameToFile() {
    // renderFbo.begin();
    //     ofClear(255);
    //     ofBackground(255, 255);
    //     fbo.draw(0, 0);
    // renderFbo.end();
    renderFbo.readToPixels(renderPixels);
    grabImg.setFromPixels(renderPixels);
    string dateString = ofGetTimestampString();
    grabImg.save(renderDirectory + "frame-" + dateString + ".png");
    frameNumber++;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

//--------------------------------------------------------------
void ofApp::onConnect( ofxLibwebsockets::Event& args ){
    cout<<"on connected"<<endl;
}

//--------------------------------------------------------------
void ofApp::onOpen( ofxLibwebsockets::Event& args ){
    cout<<"on open"<<endl;
}

//--------------------------------------------------------------
void ofApp::onClose( ofxLibwebsockets::Event& args ){
    cout<<"on close"<<endl;
}

//--------------------------------------------------------------
void ofApp::onIdle( ofxLibwebsockets::Event& args ){
    cout<<"on idle"<<endl;
}

//--------------------------------------------------------------
void ofApp::onMessage( ofxLibwebsockets::Event& args ){
    // cout<<"got message "<<args.message<<endl;
    ofxJSONElement json = ofxJSONElement(args.message);
    string country = json["local_location"]["country"].asString();
    if(country == "Sweden") {
        country = json["remote_location"]["country"].asString();
    }
    if(country == "United States") {
        country = "United States of America";
    } else if(country == "Tanzania") {
        country = "United Republic of Tanzania";
    } else if(country == "Serbia") {
        country = "Republic of Serbia";
    } else if(country == "Myanmar (Burma)") {
        country = "Myanmar";
    } else if(country == "Åland Islands") {
        country = "Finland";
    } else if(country == "Isle of Man") {
        country = "United Kingdom";
    } else if(country == "Cote d'Ivoire") {
        country = "Ivory Coast";
    } else if(country == "North Macedonia") {
        country = "Macedonia";
    }
    // "Virgin Islands", "Brittish Virgin Islans", "Hong Kong (China)", "Saint Kitts and Nevis", "Singapore", 
    // "Mauritius", "Macau (China)", "Maldives", "Palestinian Territory", "Bahrain"
    auto it = std::find(countryNames.begin(), countryNames.end(), country);
    if(it == countryNames.end() && country != "") {
        ofLog() << country;
    }

    auto it2 = pointsPerCountry.find(country);
    if(it2 != pointsPerCountry.end()) {
        it2->second += 1;
    }
}

//--------------------------------------------------------------
void ofApp::onBroadcast( ofxLibwebsockets::Event& args ){
    cout<<"got broadcast "<<args.message<<endl;
}