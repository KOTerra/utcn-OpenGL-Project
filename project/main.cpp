//
//  main.cpp
//  OpenGL Advances Lighting
//
//  Created by CGIS on 28/11/16.
//  Copyright © 2016 CGIS. All rights reserved.
//

#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Window.h"
#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"

#include <iostream>
#include <string>

#define MAX_LIGHTS 200

// Global Window object
gps::Window myWindow;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;

struct PointLight {
    glm::vec3 position;
    glm::vec3 color;
    float constant;
    float linear;
    float quadratic;
};

PointLight pointLights[MAX_LIGHTS];


struct PointLightLocs {
    GLuint position;
    GLuint color;
    GLuint constant;
    GLuint linear;
    GLuint quadratic;
};

PointLightLocs pointLightLocs[MAX_LIGHTS];

int lightsCount = 0;


gps::Camera myCamera(
    glm::vec3(0.0f, 0.0f, 2.5f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

#define BASE_CAMERA_SPEED 0.1f
#define SPRINT_CAMERA_SPEED 0.7f
float cameraSpeed = BASE_CAMERA_SPEED;

bool pressedKeys[1024];
float angleY = 0.0f;

gps::Model3D myModel;
gps::Shader myCustomShader;

GLenum glCheckError_(const char* file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM: error = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE: error = "INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION: error = "INVALID_OPERATION";
            break;
        case GL_OUT_OF_MEMORY: error = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}

#define glCheckError() glCheckError_(__FILE__, __LINE__)


void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS)
            pressedKeys[key] = true;
        else if (action == GLFW_RELEASE)
            pressedKeys[key] = false;
    }
}


bool firstMouse = true;
double lastX = 640;
double lastY = 360;
float yaw = -90.0f;
float pitch = 0.0f;


void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos - lastX);
    float yoffset = static_cast<float>(lastY - ypos);
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.05f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    myCamera.rotate(pitch, yaw);

    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // Update only the scene lights (starting from i = 1)
    // Light 0 is the headlight and stays at (0,0,0) in Eye Space
    for (int i = 1; i < MAX_LIGHTS; i++) {
        glm::vec3 lightPosEye = glm::vec3(view * glm::vec4(pointLights[i].position, 1.0f));
        glUniform3fv(pointLightLocs[i].position, 1, glm::value_ptr(lightPosEye));
    }
}

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);

    int retina_width, retina_height;
    glfwGetFramebufferSize(window, &retina_width, &retina_height);


    myWindow.setWindowDimensions(WindowDimensions{retina_width, retina_height});

    // Set the viewport to the new retina size
    glViewport(0, 0, retina_width, retina_height);

    int newWidth, newHeight;
    glfwGetWindowSize(window, &newWidth, &newHeight);
    glfwSetCursorPos(window, newWidth / 2.0, newHeight / 2.0);

    lastX = newWidth / 2.0;
    lastY = newHeight / 2.0;

    // Recalculate projection matrix
    projection = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(retina_width) / static_cast<float>(retina_height),
        0.1f,
        1000.0f
    );

    myCustomShader.useShaderProgram();
    projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}


