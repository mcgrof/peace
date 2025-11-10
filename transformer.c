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
#include <string.h>

#define PI 3.14159265359f
#define NUM_TOKENS 5
#define NUM_LAYERS 6

// Camera state
float zoom = 0.7f;  // Start very zoomed out to see everything
float cameraAngle = 0.0f;
float cameraElevation = 0.5f;
float cameraY = -3.0f;  // Start lower to see words

// Token colors and labels
typedef struct {
    float r, g, b;
    const char* label;
} Token;

Token tokens[NUM_TOKENS] = {
    {1.0f, 0.5f, 0.5f, "THE"},
    {0.5f, 1.0f, 0.5f, "DOG"},
    {0.5f, 0.5f, 1.0f, "SAT"},
    {1.0f, 1.0f, 0.4f, "ON"},
    {1.0f, 0.5f, 1.0f, "MAT"}
};

// Token positions through layers (simulated residual stream)
typedef struct {
    float x, y, z;
} Vec3;

Vec3 tokenPositions[NUM_TOKENS][NUM_LAYERS];
float animationPhase = 0.0f;
int currentLayer = 0;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    zoom += (float)yoffset * 0.2f;
    if (zoom < 0.5f) zoom = 0.5f;  // Allow much more zoom out
    if (zoom > 8.0f) zoom = 8.0f;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (float)width / (float)height;
    float fov = 60.0f;
    float near = 0.1f;
    float far = 100.0f;
    float top = near * tanf(fov * PI / 360.0f);
    float right = top * aspect;
    glFrustum(-right, right, -top, top, near, far);
    glMatrixMode(GL_MODELVIEW);
}

void drawSphere(float x, float y, float z, float radius, float r, float g, float b, float alpha) {
    int segments = 16;
    int rings = 12;

    glBegin(GL_TRIANGLES);
    for (int ring = 0; ring < rings; ring++) {
        float phi0 = PI * ring / rings;
        float phi1 = PI * (ring + 1) / rings;

        for (int seg = 0; seg < segments; seg++) {
            float theta0 = 2.0f * PI * seg / segments;
            float theta1 = 2.0f * PI * (seg + 1) / segments;

            // Vertex positions
            float x0 = sinf(phi0) * cosf(theta0);
            float y0 = cosf(phi0);
            float z0 = sinf(phi0) * sinf(theta0);

            float x1 = sinf(phi0) * cosf(theta1);
            float y1 = cosf(phi0);
            float z1 = sinf(phi0) * sinf(theta1);

            float x2 = sinf(phi1) * cosf(theta1);
            float y2 = cosf(phi1);
            float z2 = sinf(phi1) * sinf(theta1);

            float x3 = sinf(phi1) * cosf(theta0);
            float y3 = cosf(phi1);
            float z3 = sinf(phi1) * sinf(theta0);

            // Lighting based on normal (which equals position for unit sphere)
            float brightness0 = 0.5f + 0.5f * (x0 * 0.5f + y0 * 0.5f + z0 * 0.3f);
            float brightness1 = 0.5f + 0.5f * (x1 * 0.5f + y1 * 0.5f + z1 * 0.3f);
            float brightness2 = 0.5f + 0.5f * (x2 * 0.5f + y2 * 0.5f + z2 * 0.3f);
            float brightness3 = 0.5f + 0.5f * (x3 * 0.5f + y3 * 0.5f + z3 * 0.3f);

            // First triangle
            glColor4f(r * brightness0, g * brightness0, b * brightness0, alpha);
            glVertex3f(x + x0 * radius, y + y0 * radius, z + z0 * radius);
            glColor4f(r * brightness1, g * brightness1, b * brightness1, alpha);
            glVertex3f(x + x1 * radius, y + y1 * radius, z + z1 * radius);
            glColor4f(r * brightness2, g * brightness2, b * brightness2, alpha);
            glVertex3f(x + x2 * radius, y + y2 * radius, z + z2 * radius);

            // Second triangle
            glColor4f(r * brightness0, g * brightness0, b * brightness0, alpha);
            glVertex3f(x + x0 * radius, y + y0 * radius, z + z0 * radius);
            glColor4f(r * brightness2, g * brightness2, b * brightness2, alpha);
            glVertex3f(x + x2 * radius, y + y2 * radius, z + z2 * radius);
            glColor4f(r * brightness3, g * brightness3, b * brightness3, alpha);
            glVertex3f(x + x3 * radius, y + y3 * radius, z + z3 * radius);
        }
    }
    glEnd();
}

