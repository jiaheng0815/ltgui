#include "platform/gpu/gpu_device.h"

#ifdef LTGUI_PLATFORM_LINUX

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include "log.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace ltgui {
namespace gpu {

// ---- GLSL Shaders ----

static const char* kSolidVS = R"(#version 300 es
in vec2 aPos;
in vec4 aCol;
out vec4 vCol;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vCol = aCol;
}
)";

static const char* kSolidFS = R"(#version 300 es
precision mediump float;
in vec4 vCol;
out vec4 fragColor;
void main() {
    fragColor = vCol;
}
)";

static const char* kRoundedVS = R"(#version 300 es
in vec2 aPos;
in vec2 aUV;
in vec4 aCol;
in vec4 aParams;
out vec2 vUV;
out vec4 vCol;
out vec4 vParams;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vUV = aUV;
    vCol = aCol;
    vParams = aParams;
}
)";

static const char* kRoundedFS = R"(#version 300 es
precision mediump float;
in vec2 vUV;
in vec4 vCol;
in vec4 vParams;
out vec4 fragColor;

float sdRoundedBox(vec2 p, vec2 size, float r) {
    vec2 d = abs(p) - size * 0.5 + r;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - r;
}

void main() {
    float stroke = vParams.w;
    float radius = vParams.z;
    vec2 local = (vUV - 0.5) * vParams.xy;
    if (stroke <= 0.0) {
        float d = sdRoundedBox(local, vParams.xy, radius);
        float alpha = 1.0 - clamp(d * 0.5, 0.0, 1.0);
        fragColor = vCol * alpha;
    } else {
        float dOuter = sdRoundedBox(local, vParams.xy, radius);
        float dInner = sdRoundedBox(local, vParams.xy - vec2(stroke * 2.0),
                                     max(radius - stroke, 0.0));
        float alphaOuter = 1.0 - clamp(dOuter * 0.5, 0.0, 1.0);
        float alphaInner = 1.0 - clamp(dInner * 0.5, 0.0, 1.0);
        fragColor = vec4(vCol.rgb, vCol.a * (alphaOuter - alphaInner));
    }
}
)";

static const char* kEllipseFS = R"(#version 300 es
precision mediump float;
in vec2 vUV;
in vec4 vCol;
in vec4 vParams;
out vec4 fragColor;

void main() {
    float rx = vParams.x * 0.5;
    float ry = vParams.y * 0.5;
    float stroke = vParams.w;
    vec2 local = (vUV - 0.5) * vParams.xy;
    float dist = (local.x * local.x) / (rx * rx) + (local.y * local.y) / (ry * ry);
    if (stroke <= 0.0) {
        float alpha = 1.0 - clamp((dist - 1.0) * 4.0, 0.0, 1.0);
        fragColor = vCol * alpha;
    } else {
        float outer = dist;
        float innerDist = (local.x * local.x) / max(rx - stroke, 0.001f) / max(rx - stroke, 0.001f)
                        + (local.y * local.y) / max(ry - stroke, 0.001f) / max(ry - stroke, 0.001f);
        float alphaOuter = 1.0 - clamp((outer - 1.0) * 4.0, 0.0, 1.0);
        float alphaInner = 1.0 - clamp((innerDist - 1.0) * 4.0, 0.0, 1.0);
        fragColor = vec4(vCol.rgb, vCol.a * (alphaOuter - alphaInner));
    }
}
)";

static const char* kTextureVS = R"(#version 300 es
in vec2 aPos;
in vec2 aUV;
in vec4 aCol;
out vec2 vUV;
out vec4 vCol;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vUV = aUV;
    vCol = aCol;
}
)";

static const char* kTextureFS = R"(#version 300 es
precision mediump float;
in vec2 vUV;
in vec4 vCol;
out vec4 fragColor;
uniform sampler2D uTex;
void main() {
    fragColor = texture(uTex, vUV) * vCol;
}
)";

// ---- GL Helpers ----

static GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        LOG_ERROR("GL", "Shader compile error: %s", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint createProgram(const char* vsSrc, const char* fsSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    if (!vs || !fs) return 0;
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        LOG_ERROR("GL", "Link error: %s", log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

// ---- GL Texture ----

class GLTexture : public GpuTexture {
public:
    GLTexture(int w, int h, const uint8_t* rgba) : w_(w), h_(h) {
        glGenTextures(1, &tex_);
        glBindTexture(GL_TEXTURE_2D, tex_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    ~GLTexture() override { glDeleteTextures(1, &tex_); }
    int width() const override { return w_; }
    int height() const override { return h_; }
    void update(const uint8_t* rgba, int x, int y, int w, int h) override {
        glBindTexture(GL_TEXTURE_2D, tex_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    }
    void bind(int slot) override {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, tex_);
    }
    GLuint tex() const { return tex_; }
private:
    GLuint tex_ = 0;
    int w_, h_;
};

// ---- GLDevice ----

class GLDevice : public GpuDevice {
public:
    const char* name() const override { return "OpenGL ES 3.0"; }
    int width() const override { return w_; }
    int height() const override { return h_; }

    bool initialize(void* windowHandle, int width, int height) override {
        w_ = width; h_ = height;

        // Get native display and window
        EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY) {
            LOG_ERROR("GL", "eglGetDisplay failed");
            return false;
        }

        EGLint major, minor;
        if (!eglInitialize(display, &major, &minor)) {
            LOG_ERROR("GL", "eglInitialize failed");
            return false;
        }
        display_ = display;

        EGLint configAttr[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0,
            EGL_NONE
        };
        EGLConfig config;
        EGLint numConfigs;
        if (!eglChooseConfig(display, configAttr, &config, 1, &numConfigs) || numConfigs == 0) {
            // Try ES2 fallback
            configAttr[3] = EGL_OPENGL_ES2_BIT;
            if (!eglChooseConfig(display, configAttr, &config, 1, &numConfigs) || numConfigs == 0) {
                LOG_ERROR("GL", "No suitable EGL config");
                shutdown();
                return false;
            }
        }

        EGLint ctxAttr[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
        EGLContext ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttr);
        if (ctx == EGL_NO_CONTEXT) {
            // ES2 fallback
            ctxAttr[1] = 2;
            ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttr);
        }
        if (ctx == EGL_NO_CONTEXT) {
            LOG_ERROR("GL", "eglCreateContext failed");
            shutdown();
            return false;
        }
        context_ = ctx;

        EGLSurface surface = eglCreateWindowSurface(display, config,
            (EGLNativeWindowType)windowHandle, nullptr);
        if (surface == EGL_NO_SURFACE) {
            LOG_ERROR("GL", "eglCreateWindowSurface failed");
            shutdown();
            return false;
        }
        surface_ = surface;

        if (!eglMakeCurrent(display, surface, surface, ctx)) {
            LOG_ERROR("GL", "eglMakeCurrent failed");
            shutdown();
            return false;
        }

        // Compile shaders
        if (!compileShaders()) {
            shutdown();
            return false;
        }

        // Orthographic projection
        float L = 0, R = (float)w_, T = 0, B = (float)h_;
        ortho_[0] = 2.0f/(R-L); ortho_[4] = 0;           ortho_[8]  = 0; ortho_[12] = -(R+L)/(R-L);
        ortho_[1] = 0;          ortho_[5] = -2.0f/(B-T);  ortho_[9]  = 0; ortho_[13] = (B+T)/(B-T);
        ortho_[2] = 0;          ortho_[6] = 0;            ortho_[10] = 1; ortho_[14] = 0;
        ortho_[3] = 0;          ortho_[7] = 0;            ortho_[11] = 0; ortho_[15] = 1;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        return true;
    }

    void shutdown() override {
        if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; vboSize_ = 0; }
        if (display_ != EGL_NO_DISPLAY) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (surface_ != EGL_NO_SURFACE) eglDestroySurface(display_, surface_);
            if (context_ != EGL_NO_CONTEXT) eglDestroyContext(display_, context_);
            eglTerminate(display_);
        }
        glDeleteProgram(solidProg_);
        glDeleteProgram(roundedProg_);
        glDeleteProgram(ellipseProg_);
        glDeleteProgram(texProg_);
    }

    void resize(int w, int h) override {
        w_ = w; h_ = h;
        glViewport(0, 0, w, h);
        float L = 0, R = (float)w_, T = 0, B = (float)h_;
        ortho_[0] = 2.0f/(R-L); ortho_[12] = -(R+L)/(R-L);
        ortho_[5] = -2.0f/(B-T); ortho_[13] = (B+T)/(B-T);
    }

    void beginFrame() override {
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void endFrame() override {
        eglSwapBuffers(display_, surface_);
    }

    void drawTriangles(const Vertex2D* verts, int count) override {
        GLuint prog = curProg_ ? curProg_ : solidProg_;
        glUseProgram(prog);
        uploadAndDraw(verts, count, GL_TRIANGLES);
    }

    void drawTriangleStrip(const Vertex2D* verts, int count) override {
        GLuint prog = curProg_ ? curProg_ : solidProg_;
        glUseProgram(prog);
        uploadAndDraw(verts, count, GL_TRIANGLE_STRIP);
    }

    void drawLines(const Vertex2D* verts, int count) override {
        GLuint prog = curProg_ ? curProg_ : solidProg_;
        glUseProgram(prog);
        uploadAndDraw(verts, count, GL_LINES);
    }

    GpuTexture* createTexture(int w, int h, const uint8_t* rgba) override {
        return new GLTexture(w, h, rgba);
    }

    void destroyTexture(GpuTexture* tex) override { delete static_cast<GLTexture*>(tex); }

    void setBlend(bool enable) override {
        if (enable) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    }

    void setScissor(int x, int y, int w, int h) override {
        glEnable(GL_SCISSOR_TEST);
        glScissor(x, h_ - y - h, w, h);
    }

    void clearScissor() override { glDisable(GL_SCISSOR_TEST); }

    void selectShader(int type) override {
        switch (type) {
        case 0: curProg_ = solidProg_; break;
        case 1: curProg_ = roundedProg_; break;
        case 2: curProg_ = ellipseProg_; break;
        case 3: curProg_ = texProg_; break;
        default: curProg_ = solidProg_; break;
        }
        glUseProgram(curProg_);
    }

    void bindTexture(int slot, GpuTexture* tex) override {
        if (tex) tex->bind(slot);
    }

private:
    bool compileShaders() {
        solidProg_   = createProgram(kSolidVS, kSolidFS);
        roundedProg_ = createProgram(kRoundedVS, kRoundedFS);
        ellipseProg_ = createProgram(kRoundedVS, kEllipseFS);
        texProg_     = createProgram(kTextureVS, kTextureFS);
        return solidProg_ && roundedProg_ && ellipseProg_ && texProg_;
    }

    void uploadAndDraw(const Vertex2D* verts, int count, GLenum mode) {
        GLsizei byteSize = count * sizeof(Vertex2D);

        // Grow the reusable VBO if needed
        if (!vbo_ || vboSize_ < byteSize) {
            if (vbo_) glDeleteBuffers(1, &vbo_);
            vboSize_ = byteSize + byteSize / 2; // 1.5x headroom
            glGenBuffers(1, &vbo_);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_);
            glBufferData(GL_ARRAY_BUFFER, vboSize_, nullptr, GL_DYNAMIC_DRAW);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        }

        // Upload: orphan + map
        glBufferData(GL_ARRAY_BUFFER, byteSize, nullptr, GL_DYNAMIC_DRAW);
        void* ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, byteSize,
                                     GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        if (ptr) {
            memcpy(ptr, verts, byteSize);
            glUnmapBuffer(GL_ARRAY_BUFFER);
        }

        GLuint prog = curProg_ ? curProg_ : solidProg_;
        // Position at offset 0
        GLint aPos = glGetAttribLocation(prog, "aPos");
        if (aPos >= 0) {
            glEnableVertexAttribArray(aPos);
            glVertexAttribPointer(aPos, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)0);
        }
        // Color at offset 16 (after x,y,u,v = 4+4+4+4 bytes)
        GLint aCol = glGetAttribLocation(prog, "aCol");
        if (aCol >= 0) {
            glEnableVertexAttribArray(aCol);
            glVertexAttribPointer(aCol, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex2D), (void*)16);
        }
        // UV at offset 8
        GLint aUV = glGetAttribLocation(prog, "aUV");
        if (aUV >= 0) {
            glEnableVertexAttribArray(aUV);
            glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)8);
        }
        // Params at offset 20
        GLint aParams = glGetAttribLocation(prog, "aParams");
        if (aParams >= 0) {
            glEnableVertexAttribArray(aParams);
            glVertexAttribPointer(aParams, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)20);
        }

        glDrawArrays(mode, 0, count);

        if (aPos >= 0) glDisableVertexAttribArray(aPos);
        if (aCol >= 0) glDisableVertexAttribArray(aCol);
        if (aUV >= 0) glDisableVertexAttribArray(aUV);
        if (aParams >= 0) glDisableVertexAttribArray(aParams);
    }

    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLSurface surface_ = EGL_NO_SURFACE;
    EGLContext context_ = EGL_NO_CONTEXT;
    GLuint solidProg_ = 0, roundedProg_ = 0, ellipseProg_ = 0, texProg_ = 0, curProg_ = 0;
    GLuint vbo_ = 0;
    GLsizei vboSize_ = 0;
    float ortho_[16] = {};
    int w_ = 0, h_ = 0;
};

// ---- Factory ----

GpuDevice* CreateGLDevice() {
    return new GLDevice();
}

} // namespace gpu
} // namespace ltgui

#endif // LTGUI_PLATFORM_LINUX
