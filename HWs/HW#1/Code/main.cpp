#include <iostream>
#include <fstream>
#include <string>
#include <glad/glad.h>
#include <glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
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

    /* Triangle vertices
    float vertices[] = {
        -.5f, -.5f, .0f,
         .5f, -.5f, .0f,
         .0f,  .5f, .0f
    };*/


    float vertices[] = {
        .5f, .5f, .0f,
        .5f, -.5f, .0f,
        -.5f, -.5f, .0f,
        -.5f, .5f, .0f
    };

    unsigned int indices[] = {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    unsigned int EBO;
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    Shader primitiveShader("../../../HWs/HW#1/Code/shaders/vertex_shader_prim.glsl","../../../HWs/HW#1/Code/shaders/fragment_shader_prim.glsl");
    Shader perspective("../../../HWs/HW#1/Code/shaders/vert.glsl", "../../../HWs/HW#1/Code/shaders/frag.glsl");

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -13.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f/600.0f, .1f, 100.0f);

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        glClearColor(.2f, .3f, .3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        int viewLoc = glGetUniformLocation(perspective.ID, "view");
        int projLoc = glGetUniformLocation(perspective.ID, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        perspective.use();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
