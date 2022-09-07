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
glm::vec3 camPos = glm::vec3(3.0f, 3.0f, 0.0f);
glm::vec3 camFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camUp = glm::vec3(0.0f, 0.0f, 1.0f);

bool focused = false;

bool firstMouse = true;
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float fov = 45.0f;

float deltaTimeFrame = .0f;
float lastFrame = .0f;

struct state{
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 wind;
};

void setAcceleration(state& const curState, glm::vec3& const acc) {

}

void integrate(state& const curState, state& const nextState, glm::vec3& const acc, const float &timestep) {

}

bool checkCollision(state & const curState) {
    return false;
}

void findFraction(state & const curState, state & const nextState, float & fraction) {

}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float camSpeed = static_cast<float>(2.5 * deltaTimeFrame);
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
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

int main() {
    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // opengl version 3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); //opengil version 3.3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //using core profile of opengl
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
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

    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    float vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    glm::vec3 cubePositions[] = {
        glm::vec3(0.0f,  0.0f,  0.0f),
        glm::vec3(2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3(2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3(1.3f, -2.0f, -2.5f),
        glm::vec3(1.5f,  2.0f, -2.5f),
        glm::vec3(1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };

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

    
    const float cubeSize = 5.0f;
    float  cubevertices[] = {
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
    printf("%d", sizeof(cubevertices));

    glBindBuffer(GL_ARRAY_BUFFER, VBO_cube);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubevertices), cubevertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    Shader primitiveShader("../../../CSCSE696-Physical-Based-Modeling/HWs/HW#1/Code/shaders/vertex_shader_prim.glsl","../../../CSCSE696-Physical-Based-Modeling/HWs/HW#1/Code/shaders/fragment_shader_prim.glsl");
    Shader sperspective("../../../CSCSE696-Physical-Based-Modeling/HWs/HW#1/Code/shaders/vert.glsl", "../../../CSCSE696-Physical-Based-Modeling/HWs/HW#1/Code/shaders/frag.glsl");

    sperspective.use();

    float pointSize = 10.f;
    float lineSize = 6.0f;

    glPointSize(pointSize);
    glLineWidth(lineSize);

    glEnable(GL_DEPTH_TEST);
    
    
    int width, height;
    float h = 0.02;
    float f;
    state curState;
    state nextState;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    while (!glfwWindowShouldClose(window))
    {   
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTimeFrame = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window);

        glClearColor(.2f, .3f, .3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glPointSize(pointSize);
        glLineWidth(lineSize);

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
        /*
        for (unsigned int i = 0; i < 10; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            sperspective.setMat4("model", model);

            //glDrawArrays(GL_TRIANGLES, 0, 36);
            glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
        }*/
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(sin(glfwGetTime())));
        sperspective.setMat4("model", model);
        glDrawElements(GL_TRIANGLES, ball.indices.size(), GL_UNSIGNED_INT, 0);

        model = glm::mat4(1.0f);
        sperspective.setMat4("model", model);
        glBindVertexArray(VAO_cube);
        glDrawArrays(GL_LINE_STRIP, 0, 16);

        //glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
        // all drawings done lets do some imgui stuff
        
        RenderUI();
        ImGui::Begin("Sphere Settings");
        bool sliderDim = ImGui::SliderInt2("Stacks and Sectors", dims, 3, 128);
        bool sliderRad = ImGui::SliderFloat("Radius", &radius, .01f, 20.f);

        ImGui::End();

        ImGui::Begin("Render Setting");
        ImGui::SliderFloat("Point Size", &pointSize, .01f, 10.0f);
        ImGui::SliderFloat("Line Size", &lineSize, .01f, 10.0f);
        ImGui::End();

        ImGui::Begin("Simulation Setting");
        ImGui::SliderFloat("Timestep", &h, .01f, 1.0f);
        ImGui::End();

        //TODO: Simulation Part
        float timestep = h;
        glm::vec3 acc_ball;
        setAcceleration(curState, acc_ball);
        integrate(curState, nextState, acc_ball, timestep);
        if (checkCollision(curState)) {
            findFraction(curState, nextState, f);
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

        float sensitivity = 0.1f; // change this value to your liking
        xoffset *= sensitivity;
        yoffset *= sensitivity;

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