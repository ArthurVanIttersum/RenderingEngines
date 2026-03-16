#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <windows.h>
#include <psapi.h>
#include <sstream>
#include <algorithm>
#include "core/mesh.h"
#include "core/assimpLoader.h"
#include "core/texture.h"
#include "core/Camera.h"
#include "core/Light.h"
#include "core/material.h"
#include "core/GameObject.h"
#include "core/PostProcessStep.h"
#include "core/FrameBuffer.h"
#include "core/ShaderProgram.h"

//#define MAC_CLION
#define VSTUDIO

#ifdef MAC_CLION
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#endif

#ifdef VSTUDIO
// Note: install imgui with:
//     ./vcpkg.exe install imgui[glfw-binding,opengl3-binding]
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#endif

int g_width = 800;
int g_height = 600;
bool sizeChanged = false;

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebufferSizeCallback(GLFWwindow *window,
                             int width, int height) {
    g_width = width;
    g_height = height;
    glViewport(0, 0, width, height);
    sizeChanged = true;
}

std::string readFileToString(const std::string &filePath) {
    std::ifstream fileStream(filePath, std::ios::in);
    if (!fileStream.is_open()) {
        printf("Could not open file: %s\n", filePath.c_str());
        return "";
    }
    std::stringstream buffer;
    buffer << fileStream.rdbuf();
    return buffer.str();
}

GLuint generateShader(const std::string &shaderPath, GLuint shaderType) {
    printf("Loading shader: %s\n", shaderPath.c_str());
    const std::string shaderText = readFileToString(shaderPath);
    const GLuint shader = glCreateShader(shaderType);
    const char *s_str = shaderText.c_str();
    glShaderSource(shader, 1, &s_str, nullptr);
    glCompileShader(shader);
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("Error! Shader issue [%s]: %s\n", shaderPath.c_str(), infoLog);
    }
    return shader;
}



