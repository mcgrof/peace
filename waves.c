#ifdef _WIN32
#include <GL/gl.h>
#include <windows.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>

float zoom = 1.0f;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    zoom += (float)yoffset * 0.1f;
    if (zoom < 0.5f) zoom = 0.5f;
    if (zoom > 3.0f) zoom = 3.0f;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0 * zoom, 1.0 * zoom, -1.0 * zoom, 1.0 * zoom, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
}

int main() {
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Peaceful Waves", NULL, NULL);
    if (!window) {
        printf("Failed to create window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSwapInterval(1);  // Enable vsync

    // Initialize OpenGL
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    framebuffer_size_callback(window, width, height);

    while (!glfwWindowShouldClose(window)) {
        float time = glfwGetTime();

        // Clear with deep ocean color
        glClearColor(0.05f, 0.15f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glLoadIdentity();

        // Draw multiple wave layers
        for (int layer = 0; layer < 5; layer++) {
            glBegin(GL_TRIANGLE_STRIP);

            float layerOffset = layer * 0.3f - 0.6f;
            float layerSpeed = 1.0f + layer * 0.3f;
            float layerAmplitude = 0.1f + layer * 0.02f;

            // Create wave vertices
            for (int i = 0; i <= 100; i++) {
                float x = (i / 50.0f) - 1.0f;

                // Multiple sine waves for complex motion
                float wave = 0.0f;
                wave += sin(x * 3.0f * zoom + time * layerSpeed) * layerAmplitude;
                wave += sin(x * 5.0f * zoom - time * layerSpeed * 0.7f) * layerAmplitude * 0.5f;
                wave += sin(x * 7.0f * zoom + time * layerSpeed * 1.3f) * layerAmplitude * 0.3f;

                float y = layerOffset + wave;

                // Calculate position-based gradient
                float gradient = (y + 1.0f) * 0.5f;
                float horizontalGradient = (x + 1.0f) * 0.5f;

                // Time-based color shifting
                float colorShift = sin(time * 0.3f) * 0.5f + 0.5f;
                float waveColorShift = sin(time * 0.5f + x * 2.0f) * 0.3f + 0.7f;

                // Define our color palette
                // Deep blue
                float deepR = 0.1f, deepG = 0.3f, deepB = 0.6f;
                // Sky blue
                float skyR = 0.53f, skyG = 0.81f, skyB = 0.92f;
                // Lavender
                float lavR = 0.9f, lavG = 0.8f, lavB = 1.0f;
                // Peach
                float peachR = 1.0f, peachG = 0.85f, peachB = 0.7f;

                // Mix colors based on position and time
                float r, g, b;
                if (gradient < 0.33f) {
                    float t = gradient * 3.0f;
                    r = deepR * (1.0f - t) + skyR * t;
                    g = deepG * (1.0f - t) + skyG * t;
                    b = deepB * (1.0f - t) + skyB * t;
                } else if (gradient < 0.66f) {
                    float t = (gradient - 0.33f) * 3.0f;
                    r = skyR * (1.0f - t) + lavR * t;
                    g = skyG * (1.0f - t) + lavG * t;
                    b = skyB * (1.0f - t) + lavB * t;
                } else {
                    float t = (gradient - 0.66f) * 3.0f;
                    r = lavR * (1.0f - t) + peachR * t;
                    g = lavG * (1.0f - t) + peachG * t;
                    b = lavB * (1.0f - t) + peachB * t;
                }

                // Add time-based color variation
                r = r * (0.7f + colorShift * 0.3f) * waveColorShift;
                g = g * (0.8f + colorShift * 0.2f) * waveColorShift;
                b = b * (0.9f + colorShift * 0.1f) * waveColorShift;

                // Add shimmer based on wave position
                float shimmer = sin(x * 20.0f + time * 3.0f) * 0.05f;
                r += shimmer;
                g += shimmer;
                b += shimmer * 1.2f;

                float a = 0.8f + layer * 0.04f;

                glColor4f(r, g, b, a);

                // Top vertex
                glVertex2f(x * 2.0f, y);

                // Bottom vertex
                glVertex2f(x * 2.0f, -2.0f);
            }

            glEnd();
        }

        // Draw floating orbs
        for (int i = 0; i < 8; i++) {
            float orbTime = time * 0.3f + i * 1.5f;
            float orbX = sin(orbTime * 0.7f + i * 2.0f) * 1.5f;
            float orbY = cos(orbTime * 0.5f + i * 1.3f) * 0.8f + sin(orbTime) * 0.2f;
            float orbSize = 0.02f + sin(orbTime * 2.0f) * 0.01f;

            // Orb glow effect - outer glow
            glBegin(GL_TRIANGLE_FAN);
            float glowSize = orbSize * 3.0f;
            glColor4f(1.0f, 0.9f, 0.7f, 0.1f);
            glVertex2f(orbX, orbY);
            for (int j = 0; j <= 20; j++) {
                float angle = j * 2.0f * 3.14159f / 20.0f;
                float x = orbX + cos(angle) * glowSize;
                float y = orbY + sin(angle) * glowSize;
                glColor4f(1.0f, 0.9f, 0.7f, 0.0f);
                glVertex2f(x, y);
            }
            glEnd();

            // Orb core
            glBegin(GL_TRIANGLE_FAN);
            float coreR = 1.0f;
            float coreG = 0.95f - sin(orbTime * 3.0f) * 0.1f;
            float coreB = 0.8f + sin(orbTime * 2.0f) * 0.2f;
            glColor4f(coreR, coreG, coreB, 0.9f);
            glVertex2f(orbX, orbY);
            for (int j = 0; j <= 20; j++) {
                float angle = j * 2.0f * 3.14159f / 20.0f;
                float x = orbX + cos(angle) * orbSize;
                float y = orbY + sin(angle) * orbSize;
                glVertex2f(x, y);
            }
            glEnd();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}