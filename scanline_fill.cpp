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

struct IPt { int x, y; };

struct Edge {
    double x;
    int    yMin;
    int    yMax;
    double dxdy;
};

static std::vector<IPt>  verts;
static std::vector<Edge> allEdges;
static std::vector<Edge> aet;
static std::vector<int>  lastCross;
static bool outline[GRID_H][GRID_W];
static bool filled[GRID_H][GRID_W];

static bool polygonClosed = false;
static bool finished = false;
static bool autoRun = false;
static int  curY = -1;
static int  lastY = -1;
static int  yStart = 0, yEnd = 0;
static int  speed = 1;
static int  mouseX = 0, mouseY = 0;
static int  filledCount = 0;

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

static void clearAll() {
    std::memset(outline, 0, sizeof(outline));
    std::memset(filled, 0, sizeof(filled));
    verts.clear();
    allEdges.clear();
    aet.clear();
    lastCross.clear();
    polygonClosed = false;
    finished = false;
    autoRun = false;
    curY = -1;
    filledCount = 0;
    std::printf("[C] Da xoa toan bo.\n");
}

static void resetScan() {
    std::memset(filled, 0, sizeof(filled));
    aet.clear();
    lastCross.clear();
    curY = yStart - 1;
    lastY = -1;
    finished = false;
    autoRun = false;
    filledCount = 0;
}

static void closePolygon() {
    if ((int)verts.size() < 3) {
        std::printf("[Loi] Can it nhat 3 dinh (hien co %d).\n", (int)verts.size());
        return;
    }
    allEdges.clear();
    int n = (int)verts.size();
    yStart = GRID_H; yEnd = -1;
    for (int i = 0; i < n; i++) {
        IPt p = verts[i];
        IPt q = verts[(i + 1) % n];
        bresLine(p.x, p.y, q.x, q.y, [](int x, int y) {
            if (x >= 0 && x < GRID_W && y >= 0 && y < GRID_H) outline[y][x] = true;
        });
        if (p.y == q.y) continue;
        int xa = p.x, ya = p.y, xb = q.x, yb = q.y;
        if (ya > yb) { std::swap(ya, yb); std::swap(xa, xb); }
        Edge e;
        e.x = xa;
        e.yMin = ya;
        e.yMax = yb;
        e.dxdy = (double)(xb - xa) / (double)(yb - ya);
        allEdges.push_back(e);
        yStart = std::min(yStart, ya);
        yEnd = std::max(yEnd, yb);
    }
    polygonClosed = true;
    curY = yStart - 1;
    lastY = -1;
    finished = false;
    std::printf("[ENTER] Da dong da giac %d canh, y tu %d den %d (%d dong quet).\n",
                (int)allEdges.size(), yStart, yEnd, yEnd - yStart + 1);
    std::printf("[SPACE]quet 1 dong | [T] chay tu dong | [G]quet het ngay\n");
}

static void scanStep() {
    if (!polygonClosed) { std::printf("[SPACE] Hay dong da giac truoc (ENTER).\n"); return; }
    if (finished) { std::printf("[SPACE] Da quet xit het.\n"); return; }

    int budget = speed;
    while (budget-- > 0 && !finished) {
        if (curY < yStart) curY = yStart;
        if (curY > yEnd) { finished = true; break; }

        std::vector<Edge> keep;
        for (size_t i = 0; i < aet.size(); i++) {
            if (aet[i].yMax > curY) keep.push_back(aet[i]);
        }
        aet.swap(keep);

        for (size_t i = 0; i < allEdges.size(); i++) {
            if (allEdges[i].yMin != curY) continue;
            Edge e = allEdges[i];
            e.x = e.x + e.dxdy * (double)(curY - e.yMin);
            aet.push_back(e);
        }

        std::sort(aet.begin(), aet.end(), [](const Edge& a, const Edge& b) { return a.x < b.x; });

        lastCross.clear();
        for (size_t i = 0; i < aet.size(); i++) {
            int xi = (int)std::floor(aet[i].x + 0.5);
            lastCross.push_back(xi);
        }

        int pairs = 0;
        for (size_t i = 0; i + 1 < aet.size(); i += 2) {
            int x0 = (int)std::floor(aet[i].x + 0.5);
            int x1 = (int)std::floor(aet[i + 1].x + 0.5);
            if (x1 < x0) std::swap(x0, x1);
            for (int x = x0; x <= x1; x++) {
                if (x >= 0 && x < GRID_W && curY >= 0 && curY < GRID_H) {
                    if (!filled[curY][x]) { filled[curY][x] = true; filledCount++; }
                }
            }
            pairs++;
        }

        std::printf("[y=%2d] AET=%d | giao:", curY, (int)aet.size());
        for (size_t i = 0; i < aet.size(); i++) std::printf(" %.1f", aet[i].x);
        std::printf(" | cap doi=%d | da to %d px\n", pairs, filledCount);

        for (size_t i = 0; i < aet.size(); i++) aet[i].x += aet[i].dxdy;
        lastY = curY;
        curY++;
        if (curY > yEnd) finished = true;
    }
}