void drawVector(Vec3 from, Vec3 to, float r, float g, float b, float alpha) {
    // Draw dashed line
    int segments = 10;
    glBegin(GL_LINES);
    for (int i = 0; i < segments; i += 2) {
        float t0 = (float)i / segments;
        float t1 = (float)(i + 1) / segments;

        glColor4f(r, g, b, alpha);
        glVertex3f(from.x + t0 * (to.x - from.x),
                   from.y + t0 * (to.y - from.y),
                   from.z + t0 * (to.z - from.z));
        glVertex3f(from.x + t1 * (to.x - from.x),
                   from.y + t1 * (to.y - from.y),
                   from.z + t1 * (to.z - from.z));
    }
    glEnd();

    // Draw arrowhead
    float arrowSize = 0.08f;
    Vec3 dir = {to.x - from.x, to.y - from.y, to.z - from.z};
    float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len > 0.001f) {
        dir.x /= len; dir.y /= len; dir.z /= len;
        Vec3 perp1 = {-dir.y, dir.x, 0};

        glBegin(GL_TRIANGLES);
        glColor4f(r, g, b, alpha);
        glVertex3f(to.x, to.y, to.z);
        glVertex3f(to.x - dir.x * arrowSize + perp1.x * arrowSize * 0.3f,
                   to.y - dir.y * arrowSize + perp1.y * arrowSize * 0.3f,
                   to.z - dir.z * arrowSize + perp1.z * arrowSize * 0.3f);
        glVertex3f(to.x - dir.x * arrowSize - perp1.x * arrowSize * 0.3f,
                   to.y - dir.y * arrowSize - perp1.y * arrowSize * 0.3f,
                   to.z - dir.z * arrowSize - perp1.z * arrowSize * 0.3f);
        glEnd();
    }
}

void drawTrajectoryTube(int tokenIdx, int endLayer) {
    // Draw translucent tube connecting positions
    int segments = 20;
    float radius = 0.015f;

    for (int layer = 0; layer < endLayer; layer++) {
        Vec3 p0 = tokenPositions[tokenIdx][layer];
        Vec3 p1 = tokenPositions[tokenIdx][layer + 1];

        // Determine color based on layer (alternating attention/FFN)
        float r, g, b;
        if (layer % 2 == 0) {
            // Attention - blue
            r = 0.3f; g = 0.5f; b = 1.0f;
        } else {
            // FFN - orange
            r = 1.0f; g = 0.5f; b = 0.2f;
        }

        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= segments; i++) {
            float angle = 2.0f * PI * i / segments;
            float nx = cosf(angle);
            float nz = sinf(angle);

            float alpha = 0.3f;
            glColor4f(r, g, b, alpha);
            glVertex3f(p0.x + nx * radius, p0.y, p0.z + nz * radius);
            glVertex3f(p1.x + nx * radius, p1.y, p1.z + nz * radius);
        }
        glEnd();
    }
}

