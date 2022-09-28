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

int main();

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int modsdouble);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void RenderUI();
glm::vec3 camPos = glm::vec3(5.0f, 5.0f, 0.0f);
glm::vec3 camFront = glm::vec3(0.0f, 0.0f, -1.0f);
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
    std::vector<particle_gpu> pgpus;
    std::vector<particle_cpu> pcpus;
    glm::vec3 p; //position
    glm::vec3 v; //velocity
    glm::vec3 d; //direction
    float P; // period
    float t; // time
};

struct state {
    float m;
    glm::vec3 position;
    glm::vec3 velocity;
    float windFactor;
    glm::vec3 wind;
    float airResistanceFactor;
    glm::vec3 gravity;
};

void setInitConditions(state& cur, state& init) {
    cur = init;
}

void setAcceleration(state& curState, glm::vec3& acc) {
    glm::vec3 gravity = curState.m * curState.gravity;
    glm::vec3 airResistance = -curState.airResistanceFactor * curState.velocity;
    glm::vec3 totalForce = gravity + curState.windFactor * curState.wind + airResistance;
    acc = totalForce / curState.m;
}

void integrate(state& curState, state& nextState, glm::vec3& acc, float& h) {
    nextState.m = curState.m;
    nextState.airResistanceFactor = curState.airResistanceFactor;
    nextState.gravity = curState.gravity;
    nextState.wind = curState.wind;
    nextState.windFactor = curState.windFactor;
    nextState.velocity = curState.velocity + acc * h;
    nextState.position = curState.position + curState.velocity * h;
}

bool checkCollision(state& curState, state& nextState, const float radius, const float cubeSize, glm::vec3& hitNormal) {
    float hitPoint = (cubeSize / 2) - radius;
    if (nextState.position.x > hitPoint) {
        hitNormal = glm::vec3(-1.0, 0.0, 0.0);
    }
    else if (nextState.position.x < -hitPoint) {
        hitNormal = glm::vec3(1.0, 0.0, 0.0);
    }
    else if (nextState.position.y > hitPoint) {
        hitNormal = glm::vec3(0.0, -1.0, 0.0);
    }
    else if (nextState.position.y < -hitPoint) {
        hitNormal = glm::vec3(0.0, 1.0, 0.0);
    }
    else if (nextState.position.z > hitPoint) {
        hitNormal = glm::vec3(0.0, 0.0, -1.0);
    }
    else if (nextState.position.z < -hitPoint) {
        hitNormal = glm::vec3(0.0, 0.0, 1.0);
    }
    else { return false; }
    return true;
}

void findFraction(state& curState, state& nextState, const float radius, const float cubeSize, float& fraction) {
    float hitPoint = (cubeSize / 2) - radius;
    float curHeight;
    if (abs(nextState.position.x) > hitPoint) {
        curHeight = hitPoint - abs(curState.position.x);
        fraction = curHeight / (nextState.position.x - curState.position.x);
    }
    else if (abs(nextState.position.y) > hitPoint) {
        curHeight = hitPoint - abs(curState.position.y);
        fraction = curHeight / (nextState.position.y - curState.position.y);
    }
    else if (abs(nextState.position.z) > hitPoint) {
        curHeight = hitPoint - abs(curState.position.z);
        fraction = curHeight / (nextState.position.z - curState.position.z);
    }
#ifdef _DEBUG
    printf("Current Distance from Surface: %f, fraction time: %f\n", curHeight, fraction);
#endif // DEBUG


}

void collResponse(state& collState, state& nextState, glm::vec3 hitNormal, float& elas, float& mu) {
    nextState.position = collState.position;
    glm::vec3 VN = hitNormal * glm::dot(collState.velocity, hitNormal);
    glm::vec3 VT = collState.velocity - VN;
    glm::vec3 nextVT = VT;
    if (glm::length(VT) > 0.01)
        glm::vec3 nextVT = VT - glm::normalize(VT) * fmin(mu * glm::length(VN), glm::length(VT));
    glm::vec3 nextVN = -elas * VN;

#ifdef _DEBUG
    printf("    VT: %f,%f,%f\n", VT.x, VT.y, VT.z);
    printf("    VN: %f,%f,%f\n", VN.x, VN.y, VN.z);
    printf("    Next VT: %f,%f,%f\n", nextVT.x, nextVT.y, nextVT.z);
    printf("    Next VN: %f,%f,%f\n", nextVN.x, nextVN.y, nextVN.z);
#endif // _DEBUG

    nextState.velocity = nextVT + nextVN;
}

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

struct collider {
    glm::vec3 v[3];
};

