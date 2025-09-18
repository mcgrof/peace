#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

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

void draw_wave(float time, float y_offset, float amplitude) {
    glBegin(GL_TRIANGLE_STRIP);
    for (float x = -1.0; x <= 1.0; x += 0.01) {
        float y = y_offset + sin(x * 3.0 + time) * amplitude;
        y += sin(x * 5.0 - time * 0.8) * amplitude * 0.5;

        float gradient = (y + 1.0) * 0.5;
        float r = 0.9 + gradient * 0.1;
        float g = 0.8 - gradient * 0.2;
        float b = 1.0 - gradient * 0.3;

        r = r * (0.8 + sin(time * 0.3) * 0.2);
        g = g * (0.9 + sin(time * 0.3) * 0.1);

        glColor3f(r, g, b);
        glVertex2f(x, y + 0.3);
        glVertex2f(x, y - 0.3);
    }
    glEnd();
}

int main(int argc, char* argv[]) {
    int captureMode = (argc > 1 && strcmp(argv[1], "--capture") == 0);
    int captureSeconds = 30;
    int targetFPS = 30;

    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }

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

        float timeValue = captureMode ? simulatedTime : glfwGetTime();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        draw_wave(timeValue, 0.0, 0.1);
        draw_wave(timeValue * 1.2, -0.3, 0.08);
        draw_wave(timeValue * 0.8, -0.6, 0.12);

        for (int i = 0; i < 5; i++) {
            float orb_time = timeValue + i * 1.256;
            float x = sin(orb_time * 0.7) * 0.8;
            float y = cos(orb_time * 0.5) * 0.3 + sin(orb_time) * 0.1;

            glBegin(GL_TRIANGLE_FAN);
            glColor4f(1.0, 0.9, 0.7, 0.6);
            for (int j = 0; j <= 20; j++) {
                float angle = j * 2.0 * 3.14159 / 20;
                glVertex2f(x + cos(angle) * 0.02, y + sin(angle) * 0.02);
            }
            glEnd();
        }

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

    glfwTerminate();
    return 0;
}