void initializeTokenPositions() {
    // Initialize starting positions (layer 0 - embeddings)
    for (int i = 0; i < NUM_TOKENS; i++) {
        float angle = 2.0f * PI * i / NUM_TOKENS;
        tokenPositions[i][0].x = cosf(angle) * 0.8f;
        tokenPositions[i][0].y = -2.0f;
        tokenPositions[i][0].z = sinf(angle) * 0.8f;
    }

    // Simulate residual stream trajectory through layers
    for (int layer = 1; layer < NUM_LAYERS; layer++) {
        for (int i = 0; i < NUM_TOKENS; i++) {
            Vec3 prev = tokenPositions[i][layer - 1];

            if (layer % 2 == 1) {
                // Attention: tokens attend to each other
                Vec3 delta = {0, 0, 0};
                for (int j = 0; j < NUM_TOKENS; j++) {
                    if (i != j) {
                        // Attention weight (simulated)
                        float weight = 0.2f / (NUM_TOKENS - 1);
                        Vec3 other = tokenPositions[j][layer - 1];
                        delta.x += weight * (other.x - prev.x);
                        delta.y += weight * (other.y - prev.y);
                        delta.z += weight * (other.z - prev.z);
                    }
                }
                tokenPositions[i][layer].x = prev.x + delta.x;
                tokenPositions[i][layer].y = prev.y + delta.y + 0.4f;
                tokenPositions[i][layer].z = prev.z + delta.z;
            } else {
                // FFN: non-linear position-wise transformation to abstract space
                // Each token rotates/projects independently into unexpected positions
                float tokenPhase = i * 1.7f + layer * 2.3f;  // Non-uniform angles
                float expansion = 0.3f + sinf(tokenPhase) * 0.2f;  // Varying expansion

                // Non-linear projection - some tokens move out, some move in
                tokenPositions[i][layer].x = prev.x * cosf(tokenPhase * 0.5f) - prev.z * sinf(tokenPhase * 0.5f) + cosf(tokenPhase) * expansion;
                tokenPositions[i][layer].y = prev.y + 0.4f;
                tokenPositions[i][layer].z = prev.x * sinf(tokenPhase * 0.5f) + prev.z * cosf(tokenPhase * 0.5f) + sinf(tokenPhase * 1.3f) * expansion;
            }
        }
    }
}

void drawBlock(float x, float y, float z, float w, float h) {
    // Draw a filled rectangle
    glBegin(GL_QUADS);
    glVertex3f(x, y, z);
    glVertex3f(x + w, y, z);
    glVertex3f(x + w, y + h, z);
    glVertex3f(x, y + h, z);
    glEnd();
}

