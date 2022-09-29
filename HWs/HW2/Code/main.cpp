#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <chrono>
#include <random>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/random.hpp>
#include <cmath>
#include "shader.h"
#include "Sphere.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

#define MAX_PARTICLE_PER_GENERATOR 10000
#define MAX_PARTICLES 100000

int main();

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int modsdouble);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void RenderUI();
glm::vec3 camPos = glm::vec3(10.0f, 10.0f, 10.0f);
glm::vec3 camFront = glm::vec3(-1.0f, -1.0f, -1.0f);
glm::vec3 camUp = glm::vec3(0.0f, 0.0f, 1.0f);
float sensitivity = 5.0f;
bool focused = false;

bool firstMouse = true;
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = 1920.0f / 2.0;
float lastY = 1080.0 / 2.0;
float fov = 45.0f;

float deltaTimeFrame = .0f;
float lastFrame = .0f;
bool timeToSimulate = false;

struct particle_gpu {
    glm::vec3 p;
    glm::vec3 c;
};

struct particle_cpu {
    glm::vec3 v;
    float m;
    float LS;
    float COF;
    float COR;
    float age;
};

struct particleGenerator { //for now this is a directional generator, gonna add polygonal ones in the future put have to refactor etc
    unsigned int vao;
    particle_gpu* pgpus;
    particle_cpu* pcpus;
    glm::vec3 p; //position
    glm::vec3 v; //velocity
    glm::vec3 d; //direction
    float P; // period
    float t; // time
    int pnum = 0;
};

struct collider {
    glm::vec3* v;
};

void generateWireframeCube(float cubeSize, float* vertices) {
    float cubeVertices[] = {
        cubeSize / 2.0f, cubeSize / 2.0f, cubeSize / 2.0f,
        cubeSize / 2.0f, -cubeSize / 2.0f, cubeSize / 2.0f,
        -cubeSize / 2.0f, -cubeSize / 2.0f, cubeSize / 2.0f,
        -cubeSize / 2.0f, cubeSize / 2.0f, cubeSize / 2.0f,
        cubeSize / 2.0f, cubeSize / 2.0f, cubeSize / 2.0f,
        cubeSize / 2.0f, cubeSize / 2.0f, -cubeSize / 2.0f,
        -cubeSize / 2.0f, cubeSize / 2.0f, -cubeSize / 2.0f,
        -cubeSize / 2.0f, cubeSize / 2.0f, cubeSize / 2.0f,
        -cubeSize / 2.0f, cubeSize / 2.0f, -cubeSize / 2.0f,
        -cubeSize / 2.0f, -cubeSize / 2.0f, -cubeSize / 2.0f,
        -cubeSize / 2.0f, -cubeSize / 2.0f, cubeSize / 2.0f,
        -cubeSize / 2.0f, -cubeSize / 2.0f, -cubeSize / 2.0f,
        cubeSize / 2.0f, -cubeSize / 2.0f, -cubeSize / 2.0f,
        cubeSize / 2.0f, -cubeSize / 2.0f, cubeSize / 2.0f,
        cubeSize / 2.0f, -cubeSize / 2.0f, -cubeSize / 2.0f,
        cubeSize / 2.0f, cubeSize / 2.0f, -cubeSize / 2.0f,
    };
    for (int i = 0; i < 48; i++) {
        vertices[i] = cubeVertices[i];
    }
    vertices = cubeVertices;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float camSpeed = static_cast<float>(sensitivity * deltaTimeFrame);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camPos += camSpeed * camFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camPos -= camSpeed * camFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camPos -= camSpeed * glm::normalize(glm::cross(camFront, camUp));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camPos += camSpeed * glm::normalize(glm::cross(camFront, camUp));
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camPos += camSpeed * camUp;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camPos -= camSpeed * camUp;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void particleShaderSetup() {
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); // pos
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // color
}