void processMovement() {
    if (pressedKeys[GLFW_KEY_LEFT_SHIFT]) {
        cameraSpeed = SPRINT_CAMERA_SPEED;
    }
    else {
        cameraSpeed = BASE_CAMERA_SPEED;
    }

    if (pressedKeys[GLFW_KEY_Q]) {
        angleY -= 1.0f;
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    if (pressedKeys[GLFW_KEY_E]) {
        angleY += 1.0f;
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    bool cameraMoved = false;
    if (pressedKeys[GLFW_KEY_W]) {
        myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
        cameraMoved = true;
    }
    if (pressedKeys[GLFW_KEY_S]) {
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        cameraMoved = true;
    }
    if (pressedKeys[GLFW_KEY_A]) {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        cameraMoved = true;
    }
    if (pressedKeys[GLFW_KEY_D]) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        cameraMoved = true;
    }

    if (cameraMoved) {
        view = myCamera.getViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

        // Update only the scene lights (starting from i = 1)
        for (int i = 1; i < MAX_LIGHTS; i++) {
            glm::vec3 lightPosEye = glm::vec3(view * glm::vec4(pointLights[i].position, 1.0f));
            glUniform3fv(pointLightLocs[i].position, 1, glm::value_ptr(lightPosEye));
        }
    }


    if (pressedKeys[GLFW_KEY_P]) {
        glm::mat4 currentView = myCamera.getViewMatrix();


        glm::mat4 inverseView = glm::inverse(currentView);


        glm::vec3 worldPos = glm::vec3(inverseView[3]);

        std::cout << "Camera World Coords: X="
            << worldPos.x << " Y="
            << worldPos.y << " Z="
            << worldPos.z << std::endl;
    }
}


void initOpenGLState() {
    glClearColor(0.4f, 0.015f, 0.01, 1.0);

    WindowDimensions dims = myWindow.getWindowDimensions();
    glViewport(0, 0, dims.width, dims.height);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_FRAMEBUFFER_SRGB);
}


void initObjects() {
    myModel.LoadModel("models/scene/craioveclipsa.obj", "models/scene/");
}

void initShaders() {
    myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
    myCustomShader.useShaderProgram();
}

void addLight(glm::vec3 pos, glm::vec3 color, float constant, float linear, float quadratic) {
    // Safety check
    if (lightsCount >= MAX_LIGHTS) {
        std::cout << "Warning: Exceeded MAX_LIGHTS. Light not added." << std::endl;
        return;
    }

    pointLights[lightsCount].position = pos;
    pointLights[lightsCount].color = color;
    pointLights[lightsCount].constant = constant;
    pointLights[lightsCount].linear = linear;
    pointLights[lightsCount++].quadratic = quadratic;
}


