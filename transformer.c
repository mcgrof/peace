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
#include <stdlib.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define PI 3.14159265359f
#define NUM_TOKENS 5
#define NUM_LAYERS 6

// Camera state
float zoom = 8.0f;  // Start zoomed all the way in
float cameraAngle = 0.0f;
float cameraElevation = 0.5f;
float cameraY = 0.0f;  // Camera Y position
float cameraPanX = 0.0f;
float cameraPanZ = -5.0f;  // Start panned up to see content

// Mouse drag state
int isDragging = 0;
double lastMouseX = 0;
double lastMouseY = 0;

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
int currentForwardPass = 1;  // Which forward pass we're on (1-5)

// Font rendering
stbtt_fontinfo font;
unsigned char* fontBuffer = NULL;
unsigned char* fontBitmap = NULL;
int bitmapW = 512, bitmapH = 512;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    zoom += (float)yoffset * 0.2f;
    if (zoom < 0.5f) zoom = 0.5f;  // Allow much more zoom out
    if (zoom > 8.0f) zoom = 8.0f;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            isDragging = 1;
            glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        } else if (action == GLFW_RELEASE) {
            isDragging = 0;
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (isDragging) {
        double dx = xpos - lastMouseX;
        double dy = ypos - lastMouseY;

        // Pan camera based on mouse movement (inverted to feel natural)
        cameraPanX += dx * 0.01f / zoom;
        cameraPanZ += dy * 0.01f / zoom;  // Also inverted for natural feel

        lastMouseX = xpos;
        lastMouseY = ypos;
    }
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

int loadFont(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return 0;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    fontBuffer = (unsigned char*)malloc(size);
    fread(fontBuffer, 1, size, f);
    fclose(f);

    if (!stbtt_InitFont(&font, fontBuffer, 0)) {
        free(fontBuffer);
        fontBuffer = NULL;
        return 0;
    }

    fontBitmap = (unsigned char*)malloc(bitmapW * bitmapH);
    memset(fontBitmap, 0, bitmapW * bitmapH);

    return 1;
}

void drawText(const char* text, float x, float y, float size, float r, float g, float b, float a) {
    if (!fontBuffer) return;

    float scale = stbtt_ScaleForPixelHeight(&font, size);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);

    float xpos = x;
    float baseline = y + ascent * scale;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r, g, b, a);

    for (int i = 0; text[i]; i++) {
        int advance, lsb;
        int x0, y0, x1, y1;

        stbtt_GetCodepointHMetrics(&font, text[i], &advance, &lsb);
        stbtt_GetCodepointBitmapBox(&font, text[i], scale, scale, &x0, &y0, &x1, &y1);

        int w = x1 - x0;
        int h = y1 - y0;

        if (w > 0 && h > 0) {
            unsigned char* bitmap = (unsigned char*)malloc(w * h);
            stbtt_MakeCodepointBitmap(&font, bitmap, w, h, w, scale, scale, text[i]);

            // Draw bitmap as textured quads
            float bx = xpos + x0;
            float by = baseline + y0;

            glBegin(GL_QUADS);
            for (int py = 0; py < h; py++) {
                for (int px = 0; px < w; px++) {
                    unsigned char alpha_val = bitmap[py * w + px];
                    if (alpha_val > 20) {
                        float intensity = alpha_val / 255.0f;
                        glColor4f(r, g, b, a * intensity);
                        float qx = bx + px;
                        float qy = by + py;
                        glVertex2f(qx, qy);
                        glVertex2f(qx + 1, qy);
                        glVertex2f(qx + 1, qy + 1);
                        glVertex2f(qx, qy + 1);
                    }
                }
            }
            glEnd();

            free(bitmap);
        }

        xpos += advance * scale;
        if (text[i + 1]) {
            xpos += scale * stbtt_GetCodepointKernAdvance(&font, text[i], text[i + 1]);
        }
    }
}

void setupTextOverlay(int width, int height) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);  // Flip Y so 0 is at top

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
}

void restoreFromTextOverlay() {
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
}

