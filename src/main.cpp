#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadCubemap(vector<std::string> faces);

void renderQuad();
// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
bool hdr= false;
float exposure= 1.0f;
bool blinn = false;
bool flashLight = false;
bool flashLightKeyPressed = false;
bool flashLightColor=false;
bool blue = false;
bool sunLight=false;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 objectPosition = glm::vec3(0.0f);
    float objectScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 2.5f)) {}
    glm::vec3 dirLightDir = glm::vec3(3.8f, 5.4f, -2.2f);
    glm::vec3 dirLightAmbDiffSpec = glm::vec3(0.3f, 0.3f,0.3f);

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
   // stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader blendShader("resources/shaders/2.model_lighting.vs", "resources/shaders/blending.fs");
    Shader bloomShader("resources/shaders/blur.vs", "resources/shaders/blur.fs");
    Shader hdrShader("resources/shaders/bloom.vs", "resources/shaders/bloom.fs");
    Shader lightShader("resources/shaders/light.vs", "resources/shaders/light.fs");

    // load models
    // -----------
    Model meteorModel("resources/objects/meteor/Iron_Meteorite_30k.obj");
    meteorModel.SetShaderTextureNamePrefix("material.");


    Model moonModel("resources/objects/moon/NASA CGI Moon Kit.obj");
    moonModel.SetShaderTextureNamePrefix("material.");


    Model astroModel("resources/objects/astro/Astronauta.obj");
    astroModel.SetShaderTextureNamePrefix("material.");

    Model craftModel("resources/objects/spacecraft/spaceship_with_station_cabin.obj");
    craftModel.SetShaderTextureNamePrefix("material.");

    Model rocketModel("resources/objects/rocket/rocket.obj");
    rocketModel.SetShaderTextureNamePrefix("material.");

    Model ballModel("resources/objects/ball/13913_Sun_v2_l3.obj");
    ballModel.SetShaderTextureNamePrefix("material.");


    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    // create 2 color buffers (1 for normal rendering, other for brightness threshold values)
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffers[0], 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    // check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }



    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    //skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    //load textures
    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/right-min.png"),
                    FileSystem::getPath("resources/textures/skybox/left-min.png"),
                    FileSystem::getPath("resources/textures/skybox/top-min.png"),
                    FileSystem::getPath("resources/textures/skybox/bottom-min.png"),
                    FileSystem::getPath("resources/textures/skybox/front-min.png"),
                    FileSystem::getPath("resources/textures/skybox/back-min.png")
            };
    stbi_set_flip_vertically_on_load(false);
    unsigned int cubemapTexture = loadCubemap(faces);


    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);
