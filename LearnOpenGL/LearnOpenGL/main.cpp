#include <glad/glad.h>
#include <glfw3.h>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include "Model.h"
#include "Shader.h"
#include "Texture.h"
#include "Camera.h"

void ProcessInput(GLFWwindow * window, float deltaTime);
void MouseCallback(GLFWwindow * window, double xPos, double yPos);
void ScrollCallback(GLFWwindow * window, double xOffset, double yOffset);
void RenderFrameBufferQuad();
void RenderDepthFramebuffer();
void RenderQuad();
void RenderCube();
float Lerp(float a, float b, float f);
glm::vec3 Lerp3D(glm::vec3 a, glm::vec3 b, float time);

Camera camera(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 10.0f, 800.0f, 600.0f);

int winWidth = 800, winHeight = 600;

enum MoveState {
    IDLE, MOVING, RUNNING, AIMING
};

void FrameBufferCallback(GLFWwindow * window, int width, int height) {
    glViewport(0, 0, width, height);
    winWidth = width;
    winHeight = height;
}

glm::vec3 rifleOffset(0.9f, -1.1f, -2.4f);
float rifleScale = 0.032f;

bool moving = false, running = false, aiming = false;
bool moveKeyPressed[6] = { false };
MoveState moveState = MoveState::IDLE; //Movestates match with other array index to work together
glm::vec3 bobAmount[4] = { glm::vec3(0.8f, 0.6f, 0.9f), glm::vec3(0.7f, 1.5f, 0.1f), 
glm::vec3(2.0f, 4.0f, 1.5f), glm::vec3(0.2f, 0.1f, 0.1f) };
float bobVelocity[4] = { 2.0f, 5.5f, 10.0f, 2.0f };
float cameraSpeed[4] = { 10.0f, 10.0f, 15.0f, 5.0f };

bool movingIntoPlace = false; //Indicates if it's currently going from aiming to standard & vice-versa
float moveTime = 0.0f;
glm::vec3 originalRiflePos = glm::vec3(0.0f);

float dirXTimer = 0.0f;
bool increaseDirTime = true; //Whether the timer for for dirX should go increase or decrease;
glm::vec3 dirLightPos = glm::vec3(-20.0f, 15.0f, -1.0f);

struct PointLight {
    glm::vec3 m_Pos;
    glm::vec3 m_Color;

    float m_SpecShine;
    float m_Linear;
    float m_Quadratic;
};