int main() {
    glfwInit();
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(g_width, g_height, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    //Setup platforms
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 400");

    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //make basic objects
    std::shared_ptr<core::Camera> mainCamera = std::make_shared<core::Camera>(glm::vec2(static_cast<float>(g_width), static_cast<float>(g_height)));
    std::shared_ptr<core::Light> firstLight = std::make_shared<core::Light>(glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), glm::vec4(1, 1, 1, 1), core::pointLight);
    std::shared_ptr<core::Texture> cmgtGatoTexture = std::make_shared<core::Texture>("textures/CMGaTo_crop.png");

    //make shaders
    const GLuint modelVertexShader = generateShader("shaders/modelVertex.vs", GL_VERTEX_SHADER);
    const GLuint postProcessingVertexShader = generateShader("shaders/vertex.vs", GL_VERTEX_SHADER);
    const GLuint fragmentShader = generateShader("shaders/fragment.fs", GL_FRAGMENT_SHADER);
    const GLuint textureShader = generateShader("shaders/texture.fs", GL_FRAGMENT_SHADER);
    const GLuint postProcessingFragmentInvert = generateShader("shaders/postProcessInvert.fs", GL_FRAGMENT_SHADER);
    const GLuint postProcessingFragmentGrayScale = generateShader("shaders/postProcessGrayScale.fs", GL_FRAGMENT_SHADER);
    const GLuint postProcessingFragmentEdgeDetect = generateShader("shaders/postProcessEdgeDetect.fs", GL_FRAGMENT_SHADER);
    const GLuint postProcessingFragmentBlur = generateShader("shaders/postProcessBlur.fs", GL_FRAGMENT_SHADER);


    //make shader programs
    std::shared_ptr<core::ShaderProgram> modelShaderProgram = std::make_shared<core::ShaderProgram>(modelVertexShader, fragmentShader);
    std::shared_ptr<core::ShaderProgram> textureShaderProgram = std::make_shared<core::ShaderProgram>(modelVertexShader, textureShader);
    std::shared_ptr<core::ShaderProgram> postProcessingShaderProgramBasic = std::make_shared<core::ShaderProgram>(postProcessingVertexShader, textureShader);
    std::shared_ptr<core::ShaderProgram> postProcessingShaderProgramInvert = std::make_shared<core::ShaderProgram>(postProcessingVertexShader, postProcessingFragmentInvert);
    std::shared_ptr<core::ShaderProgram> postProcessingShaderProgramGrayScale = std::make_shared<core::ShaderProgram>(postProcessingVertexShader, postProcessingFragmentGrayScale);
    std::shared_ptr<core::ShaderProgram> postProcessingShaderProgramEdgeDetect = std::make_shared<core::ShaderProgram>(postProcessingVertexShader, postProcessingFragmentEdgeDetect);
    std::shared_ptr<core::ShaderProgram> postProcessingShaderProgramBlur = std::make_shared<core::ShaderProgram>(postProcessingVertexShader, postProcessingFragmentBlur);

    //delete shaders
    glDeleteShader(modelVertexShader);
    glDeleteShader(postProcessingVertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(textureShader);
    glDeleteShader(postProcessingFragmentInvert);
    glDeleteShader(postProcessingFragmentGrayScale);
    glDeleteShader(postProcessingFragmentEdgeDetect);
    glDeleteShader(postProcessingFragmentBlur);

    // IMGUI variables:
    float ambientIntensity = 0.1f;

    float ambientColor[3] = { 1,1,1 };
    float lightColor[3] = { 1,1,1 };
    int currentSceneInt = 0;
    int postProcessingStep1 = 0;
    int postProcessingStep2 = 0;
    int postProcessingStep3 = 0;


    //materials
    std::shared_ptr<core::Material> basicMaterial = std::make_shared<core::Material>(modelShaderProgram, mainCamera, firstLight, 40, ambientColor, &ambientIntensity, glm::vec4(1, 0.4, 1, 1));
    std::shared_ptr<core::Material> modifiedBasicMaterial = std::make_shared<core::Material>(modelShaderProgram, mainCamera, firstLight, 8, ambientColor, &ambientIntensity, glm::vec4(0.4, 1, 1, 1));
    std::shared_ptr<core::Material> textureMaterial = std::make_shared<core::Material>(textureShaderProgram, mainCamera, cmgtGatoTexture);

    //models

    core::Mesh quad = core::Mesh::generateQuad();
    core::Model quadModel({ quad });
    std::shared_ptr<core::Model> quadModelShared = std::make_shared<core::Model>(quadModel);

    core::Model suzanne = core::AssimpLoader::loadModel("models/nonormalmonkey.obj");
    std::shared_ptr<core::Model> suzanneShared = std::make_shared<core::Model>(suzanne);

    //PostProcessingSteps
    std::vector<std::shared_ptr<core::PostProcessStep>> postProcessingSteps;
    postProcessingSteps.push_back(std::make_shared<core::PostProcessStep>(quadModelShared, postProcessingShaderProgramBasic));
    postProcessingSteps.push_back(std::make_shared<core::PostProcessStep>(quadModelShared, postProcessingShaderProgramInvert));
    postProcessingSteps.push_back(std::make_shared<core::PostProcessStep>(quadModelShared, postProcessingShaderProgramGrayScale));
    postProcessingSteps.push_back(std::make_shared<core::PostProcessStep>(quadModelShared, postProcessingShaderProgramEdgeDetect));
    postProcessingSteps.push_back(std::make_shared<core::PostProcessStep>(quadModelShared, postProcessingShaderProgramBlur));

    //gameobjects
    std::vector<std::vector<std::unique_ptr<core::GameObject>>*> scenes;
    //scene1
    std::vector<std::unique_ptr<core::GameObject>> scene1;
    scenes.push_back(&scene1);

    scene1.push_back(std::make_unique<core::GameObject>(suzanneShared, basicMaterial));
    scene1.push_back(std::make_unique<core::GameObject>(suzanneShared, modifiedBasicMaterial));
    scene1.push_back(std::make_unique<core::GameObject>(quadModelShared, textureMaterial));


    scene1[1]->translate(glm::vec3(2, 0, 0));
    scene1[2]->translate(glm::vec3(0, 0, -2.5));
    scene1[2]->scale(glm::vec3(5, 5, 1));

    //scene2
    std::vector<std::unique_ptr<core::GameObject>> scene2;
    scenes.push_back(&scene2);

    scene2.push_back(std::make_unique<core::GameObject>(suzanneShared, basicMaterial));
    scene2.push_back(std::make_unique<core::GameObject>(suzanneShared, modifiedBasicMaterial));
    scene2.push_back(std::make_unique<core::GameObject>(quadModelShared, basicMaterial));


    scene2[1]->translate(glm::vec3(2, 1, 0));
    scene2[2]->translate(glm::vec3(0, 1, -0.5));
    scene2[2]->objectModel->scale(glm::vec3(5, 5, 1));

    //scene changing
    std::vector<std::unique_ptr<core::GameObject>>* currentScene = &scene1;


    //random stuff

    glm::vec4 clearColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    glClearColor(clearColor.r,
        clearColor.g, clearColor.b, clearColor.a);

    double currentTime = glfwGetTime();
    double finishFrameTime = 0.0;
    float deltaTime = 0.0f;
    float rotationStrength = 100.0f;

    //framebuffers for postprocessing

    std::vector<std::shared_ptr<core::FrameBuffer>> frameBuffers;

    frameBuffers.push_back(std::make_shared<core::FrameBuffer>(static_cast<float>(g_width), static_cast<float>(g_height)));
    frameBuffers.push_back(std::make_shared<core::FrameBuffer>(static_cast<float>(g_width), static_cast<float>(g_height)));
    frameBuffers.push_back(std::make_shared<core::FrameBuffer>(static_cast<float>(g_width), static_cast<float>(g_height)));

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //IMGUI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Raw Engine v2");
        ImGui::SliderFloat("AmbientIntensity", &ambientIntensity, 0, 1);

        ImGui::ColorEdit3("ambientColor", ambientColor, 1);
        ImGui::ColorEdit3("lightColor", lightColor, 1);
        firstLight->color = glm::vec4(lightColor[0], lightColor[1], lightColor[2], 1);

        ImGui::SliderInt("currentSceneInt", &currentSceneInt, 0, 1, "%d", 0);
        currentScene = scenes[currentSceneInt];
        ImGui::SliderInt("PostProcess1", &postProcessingStep1, 0, 4, "%d", 0);
        ImGui::SliderInt("PostProcess2", &postProcessingStep2, 0, 4, "%d", 0);
        ImGui::SliderInt("PostProcess3", &postProcessingStep3, 0, 4, "%d", 0);

        ImGui::Text("FPS: %f", 1 / deltaTime);

        ImGui::End();

        //camera
        processInput(window);
        mainCamera->HandleInput(window, deltaTime);

        //fix screensize
        if (sizeChanged)
        {
            for (int i = 0; i < frameBuffers.size(); i++)
            {
                frameBuffers[i]->ResizeBuffer(static_cast<float>(g_width), static_cast<float>(g_height));//fix screen resolution for post processing
            }
            mainCamera->UpdateScreenSize(glm::vec2(static_cast<float>(g_width), static_cast<float>(g_height)));
            sizeChanged = false;
        }


        //scene stuff
        scene1[0]->rotate(glm::vec3(0.0f, -1.0f, 0.0f), glm::radians(rotationStrength) * static_cast<float>(deltaTime));
        scene1[1]->rotate(glm::vec3(0.0f, -1.0f, 0.0f), glm::radians(rotationStrength) * static_cast<float>(deltaTime));

        frameBuffers[0]->SetCurrentBuffer();//render to buffer1

        for (int i = 0; i < currentScene->size(); i++)
        {
            (*currentScene)[i]->DrawObject();// all drawing in one place
        }

        //postprocessing
        frameBuffers[1]->SetCurrentBuffer();//render to buffer2
        postProcessingSteps[postProcessingStep1]->DrawStep(frameBuffers[0]->colorBufferTexture);
        frameBuffers[2]->SetCurrentBuffer();//render to buffer3
        postProcessingSteps[postProcessingStep2]->DrawStep(frameBuffers[1]->colorBufferTexture);


        glBindFramebuffer(GL_FRAMEBUFFER, 0);//render to screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        postProcessingSteps[postProcessingStep3]->DrawStep(frameBuffers[2]->colorBufferTexture);



        //render stuff
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
        finishFrameTime = glfwGetTime();
        deltaTime = static_cast<float>(finishFrameTime - currentTime);
        currentTime = finishFrameTime;
    }


    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}