void drawLetter(char letter, float x, float y, float z, float size) {
    // Chunky block letters using filled rectangles
    float w = size * 0.7f;
    float h = size;
    float thick = size * 0.25f;  // SUPER thick strokes for readability

    switch(letter) {
        case 'T':
            drawBlock(x, y + h - thick, z, w, thick);  // Top bar
            drawBlock(x + w/2 - thick/2, y, z, thick, h);  // Vertical
            break;
        case 'H':
            drawBlock(x, y, z, thick, h);  // Left vertical
            drawBlock(x + w - thick, y, z, thick, h);  // Right vertical
            drawBlock(x, y + h/2 - thick/2, z, w, thick);  // Middle bar
            break;
        case 'E':
            drawBlock(x, y, z, thick, h);  // Left vertical
            drawBlock(x, y + h - thick, z, w, thick);  // Top bar
            drawBlock(x, y + h/2 - thick/2, z, w * 0.8f, thick);  // Middle bar
            drawBlock(x, y, z, w, thick);  // Bottom bar
            break;
        case 'D':
            drawBlock(x, y, z, thick, h);  // Left vertical
            drawBlock(x, y + h - thick, z, w * 0.7f, thick);  // Top bar
            drawBlock(x, y, z, w * 0.7f, thick);  // Bottom bar
            drawBlock(x + w * 0.7f - thick, y + thick, z, thick, h - thick * 2);  // Right vertical
            break;
        case 'O':
            drawBlock(x, y, z, thick, h);  // Left vertical
            drawBlock(x + w - thick, y, z, thick, h);  // Right vertical
            drawBlock(x, y + h - thick, z, w, thick);  // Top bar
            drawBlock(x, y, z, w, thick);  // Bottom bar
            break;
        case 'G':
            drawBlock(x, y, z, thick, h);  // Left vertical
            drawBlock(x, y + h - thick, z, w, thick);  // Top bar
            drawBlock(x, y, z, w, thick);  // Bottom bar
            drawBlock(x + w - thick, y, z, thick, h/2);  // Right vertical (half)
            drawBlock(x + w/2, y + h/2 - thick/2, z, w/2, thick);  // Middle bar
            break;
        case 'S':
            drawBlock(x, y + h - thick, z, w, thick);  // Top bar
            drawBlock(x, y + h/2 - thick/2, z, w, thick);  // Middle bar
            drawBlock(x, y, z, w, thick);  // Bottom bar
            drawBlock(x, y + h/2, z, thick, h/2);  // Top-left vertical
            drawBlock(x + w - thick, y, z, thick, h/2);  // Bottom-right vertical
            break;
        case 'A':
            drawBlock(x, y, z, thick, h);  // Left vertical
            drawBlock(x + w - thick, y, z, thick, h);  // Right vertical
            drawBlock(x, y + h - thick, z, w, thick);  // Top bar
            drawBlock(x, y + h/2 - thick/2, z, w, thick);  // Middle bar
            break;
        case 'M':
            drawBlock(x, y, z, thick, h);  // Left vertical
            drawBlock(x + w - thick, y, z, thick, h);  // Right vertical
            drawBlock(x + w/2 - thick/2, y + h/2, z, thick, h/2);  // Middle vertical
            break;
        case 'N':
            drawBlock(x, y, z, thick, h);  // Left vertical
            drawBlock(x + w - thick, y, z, thick, h);  // Right vertical
            // Diagonal - simplified as blocks
            drawBlock(x + w/3 - thick/2, y + h/3, z, thick, h/3);
            break;
        case 'I':
            drawBlock(x + w/2 - thick/2, y, z, thick, h);  // Vertical
            drawBlock(x, y, z, w, thick);  // Bottom bar
            drawBlock(x, y + h - thick, z, w, thick);  // Top bar
            break;
        case 'L':
            drawBlock(x, y, z, thick, h);  // Vertical
            drawBlock(x, y, z, w, thick);  // Bottom bar
            break;
        case 'R':
            drawBlock(x, y, z, thick, h);  // Left vertical
            drawBlock(x, y + h - thick, z, w, thick);  // Top bar
            drawBlock(x, y + h/2 - thick/2, z, w * 0.7f, thick);  // Middle bar
            drawBlock(x + w * 0.7f - thick, y + h/2 - thick/2, z, thick, h/2 + thick/2);  // Right vertical upper
            break;
        case 'F':
            drawBlock(x, y, z, thick, h);  // Left vertical
            drawBlock(x, y + h - thick, z, w, thick);  // Top bar
            drawBlock(x, y + h/2 - thick/2, z, w * 0.7f, thick);  // Middle bar
            break;
        case 'W':
            drawBlock(x, y, z, thick, h);  // Left vertical
            drawBlock(x + w - thick, y, z, thick, h);  // Right vertical
            drawBlock(x + w/2 - thick/2, y, z, thick, h * 0.6f);  // Middle vertical
            break;
        case 'Y':
            drawBlock(x + w/2 - thick/2, y, z, thick, h/2);  // Bottom vertical
            drawBlock(x, y + h/2, z, thick, h/2);  // Top-left vertical
            drawBlock(x + w - thick, y + h/2, z, thick, h/2);  // Top-right vertical
            break;
        case 'V':
            drawBlock(x, y + h/2, z, thick, h/2);  // Top-left
            drawBlock(x + w - thick, y + h/2, z, thick, h/2);  // Top-right
            drawBlock(x + w/2 - thick/2, y, z, thick, h/2);  // Bottom middle
            break;
    }
}

