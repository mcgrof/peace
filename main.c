#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

float zoomLevel = 1.0f;

const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "out vec2 TexCoord;\n"
    "uniform float zoom;\n"
    "void main() {\n"
    "   gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
    "   TexCoord = aPos.xy / zoom;\n"
    "}\n";

const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoord;\n"
    "uniform float time;\n"
    "uniform float zoom;\n"
    "void main() {\n"
    "   vec2 pos = TexCoord;\n"
    "   float wave = sin(pos.x * 3.0 + time) * 0.15;\n"
    "   wave += sin(pos.x * 5.0 - time * 0.8) * 0.08;\n"
    "   wave += sin(pos.x * 7.0 + time * 1.2) * 0.04;\n"
    "   wave += sin(pos.x * 2.0 + pos.y * 4.0 + time * 0.5) * 0.05;\n"
    "   float waveY = pos.y + wave;\n"
    "   \n"
    "   float gradient = (waveY + 1.0) * 0.5;\n"
    "   vec3 deepBlue = vec3(0.1, 0.3, 0.6);\n"
    "   vec3 skyBlue = vec3(0.53, 0.81, 0.92);\n"
    "   vec3 lavender = vec3(0.9, 0.8, 1.0);\n"
    "   vec3 peach = vec3(1.0, 0.85, 0.7);\n"
    "   \n"
    "   vec3 color = mix(deepBlue, skyBlue, gradient);\n"
    "   color = mix(color, lavender, gradient * gradient);\n"
    "   color = mix(color, peach, sin(time * 0.3 + pos.x * 0.5) * 0.15 + 0.15);\n"
    "   \n"
    "   // Add wave distortion to create flowing effect\n"
    "   float intensity = 1.0 - smoothstep(-0.5, 0.5, abs(waveY));\n"
    "   color = mix(color * 0.8, color * 1.2, intensity);\n"
    "   \n"
    "   FragColor = vec4(color, 1.0);\n"
    "}\n";

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    zoomLevel += (float)yoffset * 0.1f;
    if (zoomLevel < 0.5f) zoomLevel = 0.5f;
    if (zoomLevel > 3.0f) zoomLevel = 3.0f;
}

unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("Shader compilation failed: %s\n", infoLog);
    }

    return shader;
}

int main() {
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Peaceful Waves", NULL, NULL);
    if (!window) {
        printf("Failed to create window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);

    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Create a simple full-screen quad
    float vertices[] = {
        // Full screen quad
        -3.0f, -3.0f, 0.0f,
         3.0f, -3.0f, 0.0f,
         3.0f,  3.0f, 0.0f,
        -3.0f,  3.0f, 0.0f
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
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

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        float timeValue = glfwGetTime();
        int timeLoc = glGetUniformLocation(shaderProgram, "time");
        glUniform1f(timeLoc, timeValue);

        int zoomLoc = glGetUniformLocation(shaderProgram, "zoom");
        glUniform1f(zoomLoc, zoomLevel);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}