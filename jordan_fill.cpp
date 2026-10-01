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

static std::vector<IPt> verts;
static bool outline[GRID_H][GRID_W];
static bool inside[GRID_H][GRID_W];

static bool polygonClosed = false;
static bool finished = false;
static bool autoRun = false;
static bool rayRight = true;
static int  jx = 0, jy = 0;
static int  speed = 1;
static int  insideCount = 0;
static int  mouseX = 0, mouseY = 0;

static double rayEndX = 0.0;
static bool  rayHit = false;
static int   rayCount = 0;

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
    std::memset(inside, 0, sizeof(inside));
    verts.clear();
    polygonClosed = false;
    finished = false;
    autoRun = false;
    jx = 0; jy = 0;
    insideCount = 0;
    rayHit = false;
    rayCount = 0;
    std::printf("[C] Da xoa toan bo.\n");
}

static void resetScan() {
    std::memset(inside, 0, sizeof(inside));
    jx = 0; jy = 0;
    finished = false;
    autoRun = false;
    insideCount = 0;
    rayHit = false;
    rayCount = 0;
    std::printf("[R] Reset, quet lai tu pixel (0,0).\n");
}

static void closePolygon() {
    if ((int)verts.size() < 3) {
        std::printf("[Loi] Can it nhat 3 dinh (hien co %d).\n", (int)verts.size());
        return;
    }
    int n = (int)verts.size();
    for (int i = 0; i < n; i++) {
        IPt p = verts[i];
        IPt q = verts[(i + 1) % n];
        bresLine(p.x, p.y, q.x, q.y, [](int x, int y) {
            if (x >= 0 && x < GRID_W && y >= 0 && y < GRID_H) outline[y][x] = true;
        });
    }
    polygonClosed = true;
    resetScan();
    std::printf("[ENTER] Da dong da giac %d dinh. Bat dau quet tu pixel (0,0).\n", n);
    std::printf("[SPACE] 1 pixel | [T] chay tu dong | [G] quet het ngay\n");
    std::printf("[B] doi huong tia: hien tai %s\n", rayRight ? "sang PHAI" : "sang TRAI");
}

static int countCrossings(int px, int py, double& firstX) {
    int n = (int)verts.size();
    int cnt = 0;
    double best = 0.0;
    bool have = false;
    for (int i = 0; i < n; i++) {
        IPt p = verts[i];
        IPt q = verts[(i + 1) % n];
        if (p.y == q.y) continue;
        double y1 = (double)p.y, y2 = (double)q.y;
        double lo = std::min(y1, y2), hi = std::max(y1, y2);
        if ((double)py < lo || (double)py >= hi) continue;
        double xin = (double)p.x + ((double)py - y1) * ((double)q.x - (double)p.x) / (y2 - y1);
        bool side = rayRight ? (xin > (double)px) : (xin < (double)px);
        if (!side) continue;
        cnt++;
        if (!have || (rayRight ? xin < best : xin > best)) { best = xin; have = true; }
    }
    firstX = have ? best : 0.0;
    return have ? cnt : 0;
}

static void jordanStep() {
    if (!polygonClosed) { std::printf("[SPACE] Hay dong da giac truoc (ENTER).\n"); return; }
    if (finished) { std::printf("[SPACE] Da quet xit het.\n"); return; }

    int budget = speed;
    while (budget-- > 0 && !finished) {
        if (jy >= GRID_H) { finished = true; break; }

        double fx = 0.0;
        int c = countCrossings(jx, jy, fx);
        rayCount = c;
        rayEndX = fx;
        rayHit = (c > 0);

        if (c % 2 == 1) {
            if (!inside[jy][jx]) { inside[jy][jx] = true; insideCount++; }
        } else {
            inside[jy][jx] = false;
        }

        if (!autoRun)
            std::printf("(%2d,%2d) so cat=%d -> %s\n", jx, jy, c, (c % 2 == 1) ? "TRONG" : "NGOAI");
        else if (insideCount % 500 == 0 && insideCount > 0)
            std::printf("... dang quet (%2d,%2d) so cat=%d | da to %d px\n", jx, jy, c, insideCount);

        jx++;
        if (jx >= GRID_W) { jx = 0; jy++; }
    }

    if (jy >= GRID_H) {
        finished = true;
        autoRun = false;
        std::printf("[Xong] Quet %d x %d = %d pixel, %d pixel nam trong da giac.\n",
                    GRID_W, GRID_H, GRID_W * GRID_H, insideCount);
    }
}