//lights setup
    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0, 0.0);
    pointLight.ambient = glm::vec3(0.9, 0.9, 0.9);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 0.3f;
    pointLight.linear = 0.7f;
    pointLight.quadratic = 0.7f;

    ourShader.use();
    ourShader.setInt("diffuseTexture", 0);
    blendShader.use();
    blendShader.setInt("diffuseTexture", 0);
    bloomShader.use();
    bloomShader.setInt("image", 0);
    hdrShader.use();
    hdrShader.setInt("scene", 0);
    hdrShader.setInt("bloomBlur", 1);
    lightShader.use();
    lightShader.setInt("diffuseTexture", 0);


    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //render scene into floating point framebuffer
        // ____________________________________________________________________________________
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 16.0f);
        ourShader.setInt("blinn", blinn);
        ourShader.setInt("flashLight", flashLight);
        ourShader.setInt("flashLightColor", flashLightColor);
        ourShader.setInt("blue", blue);

        //pointlightsetup
        pointLight.position = glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        //dirlightsetup
        ourShader.setVec3("dirLight.direction", programState->dirLightDir);
        ourShader.setVec3("dirLight.ambient", glm::vec3(programState->dirLightAmbDiffSpec.x));
        ourShader.setVec3("dirLight.diffuse", glm::vec3(programState->dirLightAmbDiffSpec.y));
        ourShader.setVec3("dirLight.specular", glm::vec3(programState->dirLightAmbDiffSpec.z));

        //spotlightsetup
        ourShader.setVec3("spotLight.ambient", 1.0f,1.0f, 1.0f);
        ourShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
        ourShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("spotLight.constant", 1.0f);
        ourShader.setFloat("spotLight.linear", 0.09f);
        ourShader.setFloat("spotLight.quadratic", 0.032f);
        ourShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        ourShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));


        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        //Face culling
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glFrontFace(GL_CW);

        //blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // render the loaded model
        //draw meteor

        glm::mat4 model;

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(7.0f,-20.0f,0.0f));
        model = glm::rotate( model, (float)glfwGetTime()/2, glm::vec3(1,10,0));
        model = glm::translate(model,glm::vec3(4.75f,(-0.93f+ sin(glfwGetTime())/6),-8.5f));
        model = glm::scale(model, glm::vec3 (0.3f));
        ourShader.setMat4("model", model);
        meteorModel.Draw(ourShader);


        //draw 2nd
        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(0.0f,-10.0f,0.0f));
        model = glm::translate(model, glm::vec3(8.0f, sin(glfwGetTime()), 30.0f));
        model = glm::scale(model, glm::vec3(0.4f));

        ourShader.setMat4("model", model);
        meteorModel.Draw(ourShader);

        //draw 3rd
        model = glm::mat4(1.0f);
        model = glm::rotate( model, (float)glfwGetTime()/6, glm::vec3(5,-1,-2));
        model = glm::translate(model,glm::vec3(40.5f,-7.0,-5.0f));
        model = glm::scale(model, glm::vec3(0.3));

        ourShader.setMat4("model", model);
        meteorModel.Draw(ourShader);

        //draw moon
        glm::mat4 model2 = glm::mat4(1.0f);
        model2 = glm::translate(model2,glm::vec3(25.75f,-1.4f, -4.65f));
        model2 = glm::scale(model2, glm::vec3(0.8f));
        model2 = glm::rotate(model2, glm::radians((float) -90.0), glm::vec3(1.0f, 0.0f, 0.0f));
        model2 = glm::rotate(model2, glm::radians((float) 110.0), glm::vec3(0.0f, 0.0f, 1.0f));
        ourShader.setMat4("model", model2);
        moonModel.Draw(ourShader);

        //draw spacecraft
        glm::mat4 model4 = glm::mat4(1.0f);
        model4 = glm::translate(model4,glm::vec3(12.45f, 15.6f, -9.0f));
        model4 = glm::scale(model4, glm::vec3(0.5f));
        model4 =glm::rotate(model4,glm::radians(10.8f), glm::vec3(1.0f ,0.0f, 0.0f));
        model4 =glm::rotate(model4,glm::radians(140.8f), glm::vec3(0.0f ,1.0f, 0.0f));
        model4 =glm::rotate(model4,glm::radians(-35.0f), glm::vec3(0.0f ,0.0f, -1.0f));

        ourShader.setMat4("model", model4);
        craftModel.Draw(ourShader);

        float rocketSpeed = 1.0f;   // Speed of movement

        // Update the rocket's position
        float currentTime = glfwGetTime();
        float rocketHeight = (currentTime) * rocketSpeed;

        glm::mat4 model5 = glm::mat4(1.0f);
        model5=glm::rotate(model5,glm::radians(-25.0f),glm::vec3(0.0f,0.0f,1.0f));
        model5=glm::translate(model5,glm::vec3(0.0f,-45.0f,0.0f));
        model5= glm::translate(model5, glm::vec3(0.0f, rocketHeight, 0.0f));
        model5 = glm::scale(model5, glm::vec3(0.8f));
        ourShader.setMat4("model", model5);
        rocketModel.Draw(ourShader);

        glm::vec3 rocketPosition = glm::vec3(model5[3]); //dont need it now

        glm::vec3 moonModelPosition = glm::vec3(model2[3]);
        glm::vec3 cameraPosition = programState->camera.Position;
        glm::vec3 direction = moonModelPosition - cameraPosition;
        direction = glm::normalize(direction);

        ourShader.setVec3("spotLight.position", moonModelPosition+glm::vec3(-4.0f,5.0f,5.0f));
        ourShader.setVec3("spotLight.direction", direction);

        //draw lightball
        glm::mat4 model6 = glm::mat4(1.0f);
        model6 = glm::translate(model6,glm::vec3(-8.5f,14.0,28.0f));
        model6 = glm::scale(model6, glm::vec3(0.02f));
        lightShader.use();
        lightShader.setMat4("projection", projection);
        lightShader.setMat4("view", view);
        lightShader.setVec3("viewPosition", programState->camera.Position);
        lightShader.setFloat("material.shininess", 64.0f);
        lightShader.setInt("blinn", blinn);

        //pointlightsetup
        pointLight.position = glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
        lightShader.setVec3("pointLight.position", pointLight.position);
        lightShader.setVec3("pointLight.ambient", pointLight.ambient);
        lightShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        lightShader.setVec3("pointLight.specular", pointLight.specular);
        lightShader.setFloat("pointLight.constant", pointLight.constant);
        lightShader.setFloat("pointLight.linear", pointLight.linear);
        lightShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        //dirlightsetup
        lightShader.setVec3("dirLight.direction", programState->dirLightDir);
        lightShader.setVec3("dirLight.ambient", glm::vec3(programState->dirLightAmbDiffSpec.x));
        lightShader.setVec3("dirLight.diffuse", glm::vec3(programState->dirLightAmbDiffSpec.y));
        lightShader.setVec3("dirLight.specular", glm::vec3(programState->dirLightAmbDiffSpec.z));

        //spotlightsetup
        lightShader.setVec3("spotLight.ambient", 1.0f,1.0f, 1.0f);
        lightShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
        lightShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        lightShader.setFloat("spotLight.constant", 1.0f);
        lightShader.setFloat("spotLight.linear", 0.09f);
        lightShader.setFloat("spotLight.quadratic", 0.032f);
        lightShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(150.5f)));
        lightShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(150.0f)));

        glm::vec3 ballModelPosition = glm::vec3(model6[3]);
        direction = ballModelPosition - cameraPosition;
        direction = glm::normalize(direction);

        lightShader.setVec3("spotLight.position", ballModelPosition);
        lightShader.setVec3("spotLight.direction", direction);

        lightShader.setMat4("model", model6);
        ballModel.Draw(lightShader);

        //i want to put something transparently into space

        blendShader.use();
        pointLight.position = glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
        blendShader.setVec3("pointLight.position", pointLight.position);
        blendShader.setVec3("pointLight.ambient", pointLight.ambient);
        blendShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        blendShader.setVec3("pointLight.specular", pointLight.specular);
        blendShader.setFloat("pointLight.constant", pointLight.constant);
        blendShader.setFloat("pointLight.linear", pointLight.linear);
        blendShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        blendShader.setVec3("viewPosition", programState->camera.Position);
        blendShader.setFloat("material.shininess", 32.0f);
        blendShader.setMat4("projection", projection);
        blendShader.setMat4("view", view);

        blendShader.setVec3("dirLight.direction", programState->dirLightDir);
        blendShader.setVec3("dirLight.ambient", glm::vec3(programState->dirLightAmbDiffSpec.x));
        blendShader.setVec3("dirLight.diffuse", glm::vec3(programState->dirLightAmbDiffSpec.y));
        blendShader.setVec3("dirLight.specular", glm::vec3(programState->dirLightAmbDiffSpec.z));
        //draw astronaut

        glm::mat4 model3 = glm::mat4(1.0f);
        static float angle = 0.0f;
        angle += 0.5f; // rotation speed
        model3 = glm::translate(model3, glm::vec3(13.0f, 0.0f, 0.0f));
        model3 = glm::rotate(model3, glm::radians(angle), glm::vec3(1.0f, 1.0f, 1.0f));
        model3 = glm::scale(model3, glm::vec3(1.5f));
        blendShader.setMat4("model", model3);
        astroModel.Draw(ourShader);



