#pragma once

#include "ofMain.h"
#include "ofxGeoJSON.h"
#include "ofxCv.h"

#include "ofxLibwebsockets.h"

using namespace cv;
using namespace ofxCv;

struct Triangle{
	public:
	glm::vec2 points[3];

	Triangle(glm::vec2 a, glm::vec2 b, glm::vec2 c) {
		points[0] = a;
		points[1] = b;
		points[2] = c;
	}
};

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

		void drawPointInCountry(string country, float size, int numPoints);
		void drawTriangles(string country);

		void generateTrianglesFromCountry(string country);

		void renderFrameToFile();

		// websocket methods
        void onConnect( ofxLibwebsockets::Event& args );
        void onOpen( ofxLibwebsockets::Event& args );
        void onClose( ofxLibwebsockets::Event& args );
        void onIdle( ofxLibwebsockets::Event& args );
        void onMessage( ofxLibwebsockets::Event& args );
        void onBroadcast( ofxLibwebsockets::Event& args );
		
    private:
        ofxGeoJSON geojson;
        ofEasyCam cam;

		Subdiv2D subdiv; // used for triangulating the countries
		glm::vec2 cvOffset;
		float cvScale;
		Rect rect;

		unordered_map<string, vector<Triangle>> countryTriangles;
		vector<string> countries;

		unordered_map<string, int> pointsPerCountry;

		vector<glm::vec3> points;

		ofFbo fbo;

		size_t numberOfVertices = 0;

		ofxLibwebsockets::Client client;

		float lastFrameNow = 0;
    
		// rendering
        int frameNumber = 0;
		string renderDirectory;
		ofFbo renderFbo;
		ofPixels renderPixels;
		ofImage grabImg;
		double renderCounter = 0;
		double renderInterval = 30.0;
};
