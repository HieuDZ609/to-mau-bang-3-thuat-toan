#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>

static const int GRID_W = 44;
static const int GRID_H = 32;
static const int CELL   = 28;
static const int WIN_W  = GRID_W * CELL;
static const int WIN_H  = GRID_H * CELL;

enum { C_EMPTY = 0, C_BOUND, C_FILLED, C_FRONTIER, C_CURRENT };

struct IPt { int x, y; };

static int cell[GRID_H][GRID_W];
static std::vector<IPt> verts;
static std::vector<IPt> floodStack;
static bool polygonClosed = false;
static bool fillDone = false;
static bool hasSeed = false;
static int mouseX = 0, mouseY = 0;
static int speed = 1;
static int connectivity = 4;
static bool autoRun = false;
static int filledCount = 0;

static unsigned int shaderProgram, vao, vbo;

static const char* vsSrc =
    "#version 330 core\n"
    "layout(location=0) in vec2 aPos;\n"
    "layout(location=1) in vec3 aCol;\n"
    "out vec3 vCol;\n"
    "void main(){ vCol=aCol; gl_Position=vec4(aPos,0,1); }\n";
static const char* fsSrc =
    "#version 330 core\n"
    "in vec3 vCol;\n"
    "out vec4 FragColor;\n"
    "void main(){ FragColor=vec4(vCol,1); }\n";