void drawLayerPlane(int layer, float alpha) {
    float y = tokenPositions[0][layer].y;
    float size = 2.0f;

    // Alternating colors based on layer type
    float r1, g1, b1, r2, g2, b2;
    float gridR, gridG, gridB;

    if (layer == 0) {
        // Layer 0 - Embeddings (green)
        r1 = 0.2f; g1 = 0.5f; b1 = 0.3f;
        r2 = 0.3f; g2 = 0.6f; b2 = 0.4f;
        gridR = 0.4f; gridG = 0.7f; gridB = 0.5f;
    } else if (layer % 2 == 1) {
        // Attention layers (blue)
        r1 = 0.2f; g1 = 0.3f; b1 = 0.6f;
        r2 = 0.3f; g2 = 0.4f; b2 = 0.7f;
        gridR = 0.4f; gridG = 0.5f; gridB = 0.8f;
    } else {
        // FFN layers (orange)
        r1 = 0.6f; g1 = 0.4f; b1 = 0.2f;
        r2 = 0.7f; g2 = 0.5f; b2 = 0.3f;
        gridR = 0.8f; gridG = 0.6f; gridB = 0.4f;
    }

    // Draw more visible plane with color
    glBegin(GL_QUADS);
    glColor4f(r1, g1, b1, alpha * 0.25f);
    glVertex3f(-size, y, -size);
    glVertex3f(size, y, -size);
    glColor4f(r2, g2, b2, alpha * 0.2f);
    glVertex3f(size, y, size);
    glVertex3f(-size, y, size);
    glEnd();

    // Draw grid lines with matching color
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glColor4f(gridR, gridG, gridB, alpha * 0.4f);
    for (int i = -4; i <= 4; i++) {
        float pos = i * 0.5f;
        glVertex3f(pos, y, -size);
        glVertex3f(pos, y, size);
        glVertex3f(-size, y, pos);
        glVertex3f(size, y, pos);
    }
    glEnd();
    glLineWidth(2.0f);

    // Layer number will be drawn as billboard text later
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
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
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

    // Try to load a system font
    const char* fontPaths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",  // Linux
        "/System/Library/Fonts/Helvetica.ttc",              // macOS
        "C:\\Windows\\Fonts\\arial.ttf",                    // Windows
        NULL
    };

    for (int i = 0; fontPaths[i]; i++) {
        if (loadFont(fontPaths[i])) {
            printf("Loaded font: %s\n", fontPaths[i]);
            break;
        }
    }

    if (!fontBuffer) {
        printf("Warning: Could not load system font, text will not be rendered\n");
    }

    while (!glfwWindowShouldClose(window)) {
        float time = glfwGetTime();

        // Beautiful gradient background (ocean to sunset)
        float colorPhase = sinf(time * 0.1f) * 0.5f + 0.5f;
        float bgR = 0.05f + colorPhase * 0.1f;
        float bgG = 0.15f + colorPhase * 0.2f;
        float bgB = 0.3f + colorPhase * 0.1f;
        glClearColor(bgR, bgG, bgB, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Animation: progress through layers
        // Pause at start to show the words
        if (time < 3.0f) {
            animationPhase = 0.0f;  // Hold at layer 0 for first 3 seconds
        } else {
            animationPhase += 0.02f;  // Slower
        }

        // Simple autoregressive: cycle through forward passes
        // Each forward pass goes through all 6 layers, then moves to next pass
        float LAYER_TIME = 1.0f;  // 1 second per layer
        float PASS_TIME = NUM_LAYERS * LAYER_TIME;  // Time per forward pass = 6 seconds
        float TOTAL_TIME = NUM_TOKENS * PASS_TIME;  // Total animation cycle = 30 seconds

        float cyclePhase = fmodf(animationPhase, TOTAL_TIME);

        // Determine current forward pass (1-5)
        currentForwardPass = ((int)(cyclePhase / PASS_TIME)) + 1;
        if (currentForwardPass > NUM_TOKENS) currentForwardPass = NUM_TOKENS;

        // Within current pass, which layer (0-5)
        float passLocalTime = fmodf(cyclePhase, PASS_TIME);
        currentLayer = (int)(passLocalTime / LAYER_TIME);
        if (currentLayer >= NUM_LAYERS) currentLayer = NUM_LAYERS - 1;

        // Blend between layers
        float layerBlend = fmodf(passLocalTime, LAYER_TIME) / LAYER_TIME;

        // Camera setup - moves up as layers progress
        glLoadIdentity();

        // Camera follows the action - starts at bottom (words) and moves up through layers
        float targetY = -2.5f + (currentLayer + layerBlend) * 0.4f;  // Follow the layers up
        cameraY += (targetY - cameraY) * 0.05f;  // Smooth following

        // Position camera to see current layer
        glTranslatef(cameraPanX, cameraY, -10.0f / zoom);  // Pull back more, apply pan
        glRotatef(30.0f, 1, 0, 0);  // Tilt down more to see action
        glRotatef(0.0f, 0, 1, 0);  // No rotation - keep it stable
        glTranslatef(0, 0, cameraPanZ);  // Apply Z pan after rotation

        // Draw LINEAR transformation vectors for attention layers
        if (currentLayer > 0 && currentLayer % 2 == 1 && layerBlend > 0.2f) {
            // Attention layer - show LINEAR transformation with bold colored vectors
            float vectorAlpha = layerBlend * 0.8f;

            // Only show tokens in current forward pass
            for (int i = 0; i < currentForwardPass; i++) {
                Vec3 from = tokenPositions[i][currentLayer - 1];
                Vec3 to = tokenPositions[i][currentLayer];

                // Interpolate during animation
                if (layerBlend < 1.0f) {
                    to.x = from.x + layerBlend * (to.x - from.x);
                    to.y = from.y + layerBlend * (to.y - from.y);
                    to.z = from.z + layerBlend * (to.z - from.z);
                }

                // Draw BOLD linear transformation vector with gradient
                glLineWidth(6.0f);
                glBegin(GL_LINES);
                glColor4f(tokens[i].r, tokens[i].g, tokens[i].b, vectorAlpha * 0.3f);
                glVertex3f(from.x, from.y, from.z);
                glColor4f(tokens[i].r * 1.3f, tokens[i].g * 1.3f, tokens[i].b * 1.3f, vectorAlpha);
                glVertex3f(to.x, to.y, to.z);
                glEnd();

                // Draw arrowhead at destination
                Vec3 dir = {to.x - from.x, to.y - from.y, to.z - from.z};
                float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
                if (len > 0.01f) {
                    dir.x /= len; dir.y /= len; dir.z /= len;
                    float arrowSize = 0.1f;

                    glBegin(GL_TRIANGLES);
                    glColor4f(tokens[i].r * 1.3f, tokens[i].g * 1.3f, tokens[i].b * 1.3f, vectorAlpha);
                    glVertex3f(to.x, to.y, to.z);
                    glVertex3f(to.x - dir.x * arrowSize - dir.y * arrowSize * 0.5f,
                              to.y - dir.y * arrowSize + dir.x * arrowSize * 0.5f, to.z);
                    glVertex3f(to.x - dir.x * arrowSize + dir.y * arrowSize * 0.5f,
                              to.y - dir.y * arrowSize - dir.x * arrowSize * 0.5f, to.z);
                    glEnd();
                }
            }

            // Also show cross-token attention links (thinner)
            float linkAlpha = layerBlend * 0.2f;
            glLineWidth(2.0f);
            for (int i = 0; i < currentForwardPass; i++) {
                Vec3 from = tokenPositions[i][currentLayer];
                for (int j = i + 1; j < currentForwardPass; j++) {
                    Vec3 to = tokenPositions[j][currentLayer];
                    float pulse = sinf(time * 3.0f + i + j) * 0.3f + 0.7f;

                    glBegin(GL_LINES);
                    glColor4f(0.4f, 0.6f, 1.0f, linkAlpha * pulse);
                    glVertex3f(from.x, from.y, from.z);
                    glVertex3f(to.x, to.y, to.z);
                    glEnd();
                }
            }

            glLineWidth(2.0f);
        }

        // Draw NON-LINEAR FFN transformation - curved wavy paths showing activation function
        if (currentLayer > 0 && currentLayer % 2 == 0 && layerBlend > 0.2f) {
            // FFN layer - show NON-LINEAR transformation with dramatic curved particle trails
            float transformAlpha = layerBlend * 0.7f;

            for (int i = 0; i < currentForwardPass; i++) {
                Vec3 from = tokenPositions[i][currentLayer - 1];
                Vec3 to = tokenPositions[i][currentLayer];

                // Draw multiple curved particle trails showing non-linearity
                int numTrails = 8;
                for (int trail = 0; trail < numTrails; trail++) {
                    float trailAngle = (2.0f * PI * trail / numTrails) + time * 1.5f + i;
                    float trailRadius = 0.15f;

                    glLineWidth(3.0f);
                    glBegin(GL_LINE_STRIP);

                    // Draw curved path from old position to new position
                    int steps = 15;
                    for (int step = 0; step <= steps; step++) {
                        float t = (float)step / steps;
                        float smoothT = t * t * (3.0f - 2.0f * t); // Smooth step

                        // Interpolate position
                        float x = from.x + smoothT * (to.x - from.x);
                        float y = from.y + smoothT * (to.y - from.y);
                        float z = from.z + smoothT * (to.z - from.z);

                        // Add NON-LINEAR wave distortion (activation function visualization)
                        float wave = sinf(t * PI * 3.0f + trailAngle) * trailRadius;
                        wave *= (1.0f - t); // Decay as we approach destination

                        x += cosf(trailAngle) * wave;
                        z += sinf(trailAngle) * wave;

                        // Color gradient with pulsing
                        float intensity = 1.0f - t * 0.5f;
                        float pulse = sinf(time * 4.0f + trail + step * 0.2f) * 0.3f + 0.7f;
                        glColor4f(1.0f * intensity, 0.6f * intensity, 0.2f * intensity,
                                 transformAlpha * pulse * (1.0f - t * 0.5f));
                        glVertex3f(x, y, z);
                    }
                    glEnd();
                }

                // Draw spiraling "energy" around the token at new position
                Vec3 pos = tokenPositions[i][currentLayer];
                glLineWidth(2.0f);
                glBegin(GL_LINE_STRIP);
                int spiralSteps = 20;
                for (int s = 0; s < spiralSteps; s++) {
                    float t = (float)s / spiralSteps;
                    float spiralAngle = t * PI * 4.0f + time * 3.0f + i;
                    float spiralRadius = 0.2f * (1.0f - t);

                    float x = pos.x + cosf(spiralAngle) * spiralRadius;
                    float y = pos.y + t * 0.15f - 0.075f;
                    float z = pos.z + sinf(spiralAngle) * spiralRadius;

                    float intensity = 1.0f - t;
                    glColor4f(1.0f * intensity, 0.5f * intensity, 0.2f * intensity, transformAlpha * intensity);
                    glVertex3f(x, y, z);
                }
                glEnd();
            }

            glLineWidth(2.0f);
        }

        // Draw token words below layer 0 using TrueType font
        float wordY = tokenPositions[0][0].y - 1.5f;  // Much further below layer 0

        if (fontBuffer) {
            // Switch to 2D to draw text billboards
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            // Draw ALL words at bottom (showing full sequence)
            for (int i = 0; i < NUM_TOKENS; i++) {
                Vec3 wordPos = tokenPositions[i][0];
                wordPos.y = wordY;

                // Project 3D position to screen space
                float modelview[16], projection[16];

                glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
                glGetFloatv(GL_PROJECTION_MATRIX, projection);

                // Manual projection (simplified)
                float x = wordPos.x;
                float y = wordPos.y;
                float z = wordPos.z;

                // Transform by modelview
                float mx = modelview[0]*x + modelview[4]*y + modelview[8]*z + modelview[12];
                float my = modelview[1]*x + modelview[5]*y + modelview[9]*z + modelview[13];
                float mz = modelview[2]*x + modelview[6]*y + modelview[10]*z + modelview[14];
                float mw = modelview[3]*x + modelview[7]*y + modelview[11]*z + modelview[15];

                // Transform by projection
                float px = projection[0]*mx + projection[4]*my + projection[8]*mz + projection[12]*mw;
                float py = projection[1]*mx + projection[5]*my + projection[9]*mz + projection[13]*mw;
                float pw = projection[3]*mx + projection[7]*my + projection[11]*mz + projection[15]*mw;

                if (pw != 0) {
                    px /= pw;
                    py /= pw;

                    // Convert to screen coordinates
                    float screenX = (px + 1.0f) * width / 2.0f;
                    float screenY = (1.0f - py) * height / 2.0f;

                    // Draw text at screen position
                    setupTextOverlay(width, height);

                    // Calculate text width to center it
                    float scale = stbtt_ScaleForPixelHeight(&font, 48);
                    float textWidth = 0;
                    for (int j = 0; tokens[i].label[j]; j++) {
                        int advance, lsb;
                        stbtt_GetCodepointHMetrics(&font, tokens[i].label[j], &advance, &lsb);
                        textWidth += advance * scale;
                    }

                    // Highlight tokens in current forward pass, dim future tokens
                    float brightness, alpha;
                    if (i < currentForwardPass) {
                        // Tokens we HAVE (inputs to this forward pass)
                        brightness = 1.2f;
                        alpha = 1.0f;
                    } else if (i == currentForwardPass && currentForwardPass < NUM_TOKENS) {
                        // Token being PREDICTED (not yet generated - show dimmer with pulse)
                        float pulse = sinf(time * 3.0f) * 0.2f + 0.5f;
                        brightness = 0.6f * pulse;
                        alpha = 0.5f;
                    } else {
                        // Not yet generated
                        brightness = 0.3f;
                        alpha = 0.3f;
                    }

                    drawText(tokens[i].label, screenX - textWidth/2, screenY, 48,
                            tokens[i].r * brightness, tokens[i].g * brightness, tokens[i].b * brightness, alpha);

                    restoreFromTextOverlay();
                }
            }
        } else {
            // Fallback to old block letters if no font loaded
            for (int i = 0; i < NUM_TOKENS; i++) {
                Vec3 wordPos = tokenPositions[i][0];
                wordPos.y = wordY;

                glColor4f(tokens[i].r * 1.5f, tokens[i].g * 1.5f, tokens[i].b * 1.5f, 1.0f);
                float wordSize = 0.6f;

                int letterCount = 0;
                for (int j = 0; tokens[i].label[j] != '\0'; j++) letterCount++;
                float wordWidth = wordSize * 0.7f * letterCount;

                drawWord(tokens[i].label, wordPos.x - wordWidth / 2, wordPos.y, wordPos.z, wordSize);
            }
        }

        // Draw the embedding vectors
        for (int i = 0; i < NUM_TOKENS; i++) {
            Vec3 wordPos = tokenPositions[i][0];
            wordPos.y = wordY;
            Vec3 embeddingPos = tokenPositions[i][0];

            glLineWidth(3.0f);
            glBegin(GL_LINES);
            glColor4f(tokens[i].r * 0.8f, tokens[i].g * 0.8f, tokens[i].b * 0.8f, 0.7f);
            glVertex3f(wordPos.x, wordPos.y + 0.3f, wordPos.z);  // Top of word area
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
        }
        glLineWidth(2.0f);

        // Draw layer planes
        for (int layer = 0; layer <= currentLayer; layer++) {
            float alpha = (layer == currentLayer) ? layerBlend : 1.0f;
            drawLayerPlane(layer, alpha);
        }

        // Draw layer numbers as billboards
        if (fontBuffer) {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            for (int layer = 0; layer <= currentLayer; layer++) {
                float y = tokenPositions[0][layer].y;
                float size = 2.0f;
                Vec3 labelPos = {-size + 0.3f, y + 0.1f, size - 0.3f};

                // Project to screen space
                float modelview[16], projection[16];
                glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
                glGetFloatv(GL_PROJECTION_MATRIX, projection);

                float x = labelPos.x;
                float ly = labelPos.y;
                float z = labelPos.z;

                // Transform by modelview
                float mx = modelview[0]*x + modelview[4]*ly + modelview[8]*z + modelview[12];
                float my = modelview[1]*x + modelview[5]*ly + modelview[9]*z + modelview[13];
                float mz = modelview[2]*x + modelview[6]*ly + modelview[10]*z + modelview[14];
                float mw = modelview[3]*x + modelview[7]*ly + modelview[11]*z + modelview[15];

                // Transform by projection
                float px = projection[0]*mx + projection[4]*my + projection[8]*mz + projection[12]*mw;
                float py = projection[1]*mx + projection[5]*my + projection[9]*mz + projection[13]*mw;
                float pw = projection[3]*mx + projection[7]*my + projection[11]*mz + projection[15]*mw;

                if (pw != 0) {
                    px /= pw;
                    py /= pw;

                    // Convert to screen coordinates
                    float screenX = (px + 1.0f) * width / 2.0f;
                    float screenY = (1.0f - py) * height / 2.0f;

                    // Draw layer number
                    setupTextOverlay(width, height);

                    char layerNum[8];
                    snprintf(layerNum, sizeof(layerNum), "L%d", layer);

                    float alpha = (layer == currentLayer) ? layerBlend : 1.0f;
                    drawText(layerNum, screenX, screenY, 32, 1.0f, 1.0f, 1.0f, alpha * 0.8f);

                    restoreFromTextOverlay();
                }
            }
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

        // Draw token orbs - only tokens in current forward pass
        for (int i = 0; i < currentForwardPass; i++) {
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

            // Orbs grow SLIGHTLY larger through layers (more refined representations)
            float layerProgress = (float)currentLayer / (float)(NUM_LAYERS - 1);
            float baseSize = 0.04f + layerProgress * 0.04f;  // Grow from 0.04 to 0.08 (half size)

            // Subtle pulsing glow effect
            float pulse = sinf(time * 2.0f + i) * 0.5f + 0.5f;
            float glowRadius = baseSize * 1.8f + pulse * 0.01f;

            // Outer glow (subtle)
            float glowIntensity = 0.15f + layerProgress * 0.15f;
            drawSphere(pos.x, pos.y, pos.z, glowRadius,
                      tokens[i].r, tokens[i].g, tokens[i].b, glowIntensity);

            // Core orb - brighter at higher layers
            float coreIntensity = 0.8f + layerProgress * 0.2f;
            drawSphere(pos.x, pos.y, pos.z, baseSize,
                      tokens[i].r * coreIntensity, tokens[i].g * coreIntensity, tokens[i].b * coreIntensity, 0.95f);
        }

        // Re-enable depth writes
        glDepthMask(GL_TRUE);

        // Draw HUD text overlay with proper font
        if (fontBuffer) {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            setupTextOverlay(width, height);

            // Title
            drawText("AUTOREGRESSIVE TRANSFORMER", 20, 30, 32, 1.0f, 1.0f, 1.0f, 0.9f);

            // BIG DEBUG: Show current pass number
            char bigNum[16];
            snprintf(bigNum, sizeof(bigNum), "PASS: %d", currentForwardPass);
            drawText(bigNum, width - 250, 30, 48, 1.0f, 0.0f, 0.0f, 1.0f);

            // Show which forward pass we're on with DEBUG info
            char passInfo[256];
            if (currentForwardPass < NUM_TOKENS) {
                // Build the input sequence string
                char inputSeq[128] = "";
                for (int i = 0; i < currentForwardPass; i++) {
                    if (i > 0) strcat(inputSeq, " ");
                    strcat(inputSeq, tokens[i].label);
                }
                snprintf(passInfo, sizeof(passInfo), "Forward Pass %d: [%s] -> Predicting: %s (phase: %.1f)",
                        currentForwardPass, inputSeq, tokens[currentForwardPass].label, animationPhase);
                drawText(passInfo, 20, 70, 20, 1.0f, 1.0f, 0.4f, 0.9f);
                drawText("(Bright tokens process in parallel through layers)", 20, 100, 16, 0.7f, 0.7f, 0.7f, 0.8f);
            } else {
                snprintf(passInfo, sizeof(passInfo), "Forward Pass 5: Complete! (phase: %.1f)", animationPhase);
                drawText(passInfo, 20, 70, 20, 0.5f, 1.0f, 0.5f, 0.9f);
            }

            char layerInfo[128];

            // Current layer info
            if (time < 3.0f) {
                drawText("WORDS -> EMBEDDINGS", 20, 130, 22, 0.4f, 1.0f, 1.0f, 0.9f);
                drawText("Watch multiple forward passes, each with more tokens", 20, 160, 16, 0.8f, 0.8f, 0.8f, 0.8f);
            } else if (currentLayer == 0) {
                drawText("LAYER 0: Embeddings", 20, 130, 22, 0.5f, 1.0f, 0.5f, 0.9f);
                drawText("Converting words to vectors", 20, 160, 16, 0.8f, 0.8f, 0.8f, 0.8f);
            } else if (currentLayer % 2 == 1) {
                snprintf(layerInfo, sizeof(layerInfo), "LAYER %d: Attention (LINEAR)", currentLayer);
                drawText(layerInfo, 20, 130, 22, 0.4f, 0.6f, 1.0f, 0.9f);
                drawText("Straight colored arrows = linear transformation", 20, 160, 16, 0.8f, 0.8f, 0.8f, 0.8f);
            } else {
                snprintf(layerInfo, sizeof(layerInfo), "LAYER %d: FFN (NON-LINEAR)", currentLayer);
                drawText(layerInfo, 20, 130, 22, 1.0f, 0.6f, 0.3f, 0.9f);
                drawText("Curved wavy trails = activation function (non-linear)", 20, 160, 16, 0.8f, 0.8f, 0.8f, 0.8f);
            }

            // Instructions
            drawText("Scroll to zoom | Drag to pan", 20, height - 30, 16, 0.7f, 0.7f, 0.7f, 0.7f);

            restoreFromTextOverlay();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
