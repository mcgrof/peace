#ifdef _WIN32
    #include <windows.h>
    #include <GL/gl.h>
#elif defined(__APPLE__)
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
}

int main() {
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }

    GLFWwindow* window =
        glfwCreateWindow(800, 600, "Peaceful Waves", NULL, NULL);
    if (!window) {
        printf("Failed to create window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1);  // Enable vsync

    // Initialize projection
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    while (!glfwWindowShouldClose(window)) {
        float time = glfwGetTime();

        // Clear with a soft background gradient that shifts colors
        float bgHue = sin(time * 0.1) * 0.5 + 0.5;
        float bg = sin(time * 0.2) * 0.05 + 0.95;
        float bgR = bg * (0.8 + bgHue * 0.2);
        float bgG = bg * (0.85 + sin(time * 0.15) * 0.1);
        float bgB = bg * (0.95 - bgHue * 0.15);
        glClearColor(bgR, bgG, bgB, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw multiple wave layers
        glBegin(GL_QUAD_STRIP);
        for (int layer = 0; layer < 5; layer++) {
            float layerOffset = layer * 0.15;
            float speed = 1.0 + layer * 0.3;
            float amplitude = 0.1 - layer * 0.015;

            // Set color for this layer - cycling through different hues over
            // time
            float hueShift = sin(time * 0.15) * 0.5 + 0.5;  // 0 to 1
            float phaseShift = sin(time * 0.1 + layer * 0.7) * 0.3;

            // Transition between different color schemes
            float r = 0.3 + hueShift * 0.4 +
                      sin(time * 0.3 + layer + phaseShift) * 0.3;
            float g =
                0.5 + sin(time * 0.25 + layer * 0.5) * 0.3 + hueShift * 0.2;
            float b = 0.9 - hueShift * 0.4 + cos(time * 0.2 + layer) * 0.2;
            float alpha = 0.6 - layer * 0.1;

            glColor4f(r, g, b, alpha);

            for (int i = 0; i <= 100; i++) {
                float x = (float)i / 50.0 - 1.0;
                float wave1 = sin(x * 3.14159 * 2 + time * speed) * amplitude;
                float wave2 =
                    sin(x * 3.14159 * 3 - time * speed * 0.7) * amplitude * 0.5;
                float y = wave1 + wave2 - 0.5 + layerOffset;

                glVertex2f(x, y);
                glVertex2f(x, -1.0);
            }
        }
        glEnd();

        // Draw some floating circles for a peaceful effect
        for (int i = 0; i < 8; i++) {
            float circleTime = time * 0.3 + i * 0.8;
            float x = sin(circleTime) * 0.8;
            float y = cos(circleTime * 0.7) * 0.3 + sin(time + i) * 0.1;
            float size = 0.02 + sin(time * 2 + i) * 0.01;

            // Make orbs change color too
            float orbHue = sin(time * 0.2 + i) * 0.5 + 0.5;
            glColor4f(0.9 + orbHue * 0.1, 0.85 + sin(time * 0.3 + i) * 0.15,
                      0.8 + cos(time * 0.25) * 0.2, 0.3);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(x, y);
            for (int j = 0; j <= 20; j++) {
                float angle = j * 2.0 * 3.14159 / 20.0;
                glVertex2f(x + cos(angle) * size, y + sin(angle) * size * 0.8);
            }
            glEnd();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}