void initUniforms() {
    model = glm::mat4(1.0f);
    modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));


    WindowDimensions dims = myWindow.getWindowDimensions();
    projection = glm::perspective(glm::radians(45.0f), (float)dims.width / (float)dims.height, 0.1f, 1000.0f);
    projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    lightsCount = 0;

    // Light 0: Headlight (White)
    addLight(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), 1.0f, 0.09f, 0.032f);
    //

    // Light 1: Scene Light (Red)
    addLight(glm::vec3(20.0f, 20.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 1.0f, 0.09f, 0.032f);
    addLight(glm::vec3(15.59f, 44.59f, 34.14f), glm::vec3(1.0f, 0.0f, 0.0f), 1.0f, 0.09f, 0.032f);


    // Light 2: Scene Light (Blue)
    addLight(glm::vec3(-13.2f, 43.0f, .2f), glm::vec3(0.0f, 0.0f, 1.0f), 1.0f, 0.09f, 0.032f);

    // Light 3: Scene Light (Green)
    addLight(glm::vec3(-20.0f, 20.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 1.0f, 0.09f, 0.032f);

    // Light 4
    addLight(glm::vec3(80.0f, 80.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 1.0f, 0.05f, 0.032f);

    // Light 5
    addLight(glm::vec3(0.0f, 80.0f, -80.0f), glm::vec3(0.0f, 0.0f, 1.0f), 1.0f, 0.05f, 0.032f);

    // Light 6
    addLight(glm::vec3(-80.0f, 80.0f, 0.0f), glm::vec3(0.7f, .3f, 0.1f), 1.0f, 0.05f, 0.032f);


    // Add the 12 blue lights
    const int numLights = 12;
    float y_positions[numLights] = {43.0f, 46.0f, 49.0f, 52.0f, 55.0f, 58.0f, 61.0f, 64.0f, 67.0f, 70.f, 73.f, 76.f};
    float z_positions[numLights] = {0.2f, 4.0f, 7.8f, 11.0f, 14.8f, 17.0f, 20.0f, 23.8f, 27.0f, 30.8f, 34.f, 37.8f};

    for (int i = 0; i < numLights; i++) {
        addLight(
            glm::vec3(-13.2f, y_positions[i], z_positions[i]), // position
            glm::vec3(0.014f, 0.1f, 0.894f), // color (Blue)
            1.0f, 0.009f, 0.032f // attenuation
        );
    }

    const int numEasterLights = 8;
    float x_easter_positions[numEasterLights] = {
            156.4f, 163.0f, -106.876f, -177.618f, 74.427f, -192.695f, -186.862f, -8.47f
        };
    float y_easter_positions[numEasterLights] = {
            47.0f, 56.7f, 44.178f, 48.346f, 31.52f, 78.42f, 55.27f, 46.27f
        };
    float z_easter_positions[numEasterLights] = {
            -109.f, 81.12f, 210.57f, 128.19f, 220.386f, 82.363f, -121.111f, -255.49f
        };

    for (int i = 0; i < numEasterLights; i++) {
        addLight(
            glm::vec3(x_easter_positions[i], y_easter_positions[i], z_easter_positions[i]), // position
            glm::vec3(0.9f, 0.9f, 0.9f),
            1.0f, 0.009f, 0.016f // attenuation
        );
    }


    for (int i = 0; i < MAX_LIGHTS; i++) {
        std::string i_str = std::to_string(i);
        std::string posName = "lights[" + i_str + "].position";
        std::string colName = "lights[" + i_str + "].color";
        std::string constName = "lights[" + i_str + "].constant";
        std::string linName = "lights[" + i_str + "].linear";
        std::string quadName = "lights[" + i_str + "].quadratic";

        pointLightLocs[i].position = glGetUniformLocation(myCustomShader.shaderProgram, posName.c_str());
        pointLightLocs[i].color = glGetUniformLocation(myCustomShader.shaderProgram, colName.c_str());
        pointLightLocs[i].constant = glGetUniformLocation(myCustomShader.shaderProgram, constName.c_str());
        pointLightLocs[i].linear = glGetUniformLocation(myCustomShader.shaderProgram, linName.c_str());
        pointLightLocs[i].quadratic = glGetUniformLocation(myCustomShader.shaderProgram, quadName.c_str());
    }

    for (int i = 0; i < lightsCount; i++) {
        glUniform3fv(pointLightLocs[i].color, 1, glm::value_ptr(pointLights[i].color));
        glUniform1f(pointLightLocs[i].constant, pointLights[i].constant);
        glUniform1f(pointLightLocs[i].linear, pointLights[i].linear);
        glUniform1f(pointLightLocs[i].quadratic, pointLights[i].quadratic);

        if (i == 0) {
            glUniform3fv(pointLightLocs[0].position, 1, glm::value_ptr(glm::vec3(0.0f)));
        }
        else {
            glm::vec3 lightPosEye = glm::vec3(view * glm::vec4(pointLights[i].position, 1.0f));
            glUniform3fv(pointLightLocs[i].position, 1, glm::value_ptr(lightPosEye));
        }
    }

    // Send the ACTUAL number of defined lights to the shader
    GLuint numLightsLoc = glGetUniformLocation(myCustomShader.shaderProgram, "numLights");
    glUniform1i(numLightsLoc, lightsCount); // Send 19
}


void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    myModel.Draw(myCustomShader);
}


void mainLoop() {
    while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
        renderScene();

        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());
    }
}

int main(int argc, const char* argv[]) {
    try {
        myWindow.Create(1280, 720, "Bulevardul Oltenia 34");
    }
    catch (const std::runtime_error& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    initOpenGLState();
    initObjects();
    initShaders();
    initUniforms();

    // initial mouse position
    int newWidth, newHeight;
    glfwGetWindowSize(myWindow.getWindow(), &newWidth, &newHeight);
    glfwSetCursorPos(myWindow.getWindow(), newWidth / 2.0, newHeight / 2.0);
    lastX = newWidth / 2.0;
    lastY = newHeight / 2.0;


    mainLoop();

    myWindow.Delete();

    return 0;
}
