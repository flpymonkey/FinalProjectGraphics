#ifndef LIGHTS_H
#define LIGHTS_H

#include <glm/glm.hpp>
#include <vector>

class DirectionalLight {
public:
    DirectionalLight();
    ~DirectionalLight();
    
    void setDirection(glm::vec3 direction);
    glm::vec3 getDirection();
    
    void setAmbient(glm::vec3 ambient);
    glm::vec3 getAmbient();
    
    void setDiffuse(glm::vec3 diffuse);
    glm::vec3 getDiffuse();
    
    void setSpecular(glm::vec3 specular);
    glm::vec3 getSpecular();
	
private:
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

class PointLight {
public:
    PointLight();
    ~PointLight();
    
    void setPosition(glm::vec3 position);
    glm::vec3 getPosition();
    
    void setAmbient(glm::vec3 ambient);
    glm::vec3 getAmbient();
    
    void setDiffuse(glm::vec3 diffuse);
    glm::vec3 getDiffuse();
    
    void setSpecular(glm::vec3 specular);
    glm::vec3 getSpecular();
    
    void setConstant(float constant);
    float getConstant();
    
    void setLinear(float linear);
    float getLinear();
    
    void setQuadratic(float quadratic);
    float getQuadratic();
	
private:
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

class SpotLight {
public:
    SpotLight();
    ~SpotLight();
    
    void setPosition(glm::vec3 position);
    glm::vec3 getPosition();
    
    void setDirection(glm::vec3 direction);
    glm::vec3 getDirection();
    
    void setAmbient(glm::vec3 ambient);
    glm::vec3 getAmbient();
    
    void setDiffuse(glm::vec3 diffuse);
    glm::vec3 getDiffuse();
    
    void setSpecular(glm::vec3 specular);
    glm::vec3 getSpecular();
    
    void setConstant(float constant);
    float getConstant();
    
    void setLinear(float linear);
    float getLinear();
    
    void setQuadratic(float quadratic);
    float getQuadratic();
    
    void setCutOff(float cutOff);
    float getCutOff();
    
    void setOuterCutOff(float outerCutOff);
    float getOuterCutOff();
	
private:
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;  
    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
};

#endif