void drawWord(const char* word, float x, float y, float z, float size) {
    float spacing = size * 0.7f;  // More spacing for wider letters
    float offset = 0;
    for (int i = 0; word[i] != '\0'; i++) {
        drawLetter(word[i], x + offset, y, z, size);
        offset += spacing;
    }
}

void draw2DText(const char* text, float x, float y, float size, float r, float g, float b, float a) {
    // Draw text in 2D screen space (x, y in pixels from bottom-left)
    glColor4f(r, g, b, a);
    float spacing = size * 0.7f;  // Better spacing
    float offset = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] == ' ') {
            offset += spacing * 0.6f;
        } else {
            drawLetter(text[i], x + offset, y, 0, size);
            offset += spacing;
        }
    }
}

void setupTextOverlay(int width, int height) {
    // Switch to 2D orthographic projection for text overlay
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
}

void restoreFromTextOverlay() {
    // Restore 3D projection
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
}

void drawDigit(int digit, float x, float y, float z, float size) {
    // Draw digits 0-9 using simple line segments
    float segments[10][7] = {
        {1,1,1,0,1,1,1}, // 0
        {0,0,1,0,0,1,0}, // 1
        {1,0,1,1,1,0,1}, // 2
        {1,0,1,1,0,1,1}, // 3
        {0,1,1,1,0,1,0}, // 4
        {1,1,0,1,0,1,1}, // 5
        {1,1,0,1,1,1,1}, // 6
        {1,0,1,0,0,1,0}, // 7
        {1,1,1,1,1,1,1}, // 8
        {1,1,1,1,0,1,1}  // 9
    };

    if (digit < 0 || digit > 9) return;

    float w = size * 0.4f;
    float h = size;

    glLineWidth(3.0f);
    glBegin(GL_LINES);

    // Top horizontal
    if (segments[digit][0]) {
        glVertex3f(x, y + h, z);
        glVertex3f(x + w, y + h, z);
    }
    // Top-left vertical
    if (segments[digit][1]) {
        glVertex3f(x, y + h, z);
        glVertex3f(x, y + h/2, z);
    }
    // Top-right vertical
    if (segments[digit][2]) {
        glVertex3f(x + w, y + h, z);
        glVertex3f(x + w, y + h/2, z);
    }
    // Middle horizontal
    if (segments[digit][3]) {
        glVertex3f(x, y + h/2, z);
        glVertex3f(x + w, y + h/2, z);
    }
    // Bottom-left vertical
    if (segments[digit][4]) {
        glVertex3f(x, y + h/2, z);
        glVertex3f(x, y, z);
    }
    // Bottom-right vertical
    if (segments[digit][5]) {
        glVertex3f(x + w, y + h/2, z);
        glVertex3f(x + w, y, z);
    }
    // Bottom horizontal
    if (segments[digit][6]) {
        glVertex3f(x, y, z);
        glVertex3f(x + w, y, z);
    }

    glEnd();
    glLineWidth(2.0f);
}

void drawLayerPlane(int layer, float alpha) {
    float y = tokenPositions[0][layer].y;
    float size = 2.0f;

    // Draw more visible plane
    glBegin(GL_QUADS);
    glColor4f(0.3f, 0.4f, 0.5f, alpha * 0.2f);
    glVertex3f(-size, y, -size);
    glVertex3f(size, y, -size);
    glColor4f(0.4f, 0.5f, 0.6f, alpha * 0.15f);
    glVertex3f(size, y, size);
    glVertex3f(-size, y, size);
    glEnd();

    // Draw grid lines
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glColor4f(0.5f, 0.6f, 0.7f, alpha * 0.3f);
    for (int i = -4; i <= 4; i++) {
        float pos = i * 0.5f;
        glVertex3f(pos, y, -size);
        glVertex3f(pos, y, size);
        glVertex3f(-size, y, pos);
        glVertex3f(size, y, pos);
    }
    glEnd();
    glLineWidth(2.0f);

    // Draw layer number
    glColor4f(1.0f, 1.0f, 1.0f, alpha * 0.8f);
    drawDigit(layer, -size + 0.2f, y + 0.05f, size - 0.3f, 0.3f);
}