template <typename F>
static void bresLine(int x0, int y0, int x1, int y1, F plot) {
    int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        plot(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

static void clearCanvas() {
    std::memset(cell, 0, sizeof(cell));
    verts.clear();
    floodStack.clear();
    polygonClosed = false;
    fillDone = false;
    hasSeed = false;
    filledCount = 0;
    autoRun = false;
    std::printf("[C] Da xoa toan bo man hinh.\n");
}

static void clearFilledArea() {
    for (int y = 0; y < GRID_H; y++)
        for (int x = 0; x < GRID_W; x++)
            if (cell[y][x] != C_BOUND) cell[y][x] = C_EMPTY;
    floodStack.clear();
    fillDone = false;
    hasSeed = false;
    filledCount = 0;
    autoRun = false;
    std::printf("[X] Da xoa vung to, giu nguyen duong bien.\n");
}

static void rasterizePolygon() {
    int n = (int)verts.size();
    for (int i = 0; i < n; i++) {
        IPt a = verts[i];
        IPt b = verts[(i + 1) % n];
        bresLine(a.x, a.y, b.x, b.y, [](int x, int y) {
            if (x >= 0 && x < GRID_W && y >= 0 && y < GRID_H) cell[y][x] = C_BOUND;
        });
    }
}

static void closePolygon() {
    if ((int)verts.size() < 3) {
        std::printf("[Loi] Can it nhat 3 dinh de dong da giac (hien co %d).\n", (int)verts.size());
        return;
    }
    rasterizePolygon();
    polygonClosed = true;
    std::printf("[ENTER] Da dong da giac %d dinh, duong bien da duoc raster hoa.\n", (int)verts.size());
    std::printf("       Bay gio click chuot trai vao mot diem de dat seed point.\n");
}

static void pushSeed(int px, int py) {
    if (px < 0 || px >= GRID_W || py < 0 || py >= GRID_H) return;
    if (cell[py][px] != C_EMPTY) {
        std::printf("[Loi] Diem (%d,%d) khong trong (da to hoac nam tren duong bien).\n", px, py);
        return;
    }
    floodStack.clear();
    cell[py][px] = C_FRONTIER;
    floodStack.push_back({px, py});
    hasSeed = true;
    fillDone = false;
    autoRun = false;
    filledCount = 0;
    std::printf("[SEED] (%d,%d) | 4/8-connected: %d | Toc do: %d px/buc\n", px, py, connectivity, speed);
    std::printf("[SPACE] 1 buoc to | [T] chay tu dong | [G] to het ngay\n");
}

static void floodStep() {
    for (int y = 0; y < GRID_H; y++)
        for (int x = 0; x < GRID_W; x++)
            if (cell[y][x] == C_CURRENT) cell[y][x] = C_FILLED;

    static const int dx4[4] = { 1, -1, 0, 0 }, dy4[4] = { 0, 0, 1, -1 };
    static const int dx8[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
    static const int dy8[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };

    int budget = speed;
    while (budget-- > 0 && !floodStack.empty()) {
        IPt p = floodStack.back();
        floodStack.pop_back();
        if (p.x < 0 || p.x >= GRID_W || p.y < 0 || p.y >= GRID_H) continue;
        if (cell[p.y][p.x] != C_FRONTIER) continue;

        cell[p.y][p.x] = C_CURRENT;
        filledCount++;

        const int* DX = (connectivity == 8) ? dx8 : dx4;
        const int* DY = (connectivity == 8) ? dy8 : dy4;
        int cnt = (connectivity == 8) ? 8 : 4;
        for (int k = 0; k < cnt; k++) {
            int nx = p.x + DX[k], ny = p.y + DY[k];
            if (nx < 0 || nx >= GRID_W || ny < 0 || ny >= GRID_H) continue;
            if (cell[ny][nx] == C_EMPTY) {
                cell[ny][nx] = C_FRONTIER;
                floodStack.push_back({nx, ny});
            }
        }
    }

    if (floodStack.empty() && !fillDone && hasSeed) {
        for (int y = 0; y < GRID_H; y++)
            for (int x = 0; x < GRID_W; x++)
                if (cell[y][x] == C_FRONTIER || cell[y][x] == C_CURRENT) cell[y][x] = C_FILLED;
        fillDone = true;
        autoRun = false;
        std::printf("[Xong] To xong %d pixel trong che do %d-connected.\n", filledCount, connectivity);
    }
}

static void floodAll() {
    if (!hasSeed) { std::printf("[Loi] Chua co seed point.\n"); return; }
    while (!floodStack.empty()) floodStep();
    std::printf("[G] Da to toan bo %d pixel.\n", filledCount);
}

static void addQuad(std::vector<float>& v, float x0, float y0, float x1, float y1,
                    float r, float g, float b) {
    v.insert(v.end(), { x0, y0, r, g, b });
    v.insert(v.end(), { x1, y0, r, g, b });
    v.insert(v.end(), { x1, y1, r, g, b });
    v.insert(v.end(), { x0, y0, r, g, b });
    v.insert(v.end(), { x1, y1, r, g, b });
    v.insert(v.end(), { x0, y1, r, g, b });
}

static void addCell(std::vector<float>& v, int gx, int gy, float r, float g, float b) {
    float cw = 2.0f / GRID_W, ch = 2.0f / GRID_H;
    float ix = cw * 0.06f, iy = ch * 0.06f;
    float x0 = -1.0f + gx * cw + ix, x1 = x0 + cw - 2 * ix;
    float y0 = 1.0f - gy * ch + iy,  y1 = y0 - ch + 2 * iy;
    addQuad(v, x0, y0, x1, y1, r, g, b);
}

static void buildGeometry() {
    std::vector<float> v;
    v.reserve(60000);

    float cw = 2.0f / GRID_W, ch = 2.0f / GRID_H;
    for (int gx = 0; gx <= GRID_W; gx++) {
        bool major = (gx % 5 == 0);
        float cx = -1.0f + gx * cw;
        float half = cw * (major ? 0.055f : 0.028f);
        addQuad(v, cx - half, 1.0f, cx + half, 1.0f - GRID_H * ch,
                major ? 0.80f : 0.90f, major ? 0.82f : 0.92f, major ? 0.86f : 0.95f);
    }
    for (int gy = 0; gy <= GRID_H; gy++) {
        bool major = (gy % 5 == 0);
        float cy = 1.0f - gy * ch;
        float half = ch * (major ? 0.055f : 0.028f);
        addQuad(v, -1.0f, cy + half, -1.0f + GRID_W * cw, cy - half,
                major ? 0.80f : 0.90f, major ? 0.82f : 0.92f, major ? 0.86f : 0.95f);
    }

    for (int gy = 0; gy < GRID_H; gy++) {
        for (int gx = 0; gx < GRID_W; gx++) {
            int s = cell[gy][gx];
            if (s == C_EMPTY) continue;
            if (s == C_BOUND)        addCell(v, gx, gy, 0.894f, 0.341f, 0.180f);
            else if (s == C_FILLED)  addCell(v, gx, gy, 0.165f, 0.616f, 0.561f);
            else if (s == C_FRONTIER)addCell(v, gx, gy, 0.914f, 0.769f, 0.416f);
            else                     addCell(v, gx, gy, 0.957f, 0.635f, 0.380f);
        }
    }

    if (!polygonClosed && (int)verts.size() > 0) {
        IPt a = verts.back();
        bresLine(a.x, a.y, mouseX, mouseY, [&v](int x, int y) {
            if (x >= 0 && x < GRID_W && y >= 0 && y < GRID_H && cell[y][x] == C_EMPTY)
                addCell(v, x, y, 0.70f, 0.72f, 0.78f);
        });
    }

    for (size_t i = 0; i < verts.size(); i++) {
        IPt a = verts[i];
        if (a.x >= 0 && a.x < GRID_W && a.y >= 0 && a.y < GRID_H)
            addCell(v, a.x, a.y, 0.20f, 0.24f, 0.55f);
    }

    if (polygonClosed && !hasSeed) {
        float x0 = -1.0f + mouseX * cw + cw * 0.35f, x1 = -1.0f + (mouseX + 1) * cw - cw * 0.35f;
        float y0 = 1.0f - mouseY * ch - ch * 0.15f, y1 = 1.0f - (mouseY + 1) * ch + ch * 0.15f;
        addQuad(v, x0, y0, x1, y1, 0.15f, 0.18f, 0.45f);
        bresLine(mouseX, mouseY, mouseX, mouseY, [&v](int x, int y) {
            if (x >= 0 && x < GRID_W && y >= 0 && y < GRID_H) addCell(v, x, y, 0.15f, 0.18f, 0.45f);
        });
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, v.size() * sizeof(float), v.data());
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, (int)(v.size() / 5));
}

static void mouseToGrid(GLFWwindow* w, int& gx, int& gy) {
    double mx, my;
    glfwGetCursorPos(w, &mx, &my);
    gx = (int)std::floor(mx / WIN_W * GRID_W);
    gy = (int)std::floor(my / WIN_H * GRID_H);
    gx = std::max(0, std::min(GRID_W - 1, gx));
    gy = std::max(0, std::min(GRID_H - 1, gy));
}

static void mouse_button_callback(GLFWwindow* w, int button, int action, int mods) {
    (void)mods;
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;
    int gx, gy;
    mouseToGrid(w, gx, gy);
    if (!polygonClosed) {
        verts.push_back({ gx, gy });
        std::printf("[Click] Dinh %d: (%d, %d)\n", (int)verts.size(), gx, gy);
    } else {
        pushSeed(gx, gy);
    }
}

static void cursor_callback(GLFWwindow* w, double mx, double my) {
    (void)w;
    mouseX = (int)std::floor(mx / WIN_W * GRID_W);
    mouseY = (int)std::floor(my / WIN_H * GRID_H);
}

static void key_callback(GLFWwindow* w, int key, int sc, int action, int mods) {
    (void)sc; (void)mods;
    if (action != GLFW_PRESS) return;

    switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(w, true); return;
        case GLFW_KEY_ENTER:
        case GLFW_KEY_KP_ENTER:
            closePolygon();
            return;
        case GLFW_KEY_SPACE:
            if (hasSeed) { floodStep(); std::printf("[Buoc] da to %d px | stack=%d\n", filledCount, (int)floodStack.size()); }
            else std::printf("[SPACE] Hay dat seed point truoc.\n");
            return;
        case GLFW_KEY_T:
            if (!hasSeed) { std::printf("[T] Chua co seed point.\n"); return; }
            if (fillDone) { std::printf("[T] Da to xong, dat seed moi truoc.\n"); return; }
            autoRun = !autoRun;
            std::printf("[T] Chay tu dong: %s (%d px/buc)\n", autoRun ? "BAT" : "TAT", speed);
            return;
        case GLFW_KEY_G:
            floodAll();
            return;
        case GLFW_KEY_B:
            connectivity = (connectivity == 4) ? 8 : 4;
            if (hasSeed) { clearFilledArea(); }
            std::printf("[B] Che do lien ket %d-connected.\n", connectivity);
            return;
        case GLFW_KEY_LEFT_BRACKET:
            speed = std::max(1, speed / 2);
            std::printf("[Toc do] %d px/buc\n", speed);
            return;
        case GLFW_KEY_RIGHT_BRACKET:
            speed = std::min(1024, speed * 2);
            std::printf("[Toc do] %d px/buc\n", speed);
            return;
        case GLFW_KEY_X:
            clearFilledArea();
            return;
        case GLFW_KEY_R:
            clearFilledArea();
            std::printf("[R] Reset thuat toan.\n");
            return;
        case GLFW_KEY_C:
            clearCanvas();
            return;
        case GLFW_KEY_BACKSPACE:
            if (!polygonClosed && !verts.empty()) {
                verts.pop_back();
                std::printf("[Backspace] Xoa dinh cuoi, con %d dinh.\n", (int)verts.size());
            }
            return;
        default:
            return;
    }
}

static void framebuffer_size_callback(GLFWwindow* w, int w2, int h) { (void)w; glViewport(0, 0, w2, h); }

static unsigned int compileShader(unsigned int t, const char* s) {
    unsigned int id = glCreateShader(t);
    glShaderSource(id, 1, &s, nullptr);
    glCompileShader(id);
    int ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(id, 1024, nullptr, log);
        std::printf("[Loi shader] %s\n", log);
    }
    return id;
}