void particleShaderSetup() {
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); // pos
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // color
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
    std::vector<particle_gpu> particleGPU;
    std::vector<particle_cpu> particleCPU;

    const int particle_num = 1000;
    for (int i = 0; i < particle_num; i++) {
        particle_gpu pGPU = {
            glm::gaussRand(glm::vec3(0.0,0.0,0.0),glm::vec3(2.0,2.0,2.0)),   // position
            glm::linearRand(glm::vec3(0.0,0.0,0.0),glm::vec3(1.0,1.0,1.0))  // color
        };
        particle_cpu pCPU = {
            glm::gaussRand(glm::vec3(0.0,0.0,0.0),glm::vec3(10.0,10.0,10.0)),   // velocity
            glm::gaussRand(1.0,15.0),                                           // mass
            glm::gaussRand(120.0,10.0),                                         // lifespan
            glm::gaussRand(0.1,0.9),                                            //  COR
            glm::gaussRand(0.1,0.9),                                            // COF
            0.0f                                                                // age
        };
        particleGPU.push_back(pGPU);
        particleCPU.push_back(pCPU);
    }
        unsigned int vaoPar;
    glGenVertexArrays(1, &vaoPar);
    glBindVertexArray(vaoPar);

    VertexBuffer vbParticles(&particleGPU[0], sizeof(particle_gpu) * particleGPU.size());
    particleShaderSetup();

    particleGenerator pgen1;
    glGenVertexArrays(1, &pgen1.vao);
    pgen1.p = glm::vec3(10.0,10.0,10.0);
    pgen1.v = glm::vec3(0.0, -1.0, 0.0);
    pgen1.d = glm::vec3(1.0, 1.0, 1.0);
    pgen1.P = .2;
    pgen1.t = 0.0f;
    pgen1.pgpus.push_back({ 
        glm::gaussRand(pgen1.p,glm::vec3(.1,0.1,0.1)),   // position
        glm::linearRand(glm::vec3(0.0,0.0,0.0),glm::vec3(1.0,1.0,1.0)) }
    ); //dummy first particle
    glBindVertexArray(pgen1.vao);
    VertexBuffer pgen1vb(&pgen1.pgpus[0],sizeof(particle_gpu));
    particleShaderSetup();


    particleGenerator pgen2;
    glGenVertexArrays(1, &pgen2.vao);
    pgen2.p = glm::vec3(-10.0, -10.0, -10.0);
    pgen2.v = glm::vec3(0.0, 1.0, 0.0);
    pgen2.d = glm::vec3(-1.0, -1.0, -1.0);
    pgen2.P = 1.0;
    pgen2.t = 0.0;
    pgen2.pgpus.push_back({
        glm::gaussRand(pgen2.p,glm::vec3(.1,0.1,0.1)),   // position
        glm::linearRand(glm::vec3(0.0,0.0,0.0),glm::vec3(1.0,1.0,1.0)) }
    ); //dummy first particle
    glBindVertexArray(pgen2.vao);
    VertexBuffer pgen2vb(&pgen2.pgpus[0], sizeof(particle_gpu));
    particleShaderSetup();


    Shader primitiveShader("../../../HWs/HW#1/Code/shaders/vertex_shader_prim.glsl", "../../../HWs/HW#1/Code/shaders/fragment_shader_prim.glsl");
    Shader sperspective("../../../HWs/HW#1/Code/shaders/vert.glsl", "../../../HWs/HW#1/Code/shaders/frag.glsl");
    Shader fperspective("../../../HWs/HW#1/Code/shaders/vert_flat.glsl", "../../../HWs/HW#1/Code/shaders/frag_flat.glsl");
    Shader particleShader("../../../HWs/HW2/Code/shaders/vertParticle.glsl", "../../../HWs/HW2/Code/shaders/fragParticle.glsl");

    sperspective.use();

    float pointSize = 10.f;
    float lineSize = 6.0f;

    glPointSize(pointSize);
    glLineWidth(lineSize);

    glEnable(GL_DEPTH_TEST);

    int width, height;
    float h = 0.01f;
    float f;
    float t = 0.0f;
    float t_max = 120.0f;
    state curState;
    state nextState;
    state collState;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    state init;
    init.m = 1.0f;
    init.airResistanceFactor = 0;
    init.gravity = glm::vec3(0.0, 0.0, -10.0);
    init.position = glm::vec3(0.0, 0.0, 3.0);
    init.velocity = glm::vec3(0.0, 0.0, 0.0);
    init.wind = glm::vec3(0.0, 0.0, 0.0);
    init.windFactor = 0;

    float timestep = h;
    setInitConditions(curState, init);
    bool stepSim = false;
    float timeToDraw = 0.0f;
    glm::vec3 posBuf;

    std::chrono::steady_clock::time_point t_sim = std::chrono::steady_clock::now();

    /*
    // Lorenz Params
    float sigma = 10.0f;
    float rho = 28.0f;
    float beta = 8.0f / 3.0f;
    */
    particleShader.use();

    while (!glfwWindowShouldClose(window))
    {
        // time handling for input, should not interfere with this
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTimeFrame = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window);

        glClearColor(0.6f, 1.0f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //start of imgui init stuff
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        //end of imgui init stuff

        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
        particleShader.setMat4("view", view);

        glfwGetWindowSize(window, &width, &height);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.01f, 1000.0f);
        particleShader.setMat4("projection", projection);
        
        glBindVertexArray(pgen1.vao);
        glDrawArrays(GL_POINTS, 0, pgen1.pgpus.size());

        glBindVertexArray(pgen2.vao);
        glDrawArrays(GL_POINTS, 0, pgen2.pgpus.size());
        /*
        // lorenz drawing 
        glBindVertexArray(VAO_particles);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_par_pos);
        glBufferData(GL_ARRAY_BUFFER, sizeof(particles), &particles[0], GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_POINTS, 0, 3* particle_num);

        // lorenz path
        glBindVertexArray(VAO_path);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_path_pos);
        glBufferData(GL_ARRAY_BUFFER, path_pos.size() * 3 * sizeof(float), glm::value_ptr<float>(path_pos[0]), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_path_norm);
        glBufferData(GL_ARRAY_BUFFER, path_norm.size() * 3 * sizeof(float), glm::value_ptr<float>(path_norm[0]), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_LINE_STRIP, 0, path_pos.size());

        */


        // all drawings done lets do some imgui stuff

        ImGui::Begin("Object Settings");
        ImGui::End();

        ImGui::Begin("Render Setting");
        bool sliderPS = ImGui::SliderFloat("Point Size", &pointSize, .01f, 10.0f);
        bool sliderLS = ImGui::SliderFloat("Line Size", &lineSize, .01f, 10.0f);
        if (ImGui::Button("Flat Shading")) {
            fperspective.use();
        }
        if (ImGui::Button("Smooth Shading")) {
            sperspective.use();
        }
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
            setInitConditions(curState, init);
            t = 0;
            t_sim = std::chrono::steady_clock::now();
            /*
            // Path Clearing and particle generation for lorenz
            path_pos.clear();
            path_norm.clear();
            
            for (int i = 0; i < particle_num; i++) {
                particles[i] = glm::vec3(0.0f, 0.0f, 20.0f) + glm::ballRand<float>(20.0f);
            }
            path_pos.push_back(particles[0]);
            path_norm.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
            */
        }
        ImGui::Text("Integration");
        ImGui::SliderFloat("Timestep", &h, .005f, 0.5f);
        ImGui::Text("Lorenz Parameters");
        /*
        // Lorenz GUI
        ImGui::InputFloat("Rho", &rho);
        ImGui::InputFloat("Beta", &beta);
        ImGui::InputFloat("Sigma", &sigma);
        */
        ImGui::Text("Initial Conditions");
        ImGui::InputFloat("Mass", &init.m);
        ImGui::InputFloat3("Gravity", glm::value_ptr(init.gravity));
        ImGui::InputFloat3("Position", glm::value_ptr(init.position));
        ImGui::InputFloat3("Velocity", glm::value_ptr(init.velocity));
        ImGui::InputFloat3("Wind", glm::value_ptr(init.wind));
        ImGui::InputFloat("Wind Factor", &init.windFactor);
        ImGui::InputFloat("Air Resistance Factor", &init.airResistanceFactor);
        if (ImGui::Button("Randomize")) {
        }

        ImGui::End();

        //TODO: Simulation Part
        // first determine whether it is time to simulate by checking t and now
        std::chrono::duration<float> secPassed = std::chrono::steady_clock::now() - t_sim;
        //printf("Second Passed from last sim: %f\n ,simTime in sim: %f\n", secPassed, t);

        if ((timeToSimulate || stepSim)) {//secPassed.count() >= h  &&
            pgen1.t += deltaTimeFrame;
            pgen2.t += deltaTimeFrame;
            if (pgen1.t > pgen1.P) {
                pgen1.t = 0.0f;
                pgen1.pgpus.push_back({
                    glm::gaussRand(pgen1.p,glm::vec3(.01,0.1,0.1)),   // position
                    glm::linearRand(glm::vec3(0.0,0.0,0.0),glm::vec3(1.0,1.0,1.0)) }
                ); 
                pgen1vb.UpdateData(&pgen1.pgpus[0], sizeof(particle_gpu) * pgen1.pgpus.size());
            }
            if (pgen2.t > pgen2.P) {
                pgen2.t = 0.0f;
                pgen2.pgpus.push_back({
                    glm::gaussRand(pgen2.p,glm::vec3(.01,0.1,0.1)),   // position
                    glm::linearRand(glm::vec3(0.0,0.0,0.0),glm::vec3(1.0,1.0,1.0)) }
                ); 
                pgen2vb.UpdateData(&pgen2.pgpus[0], sizeof(particle_gpu) * pgen2.pgpus.size());
            }
            //update generator locations
            pgen1.p += pgen1.v * deltaTimeFrame;
            pgen2.p += pgen2.v * deltaTimeFrame;
            /*
            // Lorenz integration, position based dynamics :D
            //lets do some lorenz stuff first
            path_pos.push_back(particles[0]);
            path_norm.push_back(glm::vec3(0.0f,0.0f,1.0f));
            
            for (int i = 0; i < particle_num; i++) {
                glm::vec3 velocity;
                velocity.x = sigma * (particles[i].y - particles[i].x);
                velocity.y = particles[i].x * (rho - particles[i].z) - particles[i].y;
                velocity.z = particles[i].x * particles[i].y - beta * particles[i].z;
                particles[i] = particles[i] + 0.1f * deltaTimeFrame * velocity;
            }
            */

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