int main() {
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(1200, 900, "Transformer Residual Stream", NULL, NULL);
    if (!window) {
        printf("Failed to create window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSwapInterval(1);

    // Initialize OpenGL
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glLineWidth(2.0f);
    glEnable(GL_POINT_SMOOTH);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    framebuffer_size_callback(window, width, height);

    initializeTokenPositions();

    printf("=== Transformer Residual Stream Visualization ===\n");
    printf("Watch the on-screen text for explanations!\n");
    printf("Scroll mouse wheel to zoom in/out\n\n");

    while (!glfwWindowShouldClose(window)) {
        float time = glfwGetTime();

        // Beautiful gradient background (ocean to sunset)
        float colorPhase = sinf(time * 0.1f) * 0.5f + 0.5f;
        float bgR = 0.05f + colorPhase * 0.1f;
        float bgG = 0.15f + colorPhase * 0.2f;
        float bgB = 0.3f + colorPhase * 0.1f;
        glClearColor(bgR, bgG, bgB, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Animation: slowly progress through layers
        // Pause at start to show the words
        if (time < 3.0f) {
            animationPhase = 0.0f;  // Hold at layer 0 for first 3 seconds
        } else {
            animationPhase += 0.005f;
        }

        // Cycle through layers with pauses
        float cycleLength = NUM_LAYERS * 2.0f;  // 2 seconds per layer
        float cyclePhase = fmodf(animationPhase, cycleLength);
        currentLayer = (int)(cyclePhase / 2.0f);
        if (currentLayer >= NUM_LAYERS) currentLayer = NUM_LAYERS - 1;

        float layerBlend = fmodf(cyclePhase, 2.0f);
        if (layerBlend > 1.0f) layerBlend = 1.0f;  // Pause at each layer

        // Camera setup - moves up as layers progress
        glLoadIdentity();

        // Camera follows the action - starts at bottom (words) and moves up through layers
        float targetY = -2.5f + (currentLayer + layerBlend) * 0.4f;  // Follow the layers up
        cameraY += (targetY - cameraY) * 0.05f;  // Smooth following

        // Position camera to see current layer
        glTranslatef(0, cameraY, -10.0f / zoom);  // Pull back more
        glRotatef(30.0f, 1, 0, 0);  // Tilt down more to see action
        glRotatef(0.0f, 0, 1, 0);  // No rotation - keep it stable

        // (Debug output removed - now shown on-screen)

        // Draw attention links between tokens at current layer
        if (currentLayer > 0 && currentLayer % 2 == 1 && layerBlend > 0.2f) {
            // Attention layer - show cross-token connections
            float linkAlpha = layerBlend * (0.3f + currentLayer * 0.1f); // Stronger at higher layers
            float linkWidth = 1.0f + currentLayer * 0.5f; // Thicker at higher layers

            glLineWidth(linkWidth);
            glEnable(GL_LINE_SMOOTH);

            for (int i = 0; i < NUM_TOKENS; i++) {
                Vec3 from = tokenPositions[i][currentLayer];
                for (int j = i + 1; j < NUM_TOKENS; j++) {
                    Vec3 to = tokenPositions[j][currentLayer];

                    // Attention link with pulsing effect
                    float pulse = sinf(time * 3.0f + i + j) * 0.3f + 0.7f;

                    glBegin(GL_LINES);
                    // Gradient from source to destination
                    glColor4f(tokens[i].r * 0.7f + 0.3f, tokens[i].g * 0.7f + 0.3f, tokens[i].b * 0.7f + 0.5f, linkAlpha * pulse);
                    glVertex3f(from.x, from.y, from.z);
                    glColor4f(tokens[j].r * 0.7f + 0.3f, tokens[j].g * 0.7f + 0.3f, tokens[j].b * 0.7f + 0.5f, linkAlpha * pulse);
                    glVertex3f(to.x, to.y, to.z);
                    glEnd();
                }
            }

            glLineWidth(2.0f);
        }

        // Draw FFN transformation indicators
        if (currentLayer > 0 && currentLayer % 2 == 0 && layerBlend > 0.2f) {
            // FFN layer - show rotation/transformation happening
            float transformAlpha = layerBlend * 0.5f;

            // Draw spinning circles around each token to show non-linear transformation
            for (int i = 0; i < NUM_TOKENS; i++) {
                Vec3 pos = tokenPositions[i][currentLayer];
                float radius = 0.25f;
                int segments = 20;

                glBegin(GL_LINE_LOOP);
                for (int seg = 0; seg < segments; seg++) {
                    float angle = 2.0f * PI * seg / segments + time * 2.0f + i;
                    float x = pos.x + cosf(angle) * radius;
                    float z = pos.z + sinf(angle) * radius;

                    // Orange glow for FFN
                    float segAlpha = transformAlpha * (0.5f + 0.5f * sinf(angle * 3.0f));
                    glColor4f(1.0f, 0.5f, 0.2f, segAlpha);
                    glVertex3f(x, pos.y, z);
                }
                glEnd();
            }
        }

        // Draw token words below layer 0 and embedding vectors
        float wordY = tokenPositions[0][0].y - 1.5f;  // Much further below layer 0
        for (int i = 0; i < NUM_TOKENS; i++) {
            Vec3 wordPos = tokenPositions[i][0];
            wordPos.y = wordY;

            // Draw the word label - MASSIVE and bright
            glColor4f(tokens[i].r * 1.5f, tokens[i].g * 1.5f, tokens[i].b * 1.5f, 1.0f);
            float wordSize = 0.6f;  // MASSIVE text

            // Calculate actual word width based on letter count
            int letterCount = 0;
            for (int j = 0; tokens[i].label[j] != '\0'; j++) letterCount++;
            float wordWidth = wordSize * 0.7f * letterCount;  // Use actual spacing

            drawWord(tokens[i].label, wordPos.x - wordWidth / 2, wordPos.y, wordPos.z, wordSize);

            // Draw embedding vector (word -> layer 0 position)
            Vec3 embeddingPos = tokenPositions[i][0];
            glLineWidth(3.0f);
            glBegin(GL_LINES);
            glColor4f(tokens[i].r * 0.8f, tokens[i].g * 0.8f, tokens[i].b * 0.8f, 0.7f);
            glVertex3f(wordPos.x, wordPos.y + wordSize * 1.1f, wordPos.z);  // Top of word
            glColor4f(tokens[i].r, tokens[i].g, tokens[i].b, 0.9f);
            glVertex3f(embeddingPos.x, embeddingPos.y - 0.1f, embeddingPos.z);  // Just below layer 0 orb
            glEnd();

            // Draw arrowhead at embedding position
            float arrowSize = 0.08f;
            glBegin(GL_TRIANGLES);
            glColor4f(tokens[i].r, tokens[i].g, tokens[i].b, 0.9f);
            glVertex3f(embeddingPos.x, embeddingPos.y - 0.1f, embeddingPos.z);
            glVertex3f(embeddingPos.x - arrowSize, embeddingPos.y - 0.1f - arrowSize * 1.5f, embeddingPos.z);
            glVertex3f(embeddingPos.x + arrowSize, embeddingPos.y - 0.1f - arrowSize * 1.5f, embeddingPos.z);
            glEnd();
            glLineWidth(2.0f);
        }

        // Draw layer planes
        for (int layer = 0; layer <= currentLayer; layer++) {
            float alpha = (layer == currentLayer) ? layerBlend : 1.0f;
            drawLayerPlane(layer, alpha);
        }

        // Draw subtle trajectories (history trails) up to current layer
        glDepthMask(GL_FALSE);
        for (int i = 0; i < NUM_TOKENS; i++) {
            if (currentLayer > 0) {
                // Draw a faint trail showing where the token has been
                for (int layer = 0; layer < currentLayer; layer++) {
                    Vec3 from = tokenPositions[i][layer];
                    Vec3 to = tokenPositions[i][layer + 1];

                    // Fade older trails
                    float trailAlpha = 0.1f * (1.0f - (float)(currentLayer - layer) / currentLayer);

                    glLineWidth(1.0f);
                    glBegin(GL_LINES);
                    glColor4f(tokens[i].r, tokens[i].g, tokens[i].b, trailAlpha);
                    glVertex3f(from.x, from.y, from.z);
                    glVertex3f(to.x, to.y, to.z);
                    glEnd();
                }
            }
        }
        glDepthMask(GL_TRUE);
        glLineWidth(2.0f);

        // Disable depth writes for transparent objects
        glDepthMask(GL_FALSE);

        // Draw token orbs - growing larger through layers
        for (int i = 0; i < NUM_TOKENS; i++) {
            // Draw at current layer position
            Vec3 pos = tokenPositions[i][currentLayer];
            if (currentLayer < NUM_LAYERS - 1 && layerBlend > 0.5f) {
                // Interpolate to next layer
                Vec3 nextPos = tokenPositions[i][currentLayer + 1];
                float t = (layerBlend - 0.5f) * 2.0f;
                pos.x = pos.x + t * (nextPos.x - pos.x);
                pos.y = pos.y + t * (nextPos.y - pos.y);
                pos.z = pos.z + t * (nextPos.z - pos.z);
            }

            // Orbs grow larger through layers (more refined representations)
            float layerProgress = (float)currentLayer / (float)(NUM_LAYERS - 1);
            float baseSize = 0.08f + layerProgress * 0.08f;  // Grow from 0.08 to 0.16

            // Pulsing glow effect
            float pulse = sinf(time * 2.0f + i) * 0.5f + 0.5f;
            float glowRadius = baseSize * 2.0f + pulse * 0.03f;

            // Outer glow (more pronounced at higher layers)
            float glowIntensity = 0.2f + layerProgress * 0.2f;
            drawSphere(pos.x, pos.y, pos.z, glowRadius,
                      tokens[i].r, tokens[i].g, tokens[i].b, glowIntensity);

            // Core orb - brighter at higher layers
            float coreIntensity = 0.8f + layerProgress * 0.2f;
            drawSphere(pos.x, pos.y, pos.z, baseSize,
                      tokens[i].r * coreIntensity, tokens[i].g * coreIntensity, tokens[i].b * coreIntensity, 1.0f);
        }

        // Re-enable depth writes
        glDepthMask(GL_TRUE);

        // Draw text overlay HUD - HUGE and simple
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        setupTextOverlay(width, height);

        // Simple status text - MUCH bigger
        if (time < 3.0f) {
            draw2DText("WORDS", 20, height - 60, 40, 0.3f, 1.0f, 1.0f, 0.9f);
        } else if (currentLayer == 0) {
            draw2DText("LAYER 0", 20, height - 60, 40, 0.5f, 1.0f, 0.5f, 0.9f);
        } else if (currentLayer % 2 == 1) {
            char layerText[20];
            snprintf(layerText, sizeof(layerText), "LAYER %d", currentLayer);
            draw2DText(layerText, 20, height - 60, 40, 0.3f, 0.5f, 1.0f, 0.9f);
            draw2DText("ATTENTION", 20, height - 110, 30, 0.5f, 0.7f, 1.0f, 0.8f);
        } else {
            char layerText[20];
            snprintf(layerText, sizeof(layerText), "LAYER %d", currentLayer);
            draw2DText(layerText, 20, height - 60, 40, 1.0f, 0.5f, 0.2f, 0.9f);
            draw2DText("FFN", 20, height - 110, 30, 1.0f, 0.7f, 0.4f, 0.8f);
        }

        restoreFromTextOverlay();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
