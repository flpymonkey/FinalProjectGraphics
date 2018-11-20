#include "lights.h"

DirectionalLight::DirectionalLight() {
    glm::vec3 v = glm::vec3(0.0f, 0.0f, 0.0f);
    
    this->direction = v;
    this->ambient = v;
    this->diffuse = v;
    this->specular = v;  
}

DirectionalLight::DirectionalLight(glm::vec3 direction) {
    this->direction = direction;
    this->ambient = glm::vec3(0.5f, 0.5f, 0.5f);
    this->diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
    this->specular = glm::vec3(0.5f, 0.5f, 0.5f);
}

DirectionalLight::~DirectionalLight() {
    // TODO:
}
    
void DirectionalLight::setDirection(glm::vec3 direction) {
    this->direction = direction;
}

glm::vec3 DirectionalLight::getDirection() {
    return this->direction;
}
    
void DirectionalLight::setAmbient(glm::vec3 ambient) {
    this->ambient = ambient;
}

glm::vec3 DirectionalLight::getAmbient() {
    return this->ambient;
}
    
void DirectionalLight::setDiffuse(glm::vec3 diffuse) {
    this->diffuse = diffuse;
}

glm::vec3 DirectionalLight::getDiffuse() {
    return this->diffuse;
}
    
void DirectionalLight::setSpecular(glm::vec3 specular) {
    this->specular = specular;
}

glm::vec3 DirectionalLight::getSpecular() {
    return this->specular;
}

PointLight::PointLight() {
    glm::vec3 v = glm::vec3(0.0f, 0.0f, 0.0f);
    float f = 0.0f;
    
    this->position = v;
    this->ambient = v;
    this->diffuse = v;
    this->specular = v;
    this->constant = f;
    this->linear = f;
    this->quadratic = f;
}

PointLight::PointLight(glm::vec3 position) { 
    this->position = position;
    this->ambient = glm::vec3(0.05f, 0.05f, 0.05f);
    this->diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
    this->specular = glm::vec3(1.0f, 1.0f, 1.0f);
    this->constant = 1.0f;
    this->linear = 0.09f;
    this->quadratic = 0.032f;
}

PointLight::~PointLight() {
    // TODO:
}
    
void PointLight::setPosition(glm::vec3 position) {
    this->position = position;
}

glm::vec3 PointLight::getPosition() {
    return this->position;
}
    
void PointLight::setAmbient(glm::vec3 ambient) {
    this->ambient = ambient;
}

glm::vec3 PointLight::getAmbient() {
    return this->ambient;
}
    
void PointLight::setDiffuse(glm::vec3 diffuse) {
    this->diffuse = diffuse;
}

glm::vec3 PointLight::getDiffuse() {
    return this->diffuse;
}
    
void PointLight::setSpecular(glm::vec3 specular) {
    this->specular = specular;
}

glm::vec3 PointLight::getSpecular() {
    return this->specular;
}
    
void PointLight::setConstant(float constant) {
    this->constant = constant;
}

float PointLight::getConstant() {
    return this->constant;
}
    
void PointLight::setLinear(float linear) {
    this->linear = linear;
}

float PointLight::getLinear() {
    return this->linear;
}
    
void PointLight::setQuadratic(float quadratic) {
    this->quadratic = quadratic;
}

float PointLight::getQuadratic() {
    return this->quadratic;
}

SpotLight::SpotLight() {
    glm::vec3 v = glm::vec3(0.0f, 0.0f, 0.0f);
    float f = 0.0f;
    
    this->position = v;
    this->direction = v;
    this->ambient = v;
    this->diffuse = v;
    this->specular = v;
    this->constant = f;
    this->linear = f;
    this->quadratic = f;
    this->cutOff = f;
    this->outerCutOff = f;
}

SpotLight::SpotLight(glm::vec3 position, glm::vec3 direction) {
    this->position = position;
    this->direction = direction;
    this->ambient = glm::vec3(0.0f, 0.0f, 0.0f);
    this->diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
    this->specular = glm::vec3(1.0f, 1.0f, 1.0f);
    this->constant = 1.0f;
    this->linear = 0.09f;
    this->quadratic = 0.032f;
    this->cutOff = glm::cos(glm::radians(12.5f));
    this->outerCutOff = glm::cos(glm::radians(15.0f));
}

SpotLight::~SpotLight() {
    // TODO:
}
    
void SpotLight::setPosition(glm::vec3 position) {
    this->position = position;
}

glm::vec3 SpotLight::getPosition() {
    return this->position;
}
    
void SpotLight::setDirection(glm::vec3 direction) {
    this->direction = direction;
}

glm::vec3 SpotLight::getDirection() {
    return this->direction;
}
    
void SpotLight::setAmbient(glm::vec3 ambient) {
    this->ambient = ambient;
}

glm::vec3 SpotLight::getAmbient() {
    return this->ambient;
}
    
void SpotLight::setDiffuse(glm::vec3 diffuse) {
    this->diffuse = diffuse;
}

glm::vec3 SpotLight::getDiffuse() {
    return this->diffuse;
}
    
void SpotLight::setSpecular(glm::vec3 specular) {
    this->specular = specular;
}

glm::vec3 SpotLight::getSpecular() {
    return this->specular;
}
    
void SpotLight::setConstant(float constant) {
    this->constant = constant;
}

float SpotLight::getConstant() {
    return this->constant;
}
    
void SpotLight::setLinear(float linear) {
    this->linear = linear;
}

float SpotLight::getLinear() {
    return this->linear;
}
    
void SpotLight::setQuadratic(float quadratic) {
    this->quadratic = quadratic;
}

float SpotLight::getQuadratic() {
    return this->quadratic;
}

void SpotLight::setCutOff(float cutOff) {
    this->cutOff = cutOff;
}

float SpotLight::getCutOff() {
    return this->cutOff;
}
    
void SpotLight::setOuterCutOff(float outerCutOff) {
    this->outerCutOff = outerCutOff;
}

float SpotLight::getOuterCutOff() {
    return this->outerCutOff;
}