static void jordanAll() {
    if (!polygonClosed) { std::printf("[G] Hay dong da giac truoc (ENTER).\n"); return; }
    autoRun = true;
    while (!finished) jordanStep();
    autoRun = false;
    std::printf("[G] Quet xong, %d pixel trong da giac.\n", insideCount);
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
            if (inside[gy][gx]) addCell(v, gx, gy, 0.227f, 0.525f, 1.0f);
        }
    }

    if (polygonClosed && !finished) {
        int ex = (int)std::floor(rayEndX);
        if (rayHit && ex >= 0 && ex < GRID_W) {
            int a = std::min(jx, ex), b = std::max(jx, ex);
            bresLine(a, jy, b, jy, [&v](int x, int y) {
                if (x >= 0 && x < GRID_W && y >= 0 && y < GRID_H) addCell(v, x, y, 1.0f, 0.706f, 0.635f);
            });
            for (int dy = -1; dy <= 1; dy++) {
                int y = jy + dy;
                if (y >= 0 && y < GRID_H) addCell(v, ex, y, 0.937f, 0.137f, 0.235f);
            }
        }
        if (jx >= 0 && jx < GRID_W && jy >= 0 && jy < GRID_H)
            addCell(v, jx, jy, 1.0f, 0.82f, 0.40f);
    }

    for (int gy = 0; gy < GRID_H; gy++)
        for (int gx = 0; gx < GRID_W; gx++)
            if (outline[gy][gx]) addCell(v, gx, gy, 0.13f, 0.13f, 0.16f);

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
    if (polygonClosed) {
        std::printf("[Click] Da giac da dong. dung [R]/[C] de ve lai.\n");
        return;
    }
    double mx, my;
    glfwGetCursorPos(w, &mx, &my);
    int gx = (int)std::floor(mx / WIN_W * GRID_W);
    int gy = (int)std::floor(my / WIN_H * GRID_H);
    gx = std::max(0, std::min(GRID_W - 1, gx));
    gy = std::max(0, std::min(GRID_H - 1, gy));
    verts.push_back({ gx, gy });
    std::printf("[Click] Dinh %d: (%d, %d)\n", (int)verts.size(), gx, gy);
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
            jordanStep();
            return;
        case GLFW_KEY_T:
            if (!polygonClosed) { std::printf("[T] Hay dong da giac truoc.\n"); return; }
            if (finished) { std::printf("[T] Da quet xong, bam [R] de quet lai.\n"); return; }
            autoRun = !autoRun;
            std::printf("[T] Chay tu dong: %s (%d px/buc)\n", autoRun ? "BAT" : "TAT", speed);
            return;
        case GLFW_KEY_G:
            jordanAll();
            return;
        case GLFW_KEY_B:
            rayRight = !rayRight;
            if (polygonClosed) resetScan();
            std::printf("[B] Huong tia: %s (ket qua phai giong nhau)\n", rayRight ? "PHAI" : "TRAI");
            return;
        case GLFW_KEY_R:
            if (polygonClosed) resetScan();
            return;
        case GLFW_KEY_LEFT_BRACKET:
            speed = std::max(1, speed / 2);
            std::printf("[Toc do] %d px/buc\n", speed);
            return;
        case GLFW_KEY_RIGHT_BRACKET:
            speed = std::min(512, speed * 2);
            std::printf("[Toc do] %d px/buc\n", speed);
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
        "TO THEO DINH LY JORDAN - Jordan Fill (chan-le, co animation)", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { glfwTerminate(); return -1; }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_callback);
    glfwSetKeyCallback(window, key_callback);

    initGL();
    std::memset(outline, 0, sizeof(outline));
    std::memset(inside, 0, sizeof(inside));

    std::printf("=========================================================\n");
    std::printf("   TO THEO DINH LY JORDAN (Jordan Fill - quy tac chan/le)\n");
    std::printf("   Luu do: trong da giac = xanh duong | tia = cam nhat\n");
    std::printf("   giao diem = do | pixel dang xet = vang | canh = den\n");
    std::printf("---------------------------------------------------------\n");
    std::printf("[Click trai] them dinh da giac\n");
    std::printf("[ENTER] dong da giac (it nhat 3 dinh)\n");
    std::printf("[SPACE] 1 pixel | [T] chay tu dong | [G] quet het ngay\n");
    std::printf("[B] doi huong tia (phai <-> trai) | [R] quet lai\n");
    std::printf("[ ] tang/giam toc do | [C] xoa het | [BACKSPACE] bo dinh | [ESC] thoat\n");
    std::printf("   Goi y: ve da giac tu cat (bat qua 4 dinh) de thay ro dac tinh chan-le.\n");
    std::printf("=========================================================\n");

    while (!glfwWindowShouldClose(window)) {
        if (autoRun && !finished) jordanStep();
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        buildGeometry();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
