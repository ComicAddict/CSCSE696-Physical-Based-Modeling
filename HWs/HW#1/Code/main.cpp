#include <iostream>
#include <vector>
#include <fstream>
#include <string>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include "shader.h"
#include "Sphere.h"

int main();

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int modsdouble);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void RenderUI();
glm::vec3 camPos = glm::vec3(5.0f, 5.0f, 0.0f);
glm::vec3 camFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camUp = glm::vec3(0.0f, 0.0f, 1.0f);
float sensitivity = 0.1f;
bool focused = false;

bool firstMouse = true;
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float fov = 45.0f;

float deltaTimeFrame = .0f;
float lastFrame = .0f;
bool timeToSimulate = false;

struct state{
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
    glm::vec3 airResistance = -curState.airResistanceFactor * curState.velocity * curState.velocity;
    glm::vec3 totalForce = gravity + curState.windFactor * curState.wind * curState.wind + airResistance;
    acc = totalForce / curState.m;
}

void integrate(state& curState, state& nextState, glm::vec3& acc, float &h) {
    nextState.m = curState.m;
    nextState.airResistanceFactor = curState.airResistanceFactor;
    nextState.gravity = curState.gravity;
    nextState.wind = curState.wind;
    nextState.windFactor = curState.windFactor;
    nextState.velocity = curState.velocity + acc*h;
    nextState.position = curState.position + curState.velocity*h;
}