int main(void)
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow * window;
      
    if (!glfwInit()) {
        std::cout << "Error: Failed to initlize GLFW" << std::endl;
        return -1;
    }

    window = glfwCreateWindow(winWidth, winHeight, "Hello World", NULL, NULL);

    if (!window) {
        std::cout << "Error: Failed to create the window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Error: Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSetFramebufferSizeCallback(window, FrameBufferCallback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetScrollCallback(window, ScrollCallback);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    float currentFrame = 0.0f;

    //Setting up gBuffer / other FBOs
    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    //Position color buffer
    Texture gPos(GL_RGBA16F, GL_RGBA, winWidth, winHeight, GL_FLOAT, GL_NEAREST, true, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPos.getProgram(), 0);

    //Normal color buffer
    Texture gNormal(GL_RGBA16F, GL_RGBA, winWidth, winHeight, GL_FLOAT, GL_NEAREST, false, GL_REPEAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal.getProgram(), 0);

    //Albedo and specular color buffer
    Texture gAlbedoSpec(GL_RGBA, GL_RGBA, winWidth, winHeight, GL_UNSIGNED_BYTE, GL_NEAREST, false, GL_REPEAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec.getProgram(), 0);

    //Position view space color buffer
    Texture gPosViewSpace(GL_RGBA16F, GL_RGBA, winWidth, winHeight, GL_FLOAT, GL_NEAREST, true, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gPosViewSpace.getProgram(), 0);

    //Normal view space color buffer
    Texture gNormalViewSpace(GL_RGBA16F, GL_RGBA, winWidth, winHeight, GL_FLOAT, GL_NEAREST, false, GL_REPEAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, gNormalViewSpace.getProgram(), 0);

    unsigned int attachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
    glDrawBuffers(5, attachments);

    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, winWidth, winHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    //SSAO Framebuffer
    unsigned int ssaoFBO;
    glGenFramebuffers(1, &ssaoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    Texture ssaoColorBuffer(GL_RED, GL_RED, winWidth, winHeight, GL_FLOAT, GL_NEAREST, false, GL_REPEAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer.getProgram(), 0);

    //SSAO Blur framebuffer
    unsigned int ssaoBlurFBO;
    glGenFramebuffers(1, &ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    Texture ssaoBlurColor(GL_RED, GL_RED, winWidth, winHeight, GL_FLOAT, GL_NEAREST, false, GL_REPEAT, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoBlurColor.getProgram(), 0);

    //HDR Framebuffer
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    unsigned int hdrColorBuffer[2];
    glGenTextures(2, hdrColorBuffer);

    for (unsigned int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, hdrColorBuffer[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, winWidth, winHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, hdrColorBuffer[i], 0);
    }

    unsigned int hdrRBODepth;
    glGenRenderbuffers(1, &hdrRBODepth);
    glBindRenderbuffer(GL_RENDERBUFFER, hdrRBODepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, winWidth, winHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, hdrRBODepth);

    unsigned int hdrAttachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, hdrAttachments);

    //Ping pong fbo for the bloom
    unsigned int pingpongFBO[2];
    unsigned int pingpongColor[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColor);

    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColor[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, winWidth, winHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColor[i], 0);
    }

    //Shadow map buffer
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    
    unsigned int dirDepthMap;
    glGenTextures(1, &dirDepthMap);
    glBindTexture(GL_TEXTURE_2D, dirDepthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dirDepthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    //Shadow map for spotlight
    unsigned int spotLightFBO;
    glGenFramebuffers(1, &spotLightFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, spotLightFBO);

    unsigned int spotlightDepthMap;
    glGenTextures(1, &spotlightDepthMap);
    glBindTexture(GL_TEXTURE_2D, spotlightDepthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, spotlightDepthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    //Shadow map for point lights
    unsigned int pointLightShadowFBO[2];
    unsigned int pointLightCubemap[2];
    glGenFramebuffers(2, pointLightShadowFBO);
    glGenTextures(2, pointLightCubemap);

    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pointLightShadowFBO[i]);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pointLightCubemap[i]);

        for (unsigned int j = 0; j < 6; j++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointLightCubemap[i], 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //Initilizing varibles needed for SSAO
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoKernel;
    for (unsigned int i = 0; i < 64; i++) {
        glm::vec3 sample(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator));

        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = static_cast<float>(i) / 64.0f;

        //So samples are closer to the origin point
        scale = Lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    //Generating the noise for SSAO
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++) {
        glm::vec3 noise(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, 0.0f);
        ssaoNoise.push_back(noise);
    }

    Texture noiseTexture(GL_RGBA32F, GL_RGB, 4, 4, GL_FLOAT, GL_NEAREST, true, GL_REPEAT, &ssaoNoise[0]);

    //Loading textures and shaders
    Shader lightCubeShader("LightSourceVertex.glsl", "LightSourceFragment.glsl");
    Shader modelShader("ModelVertex.glsl", "ModelFragment.glsl");
    Shader spriteShader("SpriteVertex.glsl", "SpriteFragment.glsl");
    Shader skyboxShader("SkyboxVertex.glsl", "SkyboxFragment.glsl");
    Shader defShader("DefVertex.glsl", "DefFragment.glsl");
    Shader ssaoShader("DefVertex.glsl", "SSAOFragment.glsl");
    Shader ssaoBlurShader("DefVertex.glsl", "SSAOBlurFragment.glsl");
    Shader hdrShader("DefVertex.glsl", "HDRFragment.glsl");
    Shader bloomBlurShader("DefVertex.glsl", "BloomBlurFragment.glsl");
    Shader dirDepthShader("DirDepthVertex.glsl", "DirDepthFragment.glsl");
    Shader plDepthShader("PLVertex.glsl", "PLGeometry.glsl", "PLFragment.glsl");
    Shader depthVis("DefVertex.glsl", "DepthMapVisFragment.glsl");

    Texture hitmarkerNear("hitmarkerNear.png");
    Texture hitmarkerFar("hitmarkerFar.png");
    Texture hitmarkerRun("hitmarkerRunning.png");

    std::vector<std::string> faces = {
        "Skybox/right.jpg",
        "Skybox/left.jpg",
        "Skybox/top.jpg",
        "Skybox/bottom.jpg",
        "Skybox/front.jpg",
        "Skybox/back.jpg"
    };

    Texture skyboxTexture(faces);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)winWidth / (float)winHeight, 0.1f, 100.0f);
    glm::mat4 spriteProjection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f, -1.0f, 1.0f);

    Model sponza("Models/sponza_scene/scene.gltf");
    Model rifle("Models/308_machine_gun/scene.gltf");
    Model lamp("Models/street_lamp/scene.gltf");
    Model subGun("Models/submachine_gun/scene.gltf");

    //Setting up the uniform buffer (for a perspective projection and raw camera view)
    unsigned int uboMatrix;
    glGenBuffers(1, &uboMatrix);
    glBindBuffer(GL_UNIFORM_BUFFER, uboMatrix);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, nullptr, GL_STATIC_DRAW);

    //Get block index in that shader
    unsigned int matrixIndexModel = glGetUniformBlockIndex(modelShader.getProgram(), "Matrix");
    unsigned int matrixIndexLightbox = glGetUniformBlockIndex(lightCubeShader.getProgram(), "Matrix");

    //Tell we're going to bind the indexes to the binding point 0
    glUniformBlockBinding(modelShader.getProgram(), matrixIndexModel, 0);
    glUniformBlockBinding(lightCubeShader.getProgram(), matrixIndexLightbox, 0);

    //Set to the binding point 0
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboMatrix);

    std::vector<PointLight> pointLights{ {glm::vec3(-10.0f, 3.0f, 0.0f), glm::vec3(25.0f), 128.0f, 0.022f, 0.0019f}, {glm::vec3(15.0f, 5.0f, 3.0f), glm::vec3(40.0f), 128.0f, 0.022f, 0.0019f} };

    //Set up information for the directional lighting to be used in shadow mapping
    float dirNearPlane = 1.0f, dirFarPlane = 70.0f;
    glm::mat4 dirLightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, dirNearPlane, dirFarPlane);
    glm::mat4 dirLightView = glm::lookAt(dirLightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 dirMatrix = dirLightProjection * dirLightView;

    //Set up the information for the spotlight shadow map
    float spotlightNearPlane = 1.0f, spotlightFar = 40.0f;
    glm::mat4 spotlightProjection = glm::perspective(glm::radians(45.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, spotlightNearPlane, spotlightFar);
    glm::mat4 spotlightView = glm::lookAt(glm::vec3(22.5f, 16.0f, -1.0f), glm::vec3(22.5f, 16.0f, -1.0f) - glm::vec3(0.01f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 spotMatrix = spotlightProjection * spotlightView;

    //Set up information for the point lights
    float aspect = (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT;
    float plNear = 1.0f;
    float plFar = 25.0f;
    glm::mat4 plProj = glm::perspective(glm::radians(90.0f), aspect, plNear, plFar);

    std::vector<std::vector<glm::mat4>> shadowTransforms;
    std::vector<glm::mat4> row;

    for (unsigned int i = 0; i < 2; i++) {
        std::vector<glm::mat4> row;
        row.push_back(plProj * glm::lookAt(pointLights[i].m_Pos, pointLights[i].m_Pos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        row.push_back(plProj * glm::lookAt(pointLights[i].m_Pos, pointLights[i].m_Pos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        row.push_back(plProj * glm::lookAt(pointLights[i].m_Pos, pointLights[i].m_Pos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        row.push_back(plProj * glm::lookAt(pointLights[i].m_Pos, pointLights[i].m_Pos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
        row.push_back(plProj * glm::lookAt(pointLights[i].m_Pos, pointLights[i].m_Pos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        row.push_back(plProj * glm::lookAt(pointLights[i].m_Pos, pointLights[i].m_Pos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
   
        shadowTransforms.push_back(row);
    }

    float submachineRotation = 0.0f, subYOffset = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        ProcessInput(window, deltaTime);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindBuffer(GL_UNIFORM_BUFFER, uboMatrix);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(camera.UpdateCamera()));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glm::mat4 model = glm::mat4(1.0f);
        submachineRotation += deltaTime * 50.0f;

        if (submachineRotation > 360.0f) {
            submachineRotation = 0.0f;
        }

        subYOffset = sin((float)glfwGetTime() * 2.5f) * 0.4f;

        dirXTimer += (increaseDirTime) ? deltaTime : -deltaTime;

        if (dirXTimer >= 15.0f && increaseDirTime) {
            dirXTimer = 15.0f;
            increaseDirTime = false;
        }
        else if (dirXTimer <= 0.0f && !increaseDirTime) { 
            dirXTimer = 0.0f;
            increaseDirTime = true;
        }

        dirLightPos.x = Lerp(-27.2f, 25.15f, dirXTimer / 15.0f);
        glm::mat4 dirLightView = glm::lookAt(dirLightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 dirMatrix = dirLightProjection * dirLightView;

        //First pass, filling in the depth map for the directional light
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        dirDepthShader.Bind();
        dirDepthShader.SetMat4("u_LightSpaceMatrix", dirMatrix);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f));
        model = glm::scale(model, glm::vec3(0.03f));
        dirDepthShader.SetMat4("u_Model", model);
        sponza.Draw(dirDepthShader, true);

        //Was aiming but now the button has been released. Go back to standard position
        if (movingIntoPlace) {
            moveTime += deltaTime * 3.0f;

            if (moveTime >= 1.0f) {
                movingIntoPlace = false;
                moveTime = 0.0f;
            }
            else {
                rifleOffset = Lerp3D(originalRiflePos, (aiming) ? glm::vec3(0.05f, -0.95f, -0.35f) : glm::vec3(0.9f, -1.1f, -2.4f), moveTime);
            }
        }

        if (aiming && !moveKeyPressed[5] && !movingIntoPlace) {
            aiming = false;
            movingIntoPlace = true;
            originalRiflePos = rifleOffset;
        }
        else if (!aiming && moveKeyPressed[5] && !movingIntoPlace) { //Was in standard position, but now it's aiming
            aiming = true;
            movingIntoPlace = true;
            originalRiflePos = rifleOffset;
        }

        if (!movingIntoPlace) {
            int index = (aiming) ? 3 : static_cast<int>(moveState);

            float bobOscillate = glm::sin(glfwGetTime() * bobVelocity[index]) * 0.001f;
            rifleOffset += bobOscillate * bobAmount[index];
            camera.SetSpeed(cameraSpeed[index]);
        }

        model = glm::mat4(1.0);
        model = glm::translate(model, rifleOffset);
        if (running) {
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(295.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(345.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        else {
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        model = glm::scale(model, glm::vec3(rifleScale));

        dirDepthShader.SetMat4("u_Model", model);
        rifle.Draw(dirDepthShader, true);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(24.0f, 0.0f, -1.0f));
        model = glm::scale(model, glm::vec3(0.4f));
        dirDepthShader.SetMat4("u_Model", model);
        lamp.Draw(dirDepthShader, true);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(19.0f, 1.5f, -1.0f));
        model = glm::rotate(model, glm::radians(submachineRotation), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.025f));
        plDepthShader.SetMat4("u_Model", model);
        subGun.Draw(dirDepthShader, true);

        for (unsigned int i = 0; i < pointLights.size(); i++) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, pointLights[i].m_Pos);
            model = glm::scale(model, glm::vec3(0.5f));

            dirDepthShader.SetMat4("u_Model", model);

            RenderCube();
        }

        dirDepthShader.UnBind();

        //Second pass, filling in the depth map for the white point light
        glBindFramebuffer(GL_FRAMEBUFFER, pointLightShadowFBO[0]);
        glClear(GL_DEPTH_BUFFER_BIT);
        plDepthShader.Bind();

        for (unsigned int i = 0; i < 6; i++) {
            plDepthShader.SetMat4("u_ShadowMatrices[" + std::to_string(i) + "]", shadowTransforms[0][i]);
        }

        plDepthShader.SetFloat("u_FarPlane", plFar);
        plDepthShader.SetVec3("u_LightPos", pointLights[0].m_Pos);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f));
        model = glm::scale(model, glm::vec3(0.03f));
        plDepthShader.SetMat4("u_Model", model);
        sponza.Draw(plDepthShader, true);

        model = glm::mat4(1.0);
        model = glm::translate(model, rifleOffset);
        if (running) {
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(295.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(345.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        else {
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        model = glm::scale(model, glm::vec3(rifleScale));

        plDepthShader.SetMat4("u_Model", model);
        rifle.Draw(plDepthShader, true);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(24.0f, 0.0f, -1.0f));
        model = glm::scale(model, glm::vec3(0.4f));
        plDepthShader.SetMat4("u_Model", model);
        lamp.Draw(plDepthShader, true);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(19.0f, 1.5f, -1.0f));
        model = glm::rotate(model, glm::radians(submachineRotation), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.025f));
        plDepthShader.SetMat4("u_Model", model);
        subGun.Draw(plDepthShader, true);

        for (unsigned int i = 0; i < pointLights.size(); i++) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, pointLights[i].m_Pos);
            model = glm::scale(model, glm::vec3(0.5f));

            plDepthShader.SetMat4("u_Model", model);

            RenderCube();
        }

        //2.5 pass, filling in the depth map for the red light
        glBindFramebuffer(GL_FRAMEBUFFER, pointLightShadowFBO[1]);
        glClear(GL_DEPTH_BUFFER_BIT);

        for (unsigned int i = 0; i < 6; i++) {
            plDepthShader.SetMat4("u_ShadowMatrices[" + std::to_string(i) + "]", shadowTransforms[1][i]);
        }

        plDepthShader.SetFloat("u_FarPlane", plFar);
        plDepthShader.SetVec3("u_LightPos", pointLights[1].m_Pos);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f));
        model = glm::scale(model, glm::vec3(0.03f));
        plDepthShader.SetMat4("u_Model", model);
        sponza.Draw(plDepthShader, true);

        model = glm::mat4(1.0);
        model = glm::translate(model, rifleOffset);
        if (running) {
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(295.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(345.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        else {
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        model = glm::scale(model, glm::vec3(rifleScale));
        plDepthShader.SetMat4("u_Model", model);
        rifle.Draw(plDepthShader, true);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(24.0f, 0.0f, -1.0f));
        model = glm::scale(model, glm::vec3(0.4f));
        plDepthShader.SetMat4("u_Model", model);
        lamp.Draw(plDepthShader, true);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(19.0f, 1.5f, -1.0f));
        model = glm::rotate(model, glm::radians(submachineRotation), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.025f));
        plDepthShader.SetMat4("u_Model", model);
        subGun.Draw(plDepthShader, true);

        for (unsigned int i = 0; i < pointLights.size(); i++) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, pointLights[i].m_Pos);
            model = glm::scale(model, glm::vec3(0.5f));

            plDepthShader.SetMat4("u_Model", model);

            RenderCube();
        }

        plDepthShader.UnBind();

        //Third pass, filling in the depth map for the spotlight
        glBindFramebuffer(GL_FRAMEBUFFER, spotLightFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        dirDepthShader.Bind();
        dirDepthShader.SetMat4("u_LightSpaceMatrix", spotMatrix);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f));
        model = glm::scale(model, glm::vec3(0.03f));
        dirDepthShader.SetMat4("u_Model", model);
        sponza.Draw(dirDepthShader, true);

        model = glm::mat4(1.0);
        model = glm::translate(model, rifleOffset);
        if (running) {
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(295.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(345.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        else {
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        model = glm::scale(model, glm::vec3(rifleScale));
        dirDepthShader.SetMat4("u_Model", model);
        rifle.Draw(dirDepthShader, true);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(24.0f, 0.0f, -1.0f));
        model = glm::scale(model, glm::vec3(0.4f));
        dirDepthShader.SetMat4("u_Model", model);
        lamp.Draw(dirDepthShader, true);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(20.25f, 6.95f, -1.0f)); 
        model = glm::rotate(model, glm::radians(submachineRotation), glm::vec3(0.0f, 1.0f, 0.0f));
        //Interesting how this works, basically as the (0.4f - subYOffset) equation approaches 0.7f the shadow becomes smaller
        //The 0.015f is to make the number even smaller and subtract that from the original size
        model = glm::scale(model, glm::vec3(0.025f) - (0.015f * (0.4f - subYOffset))); //This works
        dirDepthShader.SetMat4("u_Model", model);
        subGun.Draw(dirDepthShader, true);

        for (unsigned int i = 0; i < pointLights.size(); i++) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, pointLights[i].m_Pos);
            model = glm::scale(model, glm::vec3(0.5f));

            dirDepthShader.SetMat4("u_Model", model);

            RenderCube();
        }

        dirDepthShader.UnBind();

        //Fourth pass | filling in the gBuffer
        glViewport(0, 0, winWidth, winHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        modelShader.Bind();

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f));
        model = glm::scale(model, glm::vec3(0.03f));

        modelShader.SetInt("u_ViewApplied", true);
        modelShader.SetFloat("u_OpacityLimit", 0.5f);
        modelShader.SetMat4("u_Model", model);
        modelShader.SetVec3("u_DayColor", Lerp3D(glm::vec3(1.0f), glm::vec3(0.282f, 0.203f, 0.56f), dirXTimer / 15.0f));

        sponza.Draw(modelShader);   

        model = glm::mat4(1.0);
        model = glm::translate(model, rifleOffset);
        if (running) {
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(295.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(345.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        else {
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        model = glm::scale(model, glm::vec3(rifleScale));

        modelShader.SetInt("u_ViewApplied", false);
        modelShader.SetFloat("u_OpacityLimit", 0.0f);
        modelShader.SetMat4("u_Model", model);
        rifle.Draw(modelShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(24.0f, 0.0f, -1.0f));
        model = glm::scale(model, glm::vec3(0.4f));
        modelShader.SetInt("u_ViewApplied", true);
        modelShader.SetFloat("u_OpacityLimit", 0.0f);
        modelShader.SetMat4("u_Model", model);
        lamp.Draw(modelShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(19.0f, 1.5f + subYOffset, -1.0f));
        model = glm::rotate(model, glm::radians(submachineRotation), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.025f));
        modelShader.SetMat4("u_Model", model);
        subGun.Draw(modelShader);

        modelShader.UnBind();

        //Fifth pass, render the SSAO texture
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        ssaoShader.Bind();

        for (unsigned int i = 0; i < 64; i++) {
            ssaoShader.SetVec3("u_Samples[" + std::to_string(i) + "]", ssaoKernel[i]);
        }

        ssaoShader.SetInt("u_gbPos", 0);
        ssaoShader.SetInt("u_gbNormal", 1);
        ssaoShader.SetInt("u_TexNoise", 2);
        ssaoShader.SetMat4("u_Projection", projection);

        gPosViewSpace.Bind(0);
        gNormalViewSpace.Bind(1);
        noiseTexture.Bind(2);

        RenderFrameBufferQuad();

        gPos.UnBind();
        gNormal.UnBind();
        noiseTexture.UnBind();
        ssaoShader.UnBind();

        //5.5 pass, blur the SSAO Texture
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        ssaoBlurShader.Bind();
        ssaoBlurShader.SetInt("u_SSAOTexture", 0);
        ssaoColorBuffer.Bind(0);

        RenderFrameBufferQuad();

        ssaoColorBuffer.UnBind();
        ssaoBlurShader.UnBind();

        //Fifth pass, lighting stage
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        defShader.Bind();

        //Binding maps
        defShader.SetInt("u_gbPos", 0);
        defShader.SetInt("u_gbNormal", 1);
        defShader.SetInt("u_gbAlbedoSpec", 2);
        defShader.SetInt("u_SSAOTexture", 3);
        defShader.SetInt("u_DirShadowMap", 4);
        defShader.SetInt("u_SpotlightShadowMap", 5);
        defShader.SetInt("u_CubeDepth[0]", 6);
        defShader.SetInt("u_CubeDepth[1]", 7);

        defShader.SetFloat("u_FarPlane", plFar);

        //Binding lighting information
        defShader.SetVec3("u_ViewPos", camera.getCameraPos());
        defShader.SetVec3("u_Dirlight.m_Direction", dirLightPos);
        defShader.SetVec3("u_Dirlight.m_Diffuse", glm::vec3(0.7f));
        defShader.SetVec3("u_Dirlight.m_Specular", glm::vec3(0.3f));
        defShader.SetVec3("u_Dirlight.m_Color", glm::vec3(1.0f));
        defShader.SetFloat("u_Dirlight.m_ShinySpec", 256.0f);
        defShader.SetMat4("u_DirLightMatrix", dirMatrix);

        defShader.SetVec3("u_Spotlight.m_Pos", glm::vec3(22.5f, 16.0f, -1.0f));
        defShader.SetVec3("u_Spotlight.m_Color", glm::vec3(1.0f));
        defShader.SetVec3("u_Spotlight.m_Direction", glm::vec3(0.0f, -1.0f, 0.0f));
        defShader.SetFloat("u_Spotlight.m_Cutoff", glm::cos(glm::radians(12.5f)));
        defShader.SetFloat("u_Spotlight.m_Outercutoff", glm::cos(glm::radians(25.0f)));
        defShader.SetMat4("u_SpotlightMatrix", spotMatrix);

        for (unsigned int i = 0; i < pointLights.size(); i++) {
            defShader.SetVec3("u_PointsLights[" + std::to_string(i) + "].m_Pos", pointLights[i].m_Pos);
            defShader.SetVec3("u_PointsLights[" + std::to_string(i) + "].m_Color", pointLights[i].m_Color);
            defShader.SetFloat("u_PointsLights[" + std::to_string(i) + "].m_SpecShine", pointLights[i].m_SpecShine);
        }

        gPos.Bind(0);
        gNormal.Bind(1);
        gAlbedoSpec.Bind(2);
        ssaoBlurColor.Bind(3);
        
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, dirDepthMap);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, spotlightDepthMap);

        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pointLightCubemap[0]);

        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pointLightCubemap[1]);

        RenderFrameBufferQuad();

        defShader.UnBind();
        gPos.UnBind();
        gNormal.UnBind();
        gAlbedoSpec.UnBind();
        ssaoBlurColor.UnBind();

        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, hdrFBO);

        glBlitFramebuffer(0, 0, winWidth, winHeight, 0, 0, winWidth, winHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

        //Drawing the point lights
        lightCubeShader.Bind();

        for (unsigned int i = 0; i < pointLights.size(); i++) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, pointLights[i].m_Pos);
            model = glm::scale(model, glm::vec3(0.5f));

            lightCubeShader.SetMat4("u_Model", model);
            lightCubeShader.SetVec3("u_Color", pointLights[i].m_Color);

            RenderCube();
        }

        RenderCube();

        lightCubeShader.UnBind();

        //Sixth pass, blurring the bright spots in the scene for the bloom effect
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        bloomBlurShader.Bind();
        bloomBlurShader.SetInt("u_Image", 0);

        bool horizontal = true, firstIter = true;
        unsigned int amount = 10;
        for (unsigned int i = 0; i < amount; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            bloomBlurShader.SetInt("u_Horizontal", horizontal);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, firstIter ? hdrColorBuffer[1] : pingpongColor[!horizontal]);
            RenderFrameBufferQuad();
            horizontal = !horizontal;

            if (firstIter) {
                firstIter = false;
            }
        }

        //Seventh pass, finally outputting it to the screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        hdrShader.Bind();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrColorBuffer[0]);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColor[!horizontal]);

        hdrShader.SetInt("u_Scene", 0);
        hdrShader.SetInt("u_BloomBlur", 1);
        hdrShader.SetFloat("u_Exposure", 1.0f);

        RenderFrameBufferQuad();

        hdrShader.UnBind();

        //Drawing the hitmarker, now drawing with foward rendering
        glCullFace(GL_FRONT);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, hdrFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glBlitFramebuffer(0, 0, winWidth, winHeight, 0, 0, winWidth, winHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //Drawing the skybox
        glDepthFunc(GL_LEQUAL);
        skyboxShader.Bind();
        skyboxTexture.BindCubemap();
        glm::mat4 view = glm::mat4(glm::mat3(camera.UpdateCamera()));

        skyboxShader.SetMat4("u_Projection", projection);
        skyboxShader.SetMat4("u_View", view);
        skyboxShader.SetInt("u_SkyboxTexture", 0);
        skyboxShader.SetVec3("u_DayColor", Lerp3D(glm::vec3(1.0f), glm::vec3(0.282f, 0.203f, 0.56f), dirXTimer / 15.0f));

        RenderCube();

        skyboxTexture.UnbindCubemap();
        skyboxShader.UnBind();
        glDepthFunc(GL_LESS);
        glCullFace(GL_BACK);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glCullFace(GL_FRONT);
        spriteShader.Bind();

        if (running) {
            hitmarkerRun.Bind();
        }
        else if (moving && !aiming) {
            hitmarkerFar.Bind();
        }
        else {
            hitmarkerNear.Bind();
        }

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(winWidth / 2.0f, winHeight / 2.0f, 0.0f));
        model = glm::scale(model, glm::vec3(100.0f));

        spriteShader.SetMat4("u_Projection", spriteProjection);
        spriteShader.SetMat4("u_Model", model);
        spriteShader.SetInt("u_SpriteTexture", 0);
        spriteShader.SetVec3("u_Color", glm::vec3(1.0f));

        RenderQuad();

        glBindTexture(GL_TEXTURE_2D, 0);
        spriteShader.UnBind();
        glEnable(GL_DEPTH_TEST);
        glCullFace(GL_BACK);
        glDisable(GL_BLEND);

        /*//visualizing the depth map
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        depthVis.Bind();

        depthVis.SetInt("u_DepthMap", 0);
        depthVis.SetFloat("u_NearPlane", dirNearPlane);
        depthVis.SetFloat("u_FarPlane", dirFarPlane);
        depthVis.SetInt("u_Perspective", false);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dirDepthMap);

        RenderDepthFramebuffer();

        depthVis.UnBind();*/

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

unsigned int framebufferVAO = 0;
unsigned int framebufferVBO;
void RenderFrameBufferQuad() {
    if (framebufferVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
             -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
              1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
              1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &framebufferVAO);
        glGenBuffers(1, &framebufferVBO);
        glBindVertexArray(framebufferVAO);
        glBindBuffer(GL_ARRAY_BUFFER, framebufferVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }

    glBindVertexArray(framebufferVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

unsigned int depthVAO = 0;
unsigned int depthVBO;
void RenderDepthFramebuffer() {
    if (depthVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
             0.1f,  0.1f, 0.0f, 0.0f, 1.0f,
             0.1f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  0.1f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &depthVAO);
        glGenBuffers(1, &depthVBO);
        glBindVertexArray(depthVAO);
        glBindBuffer(GL_ARRAY_BUFFER, depthVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }

    glBindVertexArray(depthVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
unsigned int quadIBO;
void RenderQuad() {
    if (quadVAO == 0) {
        float quadVertices[] = {
             0.5f,  0.5f,  1.0f, 1.0f,
             0.5f, -0.5f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f,  0.0f, 1.0f
        };

        unsigned int indices[] = {
            0, 1, 3,
            1, 2, 3
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glGenBuffers(1, &quadIBO);

        glBindVertexArray(quadVAO);

        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void RenderCube() {
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };

        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void ProcessInput(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, 1);
    }

    //Camera Input / Movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.Translate(Direction::FOWARD, deltaTime);
        moveKeyPressed[0] = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.Translate(Direction::BACKWARD, deltaTime);
        moveKeyPressed[1] = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.Translate(Direction::LEFT, deltaTime);
        moveKeyPressed[2] = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.Translate(Direction::RIGHT, deltaTime);
        moveKeyPressed[3] = true;
    }

    //Holding down any of the shift keys -> meaning running
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        moveKeyPressed[4] = true;
    }

    //Pressing the right button on the mouse for aiming
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        moveKeyPressed[5] = true;
    }

    //Releasing the shift key so no longer running
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE && moveKeyPressed[4]) {
        moveKeyPressed[4] = false;
    }

    //No longer aiming, letting go of the right button on the mouse
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE && moveKeyPressed[5]) {
        moveKeyPressed[5] = false;
    }

    //Actual player movement (actually just moving the camera)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE && moveKeyPressed[0]) {
        moveKeyPressed[0] = false;
    }
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE && moveKeyPressed[1]) {
        moveKeyPressed[1] = false;
    }
    else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE && moveKeyPressed[2]) {
        moveKeyPressed[2] = false;
    }
    else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE && moveKeyPressed[3]) {
        moveKeyPressed[3] = false;
    }

    moveState = MoveState::IDLE;

    //Check if any of the WASD keys are being pressed. If so -> movement
    if (moveKeyPressed[0] || moveKeyPressed[1] || moveKeyPressed[2] || moveKeyPressed[3]) {
        moveState = MoveState::MOVING;
    }

    //If the player is moving as checked above ^, and the shift key is being pressed and not aiming -> running
    if (moveState == MoveState::MOVING && moveKeyPressed[4] && !moveKeyPressed[5]) {
        moveState = MoveState::RUNNING;
    }

    moving = (moveKeyPressed[0] || moveKeyPressed[1] || moveKeyPressed[2] || moveKeyPressed[3]);
    running = moving && moveKeyPressed[4] && !moveKeyPressed[5]; //To ensure that it's shift is not only being held down but it's moving as well
}

void MouseCallback(GLFWwindow * window, double xPos, double yPos) {
    camera.MouseMovement(xPos, yPos);
}

void ScrollCallback(GLFWwindow * window, double xOffset, double yOffset) {
    //rifleScale += yOffset / 1000.0f;
}

float Lerp(float a, float b, float f) {
    return a + f * (b - a);
}

glm::vec3 Lerp3D(glm::vec3 a, glm::vec3 b, float time) {
    return a * (1.0f - time) + b * time;
}