//        if (programState->ImGuiEnabled)
      //      DrawImGui(programState);

        //draw skybox
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 5;
        bloomShader.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            bloomShader.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
        //____________________________________________________________________________________________________
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        hdrShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        hdrShader.setInt("hdr", hdr);
        hdrShader.setFloat("exposure", exposure);
        renderQuad();

        if(programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteVertexArrays(1, &skyboxVAO);
    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

/*
    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("object position", (float*)&programState->objectPosition);
        ImGui::DragFloat("object scale", &programState->objectScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }
*/
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if(glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS){

        hdr= !hdr;
    }
    if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS){

        if(exposure >= 0.5)
            exposure-= 0.1;
    }
    if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {

        if (exposure <=0.9)
            exposure += 0.1;
    }
    if (key == GLFW_KEY_B && action == GLFW_PRESS)
        blinn = !blinn;

    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
    {
        flashLight = !flashLight;
        flashLightKeyPressed = true;
        if(!flashLight)
            flashLightColor= false;
        if(!flashLight)
            blue= false;

    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE)
    {
        flashLightKeyPressed = false;
    }

    if (key == GLFW_KEY_F && action == GLFW_PRESS && flashLight)
        flashLightColor = !flashLightColor;


    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && flashLight)
    {
        blue=!blue;
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
        blue = false;
    }

    if (key == GLFW_KEY_N && action == GLFW_PRESS)
        sunLight=!sunLight;


}
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
// renderQuad() renders a 1x1 XY quad in NDC
// __________________________________________________________________________________________
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}