bool checkCollision(state & curState, state & nextState, const float radius, const float cubeSize, glm::vec3 &hitNormal) {
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

void collResponse(state& collState, state& nextState, glm::vec3 hitNormal) {
    float elas = 0.5f;
    float mu = 0.1f;
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

void generateWireframeCube(float cubeSize, float *vertices) {
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
        camPos -= camSpeed * glm::normalize(glm::cross(camFront,camUp));
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
    int dims[2] = { 32, 16 };
    float radius = .5f;
    Sphere ball = Sphere(dims[0], dims[1], true, radius);

    unsigned int VAO_sphere;
    glGenVertexArrays(1, &VAO_sphere);

    unsigned int VBO_pos;
    glGenBuffers(1, &VBO_pos);

    unsigned int VBO_normal;
    glGenBuffers(1, &VBO_normal);

    unsigned int EBO;
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO_sphere);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO_pos);
    glBufferData(GL_ARRAY_BUFFER, ball.vertices.size() * sizeof(float), &ball.vertices[0], GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_normal);
    glBufferData(GL_ARRAY_BUFFER, ball.normals.size() * sizeof(float), &ball.normals[0], GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ball.indices.size()*sizeof(float), &ball.indices[0], GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    unsigned int VAO_cube;
    glGenVertexArrays(1, &VAO_cube);

    unsigned int VBO_cube;
    glGenBuffers(1, &VBO_cube);
    
    glBindVertexArray(VAO_cube);

    
    float cubeSize = 10.0f;
    float  cubevertices[48];
    generateWireframeCube(cubeSize, cubevertices);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_cube);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubevertices), cubevertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    Shader primitiveShader("../../../HWs/HW#1/Code/shaders/vertex_shader_prim.glsl","../../../HWs/HW#1/Code/shaders/fragment_shader_prim.glsl");
    Shader sperspective("../../../HWs/HW#1/Code/shaders/vert.glsl", "../../../HWs/HW#1/Code/shaders/frag.glsl");

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
    init.gravity = glm::vec3(0.0, 0.0, -1.0);
    init.position = glm::vec3(0.0, 0.0, 3.0);
    init.velocity = glm::vec3(0.0, 0.0, 0.0);
    init.wind = glm::vec3(0.0, 0.0, 0.0);
    init.windFactor = 0;

    float timestep = h;
    setInitConditions(curState, init);
    bool stepSim = false;
    float timeToDraw = 0.0f;
    glm::vec3 posBuf;
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTimeFrame = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window);

        glClearColor(.2f, .3f, .3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
        

        //start of imgui init stuff
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //end of imgui init stuff

        sperspective.use();

        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
        sperspective.setMat4("view", view);

        glfwGetWindowSize(window, &width, &height);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
        sperspective.setMat4("projection", projection);

        glBindVertexArray(VAO_sphere);
        glm::mat4 model = glm::mat4(1.0f);
        
        if (glfwGetTime() > t) {
            posBuf = curState.position;
        }
        model = glm::translate(model, posBuf);
        sperspective.setMat4("model", model);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(ball.indices.size()), GL_UNSIGNED_INT, 0);

        model = glm::mat4(1.0f);
        sperspective.setMat4("model", model);

        glBindVertexArray(VAO_cube);
        glDrawArrays(GL_LINE_STRIP, 0, 16);

        //glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
        // all drawings done lets do some imgui stuff

        //RenderUI();
        ImGui::Begin("Object Settings");
        bool sliderDim = ImGui::SliderInt2("Stacks and Sectors", dims, 3, 128);
        bool sliderRad = ImGui::SliderFloat("Radius", &radius, .01f, 20.f);
        bool sliderCube = ImGui::SliderFloat("Cube Size", &cubeSize, .01f, 20.f);
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
            setInitConditions(curState, init);
        }
        ImGui::Text("Integration");
        ImGui::SliderFloat("Timestep", &h, .01f, 1.0f);
        ImGui::Text("Initial Conditions");
        ImGui::InputFloat3("Gravity", glm::value_ptr(init.gravity));
        ImGui::InputFloat3("Position", glm::value_ptr(init.position));
        ImGui::InputFloat3("Velocity", glm::value_ptr(init.velocity));
        ImGui::InputFloat3("Wind", glm::value_ptr(init.wind));
        ImGui::InputFloat("Wind Factor", &init.windFactor);
        ImGui::InputFloat("Air Resistance Factor", &init.airResistanceFactor);

        ImGui::End();

        //TODO: Simulation Part
        if (timeToSimulate || stepSim) {
            #ifdef _DEBUG
            printf("simulating, Time: %f, GLFWTime: %f, deltaFrameTime: %f\n", t, static_cast<float>(glfwGetTime()), deltaTimeFrame);
            printf("    Current Pos: %f, %f, %f     Current Velocity: %f, %f, %f",curState.position.x, curState.position.y, curState.position.z, curState.velocity.x, curState.velocity.y, curState.velocity.z);
            #endif // _DEBUG

            glm::vec3 acc_ball = glm::vec3(0.0, 0.0, 0.0);
            setAcceleration(curState, acc_ball);

            timestep = h;
            integrate(curState, nextState, acc_ball, timestep);

            glm::vec3 hitNormal = glm::vec3(1.0, 0.0, 0.0);

            if (checkCollision(curState, nextState, radius, cubeSize, hitNormal)) { //for checking collision we can create a collider class with taking vertices of the shape
                findFraction(curState, nextState, radius, cubeSize, f);
                timestep = f * h;
                integrate(curState, collState, acc_ball, timestep);

                #ifdef _DEBUG
                printf("There is a collision at: %f,%f,%f, fraction timestep: %f\n", collState.position.x, collState.position.y, collState.position.z, f);
                #endif // _DEBUG

                collResponse(collState, nextState, hitNormal);
                t += timestep;
            }
            else {
                t += timestep;
            }
            curState = nextState;
            stepSim = false;
        }

        if (sliderDim || sliderRad) {
            ball.setDims(dims[0], dims[1], radius);
            glGenBuffers(1, &VBO_pos);

            glGenBuffers(1, &VBO_normal);

            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO_sphere);

            glBindBuffer(GL_ARRAY_BUFFER, VBO_pos);
            glBufferData(GL_ARRAY_BUFFER, ball.vertices.size() * sizeof(float), &ball.vertices[0], GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glBindBuffer(GL_ARRAY_BUFFER, VBO_normal);
            glBufferData(GL_ARRAY_BUFFER, ball.normals.size() * sizeof(float), &ball.normals[0], GL_DYNAMIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, ball.indices.size() * sizeof(float), &ball.indices[0], GL_DYNAMIC_DRAW);
        }

        if (sliderCube) {
            generateWireframeCube(cubeSize, cubevertices);
            glBindBuffer(GL_ARRAY_BUFFER, VBO_cube);
            glBufferData(GL_ARRAY_BUFFER, sizeof(cubevertices), cubevertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
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

    glDeleteVertexArrays(1, &VAO_sphere);
    glDeleteBuffers(1, &VBO_pos);
    glDeleteBuffers(1, &VBO_normal);

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
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS){
        
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        if(focused)
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
