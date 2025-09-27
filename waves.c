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
float clickX = 0.0f;
float clickY = 0.0f;
float clickTime = -10.0f;
int mousePressed = 0;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    zoom += (float)yoffset * 0.1f;
    if (zoom < 0.5f) zoom = 0.5f;
    if (zoom > 3.0f) zoom = 3.0f;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            // Convert to OpenGL coordinates accounting for zoom
            clickX = ((xpos / width) * 2.0f - 1.0f) * zoom;
            clickY = -((ypos / height) * 2.0f - 1.0f) * zoom;
            clickTime = glfwGetTime();
            mousePressed = 1;
        } else if (action == GLFW_RELEASE) {
            mousePressed = 0;
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (mousePressed) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // Update position continuously while dragging
        clickX = ((xpos / width) * 2.0f - 1.0f) * zoom;
        clickY = -((ypos / height) * 2.0f - 1.0f) * zoom;
        // Don't reset clickTime to keep the effect continuous
    }
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
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
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

                // Add ripple and vortex effects from mouse
                float dx = (x * 2.0f) - clickX;
                float dy = layerOffset - clickY;
                float dist = sqrt(dx * dx + dy * dy);

                float timeSinceClick = time - clickTime;
                if (timeSinceClick >= 0.0f && timeSinceClick < 5.0f) {
                    // Create expanding ripples
                    float rippleSpeed = 3.0f;
                    float rippleRadius = timeSinceClick * rippleSpeed;
                    float rippleWidth = 0.3f;

                    // Multiple ripple rings
                    for (int r = 0; r < 3; r++) {
                        float ringOffset = r * 0.5f;
                        float ringDist = fabs(dist - (rippleRadius - ringOffset));
                        if (ringDist < rippleWidth) {
                            float rippleStrength = (1.0f - ringDist / rippleWidth) * (1.0f - timeSinceClick / 5.0f);
                            wave += sin(dist * 10.0f - time * 5.0f) * rippleStrength * 0.3f;
                        }
                    }
                }

                // Add vortex/whirlpool effect when mouse is held down
                if (mousePressed && dist < 1.0f) {
                    float vortexStrength = (1.0f - dist) * 0.5f;
                    float angle = atan2(dy, dx);

                    // Swirling motion
                    wave += sin(angle * 5.0f + time * 10.0f - dist * 20.0f) * vortexStrength;

                    // Pulling effect toward center
                    wave -= dist * vortexStrength * 0.3f;

                    // Add chaotic turbulence
                    wave += sin(x * 50.0f + time * 20.0f) * cos(dy * 50.0f) * vortexStrength * 0.2f;

                    // Pulsing effect
                    wave *= 1.0f + sin(time * 15.0f) * vortexStrength * 0.3f;
                }

                float y = layerOffset + wave;

                // Calculate position-based gradient
                float gradient = (y + 1.0f) * 0.5f;

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

        // Draw click explosion particles and continuous effects
        float timeSinceClick = time - clickTime;
        if (mousePressed) {
            // Continuous vortex particles while holding
            for (int p = 0; p < 30; p++) {
                float angle = p * 3.14159f * 2.0f / 30.0f + time * 3.0f;
                float radius = sin(time * 2.0f + p * 0.5f) * 0.3f + 0.2f;
                float px = clickX + cos(angle) * radius;
                float py = clickY + sin(angle) * radius;

                glBegin(GL_TRIANGLE_FAN);
                // Rainbow colors
                float r = sin(p * 0.3f + time * 5.0f) * 0.5f + 0.5f;
                float g = cos(p * 0.3f + time * 5.0f) * 0.5f + 0.5f;
                float b = sin(p * 0.3f + time * 5.0f + 3.14159f) * 0.5f + 0.5f;
                glColor4f(r, g, b, 0.7f);
                glVertex2f(px, py);
                for (int j = 0; j <= 8; j++) {
                    float a = j * 2.0f * 3.14159f / 8.0f;
                    float size = 0.02f + sin(time * 10.0f + p) * 0.01f;
                    glVertex2f(px + cos(a) * size, py + sin(a) * size);
                }
                glEnd();
            }
        } else if (timeSinceClick >= 0.0f && timeSinceClick < 3.0f) {
            // Explosion particles after release
            for (int p = 0; p < 20; p++) {
                float angle = p * 3.14159f * 2.0f / 20.0f;
                float particleSpeed = 0.5f + (p % 3) * 0.2f;
                float px = clickX + cos(angle) * timeSinceClick * particleSpeed;
                float py = clickY + sin(angle) * timeSinceClick * particleSpeed - timeSinceClick * timeSinceClick * 0.1f;
                float particleSize = 0.03f * (1.0f - timeSinceClick / 3.0f);

                glBegin(GL_TRIANGLE_FAN);
                float intensity = 1.0f - timeSinceClick / 3.0f;
                glColor4f(1.0f, 0.5f + sin(p + time * 5.0f) * 0.5f, 0.2f, intensity);
                glVertex2f(px, py);
                for (int j = 0; j <= 8; j++) {
                    float a = j * 2.0f * 3.14159f / 8.0f;
                    glVertex2f(px + cos(a) * particleSize, py + sin(a) * particleSize);
                }
                glEnd();
            }
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