static void scanAll() {
    if (!polygonClosed) { std::printf("[G] Hay dong da giac truoc (ENTER).\n"); return; }
    while (!finished) scanStep();
    std::printf("[G] Quet xong %d dong, to tong %d pixel.\n", yEnd - yStart + 1, filledCount);
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

static void spanColor(int y, float& r, float& g, float& b) {
    float t = 0.0f;
    if (yEnd > yStart) t = (float)(y - yStart) / (float)(yEnd - yStart);
    r = 0.99f - 0.10f * t;
    g = 0.80f - 0.44f * t;
    b = 0.30f - 0.28f * t;
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

    if (polygonClosed && lastY >= 0 && lastY < GRID_H && !verts.empty()) {
        int gxMin = GRID_W, gxMax = 0;
        for (size_t i = 0; i < verts.size(); i++) {
            gxMin = std::min(gxMin, verts[i].x);
            gxMax = std::max(gxMax, verts[i].x);
        }
        float cy = 1.0f - (lastY + 0.5f) * ch;
        float half = ch * 0.30f;
        addQuad(v, -1.0f + gxMin * cw, cy + half, -1.0f + (gxMax + 1) * cw, cy - half,
                1.0f, 0.749f, 0.784f);
    }

    if (polygonClosed) {
        for (int gy = 0; gy < GRID_H; gy++) {
            for (int gx = 0; gx < GRID_W; gx++) {
                if (!filled[gy][gx]) continue;
                float r, g, b;
                spanColor(gy, r, g, b);
                addCell(v, gx, gy, r, g, b);
            }
        }
    }

    for (int gy = 0; gy < GRID_H; gy++)
        for (int gx = 0; gx < GRID_W; gx++)
            if (outline[gy][gx]) addCell(v, gx, gy, 0.114f, 0.114f, 0.114f);

    for (size_t i = 0; i < lastCross.size(); i++) {
        int x = lastCross[i];
        for (int dy = -1; dy <= 1; dy++) {
            int y = lastY + dy;
            if (x >= 0 && x < GRID_W && y >= 0 && y < GRID_H) addCell(v, x, y, 0.227f, 0.525f, 1.0f);
        }
    }

    if (!polygonClosed && (int)verts.size() > 0) {
        IPt a = verts.back();
        bresLine(a.x, a.y, mouseX, mouseY, [&v](int x, int y) {
            if (x >= 0 && x < GRID_W && y >= 0 && y < GRID_H && !outline[y][x])
                addCell(v, x, y, 0.70f, 0.72f, 0.78f);
        });
    }

    for (size_t i = 0; i < verts.size(); i++) {
        IPt a = verts[i];
        if (a.x >= 0 && a.x < GRID_W && a.y >= 0 && a.y < GRID_H)
            addCell(v, a.x, a.y, 0.20f, 0.24f, 0.55f);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, v.size() * sizeof(float), v.data());
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, (int)(v.size() / 5));
}

static void mouse_button_callback(GLFWwindow* w, int button, int action, int mods) {
    (void)mods;
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;
    double mx, my;
    glfwGetCursorPos(w, &mx, &my);
    int gx = (int)std::floor(mx / WIN_W * GRID_W);
    int gy = (int)std::floor(my / WIN_H * GRID_H);
    gx = std::max(0, std::min(GRID_W - 1, gx));
    gy = std::max(0, std::min(GRID_H - 1, gy));
    if (!polygonClosed) {
        verts.push_back({ gx, gy });
        std::printf("[Click] Dinh %d: (%d, %d)\n", (int)verts.size(), gx, gy);
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
            scanStep();
            return;
        case GLFW_KEY_T:
            if (!polygonClosed) { std::printf("[T] Hay dong da giac truoc.\n"); return; }
            if (finished) { std::printf("[T] Da quet xong, bam [R] de quet lai.\n"); return; }
            autoRun = !autoRun;
            std::printf("[T] Chay tu dong: %s (%d dong/buc)\n", autoRun ? "BAT" : "TAT", speed);
            return;
        case GLFW_KEY_G:
            scanAll();
            return;
        case GLFW_KEY_R:
            if (polygonClosed) { resetScan(); std::printf("[R] Reset, quet lai tu y=%d.\n", yStart); }
            return;
        case GLFW_KEY_LEFT_BRACKET:
            speed = std::max(1, speed / 2);
            std::printf("[Toc do] %d dong/buc\n", speed);
            return;
        case GLFW_KEY_RIGHT_BRACKET:
            speed = std::min(64, speed * 2);
            std::printf("[Toc do] %d dong/buc\n", speed);
            return;
        case GLFW_KEY_C:
            clearAll();
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
        "TO THEO DONG QUET - Scanline Fill (Active Edge Table, co animation)", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { glfwTerminate(); return -1; }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_callback);
    glfwSetKeyCallback(window, key_callback);

    initGL();
    std::memset(outline, 0, sizeof(outline));
    std::memset(filled, 0, sizeof(filled));

    std::printf("=========================================================\n");
    std::printf("   TO THEO DONG QUET (Scanline Fill - Active Edge Table)\n");
    std::printf("   Luu do: nen trong da giac = xanh rat nhat | da to = vang->cam\n");
    std::printf("   dong quet hien tai = hong | giao diem = xanh duong | canh = den\n");
    std::printf("---------------------------------------------------------\n");
    std::printf("[Click trai] them dinh da giac\n");
    std::printf("[ENTER] dong da giac (it nhat 3 dinh)\n");
    std::printf("[SPACE] quet 1 dong | [T] chay tu dong | [G] quet het ngay\n");
    std::printf("[R] quet lai tu dau | [ ] tang/giam toc do\n");
    std::printf("[C] xoa het | [BACKSPACE] bo dinh | [ESC] thoat\n");
    std::printf("=========================================================\n");

    while (!glfwWindowShouldClose(window)) {
        if (autoRun && !finished) scanStep();
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        buildGeometry();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
