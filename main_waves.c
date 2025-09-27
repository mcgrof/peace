#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

float zoomLevel = 1.0f;

const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "out vec2 FragCoord;\n"
    "void main() {\n"
    "   gl_Position = vec4(aPos, 1.0);\n"
    "   FragCoord = aPos.xy;\n"
    "}\n";

const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 FragCoord;\n"
    "uniform float time;\n"
    "uniform float zoom;\n"
    "uniform vec2 resolution;\n"
    "\n"
    "void main() {\n"
    "   vec2 uv = FragCoord * zoom;\n"
    "   \n"
    "   // Create multiple wave layers\n"
    "   float wave = 0.0;\n"
    "   wave += sin(uv.x * 6.0 + time * 2.0) * 0.1;\n"
    "   wave += sin(uv.x * 4.0 - time * 1.5) * 0.15;\n"
    "   wave += sin(uv.x * 8.0 + uv.y * 3.0 + time) * 0.05;\n"
    "   wave += cos(uv.x * 2.0 + time * 0.8) * 0.2;\n"
    "   \n"
    "   // Apply wave to y coordinate\n"
    "   float distorted_y = uv.y - wave * 0.5;\n"
    "   \n"
    "   // Create color gradient based on wave height\n"
    "   float gradient = (distorted_y + 1.0) * 0.5;\n"
    "   \n"
    "   // Ocean colors\n"
    "   vec3 deepWater = vec3(0.0, 0.1, 0.3);\n"
    "   vec3 midWater = vec3(0.0, 0.3, 0.5);\n"
    "   vec3 shallowWater = vec3(0.2, 0.5, 0.7);\n"
    "   vec3 foam = vec3(0.9, 0.95, 1.0);\n"
    "   \n"
    "   vec3 color;\n"
    "   if (gradient < 0.25) {\n"
    "       color = mix(deepWater, midWater, gradient * 4.0);\n"
    "   } else if (gradient < 0.5) {\n"
    "       color = mix(midWater, shallowWater, (gradient - 0.25) * 4.0);\n"
    "   } else if (gradient < 0.75) {\n"
    "       color = mix(shallowWater, foam, (gradient - 0.5) * 4.0);\n"
    "   } else {\n"
    "       color = foam;\n"
    "   }\n"
    "   \n"
    "   // Add shimmer effect\n"
    "   float shimmer = sin(uv.x * 20.0 + time * 5.0) * sin(uv.y * 20.0 - time * 3.0);\n"
    "   color += shimmer * 0.05;\n"
    "   \n"
    "   FragColor = vec4(color, 1.0);\n"
    "}\n";

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    zoomLevel -= (float)yoffset * 0.1f;
    if (zoomLevel < 0.3f) zoomLevel = 0.3f;
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

    // Full screen quad
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f
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
        glClearColor(0.0f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        float timeValue = glfwGetTime();
        int timeLoc = glGetUniformLocation(shaderProgram, "time");
        glUniform1f(timeLoc, timeValue);

        int zoomLoc = glGetUniformLocation(shaderProgram, "zoom");
        glUniform1f(zoomLoc, zoomLevel);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        int resLoc = glGetUniformLocation(shaderProgram, "resolution");
        glUniform2f(resLoc, (float)width, (float)height);

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