float hpSign(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3) {
    return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

int main() {

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // opengl version 3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); //opengil version 3.3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //using core profile of opengl
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1920, 1080, "LearnOpenGL", NULL, NULL);
    //glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, 1920, 1080, GLFW_DONT_CARE);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glViewport(0, 0, 1920, 1080);

    //TODO generate particles, at this point these are just state vector
    
    
    float coneH = 2.0f;
    float coneR = 1.0f;
    const int n = 32;
    glm::vec3 coneVertices[96] = {};
    //sides first
    float pi = 3.14159265;
    for (int i = 0; i < n; i++) {
        coneVertices[3 * i] = glm::vec3(0.0, 0.0, coneH);
        coneVertices[3 * i + 1] = glm::vec3(std::cos(i * pi * 2.0f / n)* coneR, std::sin(i * pi * 2.0f / n)* coneR, 0.0);
        coneVertices[3 * i + 2] = glm::vec3(std::cos((i + 1) * pi * 2.0 / n )* coneR, std::sin((i + 1) * pi * 2.0 / n)* coneR, 0.0);
    }
    printf("%d",GL_MAX_ELEMENTS_VERTICES);
    unsigned int coneVao;
    glGenVertexArrays(1, &coneVao);

    glBindVertexArray(coneVao);
    VertexBuffer cvb(coneVertices, sizeof(coneVertices));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


    particleGenerator pgen1;
    glGenVertexArrays(1, &pgen1.vao);
    pgen1.pgpus = new particle_gpu[MAX_PARTICLE_PER_GENERATOR];
    pgen1.pcpus = new particle_cpu[MAX_PARTICLE_PER_GENERATOR];
    pgen1.p = glm::vec3(10.0,10.0,10.0);
    pgen1.v = glm::vec3(0.0, -1.0, 0.0);
    pgen1.d = glm::vec3(1.0, 1.0, 1.0);
    pgen1.P = .2;
    pgen1.t = 0.0f;

    glBindVertexArray(pgen1.vao);
    VertexBuffer pgen1vb(&pgen1.pgpus[0],sizeof(particle_gpu));
    particleShaderSetup();

    particleGenerator pgen2;
    glGenVertexArrays(1, &pgen2.vao);
    pgen2.pgpus = new particle_gpu[MAX_PARTICLE_PER_GENERATOR];
    pgen2.pcpus = new particle_cpu[MAX_PARTICLE_PER_GENERATOR];
    pgen2.p = glm::vec3(-10.0, -10.0, -10.0);
    pgen2.v = glm::vec3(0.0, 1.0, 0.0);
    pgen2.d = glm::vec3(-1.0, -1.0, -1.0);
    pgen2.P = 1.0;
    pgen2.t = 0.0;

    glBindVertexArray(pgen2.vao);
    VertexBuffer pgen2vb(&pgen2.pgpus[0], sizeof(particle_gpu));
    particleShaderSetup();

    // add colliders
    // for not a simple triangle is enough i think :D
    glm::vec3 tri[3] = {glm::vec3(0.0, 0.0, 0.0),
                        glm::vec3(0.0, 10.0, 10.0),
                        glm::vec3(0.0, -10.0, 10.0) };
    glm::vec3 norm = glm::normalize(glm::cross(tri[0] - tri[1], tri[0] - tri[2]));
    unsigned int collVao;
    glGenVertexArrays(1, &collVao);
    glBindVertexArray(collVao);
    VertexBuffer colvbo(tri,sizeof(tri));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


    //trying to render an icosphere


    Shader primitiveShader("../../../HWs/HW#1/Code/shaders/vertex_shader_prim.glsl", "../../../HWs/HW#1/Code/shaders/fragment_shader_prim.glsl");
    Shader sperspective("../../../HWs/HW#1/Code/shaders/vert.glsl", "../../../HWs/HW#1/Code/shaders/frag.glsl");
    Shader fperspective("../../../HWs/HW#1/Code/shaders/vert_flat.glsl", "../../../HWs/HW#1/Code/shaders/frag_flat.glsl");
    Shader particleShader("../../../HWs/HW2/Code/shaders/vertParticle.glsl", "../../../HWs/HW2/Code/shaders/fragParticle.glsl");
    Shader coneShader("../../../HWs/HW2/Code/shaders/vertCone.glsl", "../../../HWs/HW2/Code/shaders/fragCone.glsl");
    Shader collShader("../../../HWs/HW2/Code/shaders/vertColl.glsl", "../../../HWs/HW2/Code/shaders/fragColl.glsl");

    float pointSize = 4.0f;
    float lineSize = 6.0f;

    glPointSize(pointSize);
    glLineWidth(lineSize);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int width, height;
    float h = 0.01f;
    float f;
    float t = 0.0f;
    float t_max = 120.0f;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    float timestep = h;
    bool stepSim = false;
    float timeToDraw = 0.0f;
    
    std::chrono::steady_clock::time_point t_sim = std::chrono::steady_clock::now();

    
    // Lorenz Params
    float sigma = 10.0f;
    float rho = 28.0f;
    float beta = 8.0f / 3.0f;
    float g = 0.0f;
    float lorenzFac = 0.0f;
    float velVariance = 0.5f;
    particleShader.use();

    while (!glfwWindowShouldClose(window))
    {
        // time handling for input, should not interfere with this
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTimeFrame = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //start of imgui init stuff
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        //end of imgui init stuff
        particleShader.use();
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
        particleShader.setMat4("view", view);

        glfwGetWindowSize(window, &width, &height);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.01f, 100000.0f);
        particleShader.setMat4("projection", projection);
        
        glBindVertexArray(pgen1.vao);
        glDrawArrays(GL_POINTS, 0, pgen1.pnum);

        glBindVertexArray(pgen2.vao);
        glDrawArrays(GL_POINTS, 0, pgen2.pnum);

        coneShader.use();
        coneShader.setMat4("view", view);
        coneShader.setMat4("projection", projection);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, pgen1.p);
        model = glm::rotate(model, acos(glm::dot(glm::vec3(0.0, 0.0, -1.0), glm::normalize(pgen1.d))), glm::cross(glm::vec3(0.0, 0.0, -1.0), glm::normalize(pgen1.d)));
        coneShader.setMat4("model", model);
        glBindVertexArray(coneVao);
        glDrawArrays(GL_TRIANGLES, 0, 96);

        model = glm::mat4(1.0f);
        model = glm::translate(model, pgen2.p);
        model = glm::rotate(model, acos(glm::dot(glm::vec3(0.0, 0.0, -1.0), glm::normalize(pgen2.d))), glm::cross(glm::vec3(0.0, 0.0, -1.0), glm::normalize(pgen2.d)));
        coneShader.setMat4("model", model);
        glBindVertexArray(coneVao);
        glDrawArrays(GL_TRIANGLES, 0, 96);
        
        collShader.use();
        collShader.setMat4("view", view);
        collShader.setMat4("projection", projection);
        model = glm::mat4(1.0f);
        collShader.setMat4("model", model);
        glBindVertexArray(collVao);
        glDrawArrays(GL_TRIANGLES, 0 , 3);

        glBindVertexArray(0);
        // all drawings done lets do some imgui stuff

        ImGui::Begin("Particle Generator Settings");
        ImGui::Text("PG1 # particles: %d", pgen1.pnum);
        ImGui::DragFloat("PG1 Period", &pgen1.P, 0.001);
        ImGui::DragFloat3("PG1 Direction", glm::value_ptr(pgen1.d), 0.05);
        ImGui::DragFloat3("PG1 Velocity", glm::value_ptr(pgen1.v), 0.05);
        ImGui::Text("PG2 # particles: %d", pgen2.pnum);
        ImGui::DragFloat("PG2 Period", &pgen2.P, 0.001);
        ImGui::DragFloat3("PG2 Direction", glm::value_ptr(pgen2.d), 0.05);
        ImGui::DragFloat3("PG2 Velocity", glm::value_ptr(pgen2.v), 0.05);
        ImGui::Text("Total particles: %d", pgen1.pnum + pgen2.pnum);
        ImGui::End();

        ImGui::Begin("Particle Settings");
        ImGui::DragFloat("Particle Velocity Variance", &velVariance, 0.1);
        ImGui::End();

        ImGui::Begin("Render Setting");
        bool sliderPS = ImGui::SliderFloat("Point Size", &pointSize, .01f, 10.0f);
        bool sliderLS = ImGui::SliderFloat("Line Size", &lineSize, .01f, 10.0f);
        ImGui::End();

        ImGui::Begin("Simulation Setting");
        if (ImGui::Button("Start/Pause Simulation")) {
            timeToSimulate = !timeToSimulate;
            stepSim = false;
        }
        if (ImGui::Button("Step Simulation")) {
            stepSim = true;
        }
        if (ImGui::Button("Reset")) {
            t = 0;
            t_sim = std::chrono::steady_clock::now();
            delete pgen1.pgpus;
            delete pgen2.pgpus;
            pgen1.pnum = 0;
            pgen2.pnum = 0;
            pgen1.pgpus = new particle_gpu[MAX_PARTICLE_PER_GENERATOR];
            pgen1.pcpus = new particle_cpu[MAX_PARTICLE_PER_GENERATOR];

            pgen2.pgpus = new particle_gpu[MAX_PARTICLE_PER_GENERATOR];
            pgen2.pcpus = new particle_cpu[MAX_PARTICLE_PER_GENERATOR];
        }
        ImGui::Text("Integration");
        ImGui::SliderFloat("Timestep", &h, .005f, 0.5f);
        ImGui::Text("Lorenz Parameters");
        
        // Lorenz GUI
        ImGui::DragFloat("Rho", &rho, 0.005f);
        ImGui::DragFloat("Beta", &beta, 0.005f);
        ImGui::DragFloat("Sigma", &sigma, 0.005f);
        ImGui::DragFloat("Lorenz Factor", &lorenzFac, 0.005f);
        ImGui::Text("Initial Conditions");
        
        ImGui::Text("World Settings");
        ImGui::DragFloat("Gravity", &g, 0.005f);

        ImGui::End();

        //TODO: Simulation Part
        // first determine whether it is time to simulate by checking t and now
        std::chrono::duration<float> secPassed = std::chrono::steady_clock::now() - t_sim;
        //printf("Second Passed from last sim: %f\n ,simTime in sim: %f\n", secPassed, t);

        if ((timeToSimulate || stepSim)) {//secPassed.count() >= h  &&
            pgen1.t += deltaTimeFrame;
            pgen2.t += deltaTimeFrame;
            if (pgen1.t > pgen1.P && pgen1.pnum < MAX_PARTICLE_PER_GENERATOR) {
                pgen1.t = 0.0f;
                glm::vec3 vgen = pgen1.v + 3.0f * pgen1.d;
                glm::vec3 var = glm::vec3(velVariance, velVariance, velVariance);
                glm::vec3 vgenc = glm::gaussRand(vgen, var-glm::dot(var,vgen));
                pgen1.pcpus[pgen1.pnum] = {
                    vgenc,
                    0.1,
                    120.0,
                    0.1,
                    0.1,
                    0.0
                    };
                pgen1.pgpus[pgen1.pnum] = {
                    glm::gaussRand(pgen1.p,glm::vec3(.1,0.1,0.1)),   // position
                    (1-(glm::length(pgen1.pcpus[pgen1.pnum].v) / 100.0f))*glm::vec3(.0,.0,1.0)+
                    (glm::length(pgen1.pcpus[pgen1.pnum].v) * glm::vec3(1.0,.0,.0)/100.0f)};
                pgen1.pnum += 1;
            }

            if (pgen2.t > pgen2.P && pgen2.pnum < MAX_PARTICLE_PER_GENERATOR) {
                pgen2.t = 0.0f;
                glm::vec3 vgen = pgen2.v + 3.0f*pgen2.d;
                glm::vec3 var = glm::vec3(velVariance, velVariance, velVariance);
                glm::vec3 vgenc = glm::gaussRand(vgen, var - glm::dot(var, vgen));
                pgen2.pcpus[pgen2.pnum] = {
                    vgenc,
                    0.1,
                    120.0,
                    0.1,
                    0.1,
                    0.0
                };
                pgen2.pgpus[pgen2.pnum] = {
                    glm::gaussRand(pgen2.p,glm::vec3(.1,0.1,0.1)),   // position
                    (1 - (glm::length(pgen2.pcpus[pgen2.pnum].v) / 100.0f))* glm::vec3(.0,.0,1.0) +
                    (glm::length(pgen2.pcpus[pgen2.pnum].v) * glm::vec3(1.0,.0,.0) / 100.0f) };
                pgen2.pnum += 1;
            }
            //update generator locations
            pgen1.p += pgen1.v * deltaTimeFrame;
            pgen2.p += pgen2.v * deltaTimeFrame;

            //integration
            
            for (int i = 0; i < pgen1.pnum; i++) {
                glm::vec3 velocity;
                velocity.x = sigma * (pgen1.pgpus[i].p.y - pgen1.pgpus[i].p.x);
                velocity.y = pgen1.pgpus[i].p.x * (rho - pgen1.pgpus[i].p.z) - pgen1.pgpus[i].p.y;
                velocity.z = pgen1.pgpus[i].p.x * pgen1.pgpus[i].p.y - beta * pgen1.pgpus[i].p.z;
                glm::vec3 p_prev = pgen1.pgpus[i].p;
                float d = glm::dot(pgen1.pgpus[i].p - tri[0], norm);
                pgen1.pgpus[i].p += pgen1.pcpus[i].v * deltaTimeFrame;
                float dn = glm::dot(pgen1.pgpus[i].p - tri[0], norm);
                pgen1.pcpus[i].v += g * glm::vec3(0.0, 0.0, -1.0) * deltaTimeFrame;
                pgen1.pcpus[i].v = (1 - lorenzFac * 0.01f) * pgen1.pcpus[i].v + lorenzFac * 0.01f * velocity;
                if (signbit(d) != signbit(dn)) {
                    printf("coll level 1\n");
                    //collision may happened need to check projections
                    glm::vec3 collP = p_prev + pgen1.pcpus[i].v * deltaTimeFrame * (abs(d) / (abs(d) + abs(dn)));
                    //for xy
                    float e1 = hpSign(glm::vec2(collP.y, collP.z), glm::vec2(tri[0].y, tri[0].z), glm::vec2(tri[1].y, tri[1].z));
                    float e2 = hpSign(glm::vec2(collP.y, collP.z), glm::vec2(tri[1].y, tri[1].z), glm::vec2(tri[2].y, tri[2].z));
                    float e3 = hpSign(glm::vec2(collP.y, collP.z), glm::vec2(tri[2].y, tri[2].z), glm::vec2(tri[0].y, tri[0].z));

                    if (signbit(e1) == signbit(e2) && signbit(e2) == signbit(e3)) {
                        printf("coll level 2\n");
                        printf("coll happened\n");
                        pgen1.pgpus[i].p -= 2.0f*glm::dot(pgen1.pgpus[i].p,norm)*norm;
                        glm::vec3 vn = glm::dot(pgen1.pcpus[i].v, norm) * norm;

                        glm::vec3 vt = pgen1.pcpus[i].v - vn;
                        pgen1.pcpus[i].v = -vn+vt;
                    }
                }
                
                pgen1.pgpus[i].c =
                    (1 - (glm::length(pgen1.pcpus[i].v) / 100.0f)) * glm::vec3(.0, .0, 1.0) +
                    (glm::length(pgen1.pcpus[i].v) * glm::vec3(1.0, .0, .0) / 100.0f);
            }
            
            for (int i = 0; i < pgen2.pnum; i++) {
                glm::vec3 velocity;
                velocity.x = sigma * (pgen2.pgpus[i].p.y - pgen2.pgpus[i].p.x);
                velocity.y = pgen2.pgpus[i].p.x * (rho - pgen2.pgpus[i].p.z) - pgen2.pgpus[i].p.y;
                velocity.z = pgen2.pgpus[i].p.x * pgen2.pgpus[i].p.y - beta * pgen2.pgpus[i].p.z;
                pgen2.pgpus[i].p += pgen2.pcpus[i].v * deltaTimeFrame;
                pgen2.pcpus[i].v += g * glm::vec3(0.0, 0.0, -1.0) * deltaTimeFrame;
                pgen2.pcpus[i].v = (1 - lorenzFac*0.01f) * pgen2.pcpus[i].v + lorenzFac * 0.01f * velocity;
                pgen2.pgpus[i].c =
                    (1 - (glm::length(pgen2.pcpus[i].v) / 100.0f)) * glm::vec3(.0, .0, 1.0) +
                    (glm::length(pgen2.pcpus[i].v) * glm::vec3(1.0, .0, .0) / 100.0f);
            }

            pgen1vb.UpdateData(&pgen1.pgpus[0], sizeof(particle_gpu) * pgen1.pnum);
            pgen2vb.UpdateData(&pgen2.pgpus[0], sizeof(particle_gpu) * pgen2.pnum);

            stepSim = false;
            t += deltaTimeFrame;
            t_sim = std::chrono::steady_clock::now();
        }

        if (sliderPS) {
            glPointSize(pointSize);
        }
        if (sliderLS) {
            glLineWidth(lineSize);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    delete pgen1.pgpus;
    delete pgen1.pcpus;
    delete pgen2.pgpus;
    delete pgen2.pcpus;

    glfwTerminate();

    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    if (focused) {

        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = ypos - lastY; // reversed since y-coordinates go from bottom to top
        lastX = xpos;
        lastY = ypos;

        float mouseSens = 0.2f;
        xoffset *= mouseSens;
        yoffset *= mouseSens;

        yaw -= xoffset;
        pitch -= yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.z = sin(glm::radians(pitch));
        camFront = glm::normalize(front);

    }
    else {
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);
        lastX = xpos;
        lastY = ypos;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int modsdouble)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {

    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        if (focused)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        else
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        focused = !focused;
    }
}


void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{

    sensitivity += 0.2f * static_cast<float>(yoffset);
    if (sensitivity < 0) {
        sensitivity = 0.01f;
    }

    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;
}

void RenderUI() {

    ImGui::Begin("Variables");
    ImGui::Text("Hello There!");


    ImGui::End();
}
