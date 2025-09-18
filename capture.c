#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "out vec3 FragPos;\n"
    "uniform float time;\n"
    "void main() {\n"
    "   float wave = sin(aPos.x * 3.0 + time) * 0.1;\n"
    "   wave += sin(aPos.x * 5.0 - time * 0.8) * 0.05;\n"
    "   vec3 pos = aPos;\n"
    "   pos.y += wave;\n"
    "   gl_Position = vec4(pos, 1.0);\n"
    "   FragPos = pos;\n"
    "}\n";

const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 FragPos;\n"
    "uniform float time;\n"
    "void main() {\n"
    "   float gradient = (FragPos.y + 1.0) * 0.5;\n"
    "   vec3 skyBlue = vec3(0.53, 0.81, 0.92);\n"
    "   vec3 lavender = vec3(0.9, 0.8, 1.0);\n"
    "   vec3 peach = vec3(1.0, 0.85, 0.7);\n"
    "   vec3 color = mix(lavender, skyBlue, gradient);\n"
    "   color = mix(color, peach, sin(time * 0.3) * 0.2 + 0.2);\n"
    "   FragColor = vec4(color, 1.0);\n"
    "}\n";

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
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

void captureFrame(int width, int height, int frameNumber) {
    unsigned char* pixels = malloc(width * height * 3);
    if (!pixels) return;

    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    char filename[256];
    snprintf(filename, sizeof(filename), "frames/frame_%05d.ppm", frameNumber);

    FILE* f = fopen(filename, "wb");
    if (f) {
        fprintf(f, "P6\n%d %d\n255\n", width, height);
        for (int y = height - 1; y >= 0; y--) {
            fwrite(pixels + y * width * 3, 1, width * 3, f);
        }
        fclose(f);
    }

    free(pixels);
}

int main(int argc, char* argv[]) {
    int captureMode = (argc > 1 && strcmp(argv[1], "--capture") == 0);
    int captureSeconds = 30;
    int targetFPS = 30;

    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (captureMode) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Peaceful Waves", NULL, NULL);
    if (!window) {
        printf("Failed to create window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    GLenum err = glewInit();
    if (err != GLEW_OK) {
        printf("Failed to initialize GLEW: %s\n", glewGetErrorString(err));
        return -1;
    }

    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float vertices[] = {
        -1.0f, -0.3f, 0.0f,
         1.0f, -0.3f, 0.0f,
         1.0f,  0.3f, 0.0f,
        -1.0f,  0.3f, 0.0f,
        -1.0f, -0.5f, 0.0f,
         1.0f, -0.5f, 0.0f,
         1.0f,  0.1f, 0.0f,
        -1.0f,  0.1f, 0.0f,
        -1.0f, -0.7f, 0.0f,
         1.0f, -0.7f, 0.0f,
         1.0f, -0.1f, 0.0f,
        -1.0f, -0.1f, 0.0f
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0,
        4, 5, 6,
        6, 7, 4,
        8, 9, 10,
        10, 11, 8
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

    int frameCount = 0;
    int totalFrames = captureSeconds * targetFPS;
    double frameTime = 1.0 / targetFPS;
    double simulatedTime = 0.0;

    if (captureMode) {
        system("mkdir -p frames");
        printf("Capturing %d seconds at %d FPS (%d frames)...\n", captureSeconds, targetFPS, totalFrames);
    }

    while (!glfwWindowShouldClose(window)) {
        if (captureMode && frameCount >= totalFrames) {
            break;
        }

        glClearColor(0.95f, 0.95f, 0.98f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        float timeValue = captureMode ? simulatedTime : glfwGetTime();
        int timeLoc = glGetUniformLocation(shaderProgram, "time");
        glUniform1f(timeLoc, timeValue);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);

        if (captureMode) {
            captureFrame(800, 600, frameCount);
            frameCount++;
            simulatedTime += frameTime;

            if (frameCount % targetFPS == 0) {
                printf("Captured %d/%d seconds\n", frameCount / targetFPS, captureSeconds);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    if (captureMode) {
        printf("Capture complete! %d frames saved to frames/\n", frameCount);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}