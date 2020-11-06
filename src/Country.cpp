#include "Country.h"


Country::Country() {
    mesh.setMode(OF_PRIMITIVE_POINTS);
    glEnable(GL_POINT_SMOOTH);
    glPointSize(2);
}

void Country::addPoint(glm::vec2 point, ofColor col) {
    mesh.addColor(col);
    mesh.addVertex(glm::vec3(point.x, point.y, 0));
}

void Country::draw() {
    ofSetColor(255, alpha * 255);
    mesh.draw();
}