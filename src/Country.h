#pragma once
#include "ofMain.h"

class Country {
    public:
    ofMesh mesh;
    float alpha = 1.0;

    Country();

    void addPoint(glm::vec2 point, ofColor col);
    void draw();
};