static void initGL() {
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 60000 * 5 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H,
        "TO THEO DUONG BIEN - Boundary Fill (8/4 connected, co animation)", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { glfwTerminate(); return -1; }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_callback);
    glfwSetKeyCallback(window, key_callback);

    initGL();
    std::memset(cell, 0, sizeof(cell));

    std::printf("=========================================================\n");
    std::printf("   TO THEO DUONG BIEN (Boundary Fill)\n");
    std::printf("   Luu do: vien do = do cam  |  da to = xanh lam\n");
    std::printf("   frontier (trong stack) = vang  |  dang xu ly = cam\n");
    std::printf("---------------------------------------------------------\n");
    std::printf("[Click trai] them dinh da giac / dat seed point\n");
    std::printf("[ENTER] dong da giac (it nhat 3 dinh)\n");
    std::printf("[SPACE] to 1 buoc | [T] chay tu dong | [G] to het ngay\n");
    std::printf("[B] doi 4/8-connected | [ ] tang/giam toc do\n");
    std::printf("[X]/[R] xoa vung to | [C] xoa het | [BACKSPACE] bo dinh\n");
    std::printf("[ESC] thoat\n");
    std::printf("=========================================================\n");

    while (!glfwWindowShouldClose(window)) {
        if (autoRun && hasSeed && !fillDone) floodStep();
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        buildGeometry();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
