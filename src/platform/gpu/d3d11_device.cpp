#include "platform/gpu/gpu_device.h"

#ifdef LTGUI_PLATFORM_WINDOWS

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include "log.h"
#include <cstdio>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace ltgui {
namespace gpu {

// ---- Embedded shaders ----

static const char* kSolidShader = R"(
struct VS_IN { float2 pos : POSITION; float4 col : COLOR; };
struct VS_OUT { float4 pos : SV_POSITION; float4 col : COLOR; };

VS_OUT VSMain(VS_IN input) {
    VS_OUT output;
    output.pos = float4(input.pos, 0.0, 1.0);
    output.col = input.col;
    return output;
}

float4 PSMain(VS_OUT input) : SV_TARGET {
    return input.col;
}
)";

static const char* kRoundedShader = R"(
struct VS_IN { float2 pos : POSITION; float2 uv : TEXCOORD; float4 col : COLOR; float4 params : TEXCOORD1; };
struct VS_OUT { float4 pos : SV_POSITION; float2 uv : TEXCOORD; float4 col : COLOR; float4 params : TEXCOORD1; };

cbuffer ScreenCB : register(b0) { float2 screenSize; };

VS_OUT VSMain(VS_IN input) {
    VS_OUT output;
    output.pos = float4(input.pos, 0.0, 1.0);
    output.uv = input.uv;
    output.col = input.col;
    output.params = input.params;
    return output;
}

float sdRoundedBox(float2 p, float2 size, float r) {
    float2 d = abs(p) - (size * 0.5) + r;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - r;
}

float4 PSMain(VS_OUT input) : SV_TARGET {
    // params: x=width, y=height, z=radius, w=strokeWidth (<0 = fill)
    float2 halfSize = float2(input.params.x * 0.5, input.params.y * 0.5);
    float radius = input.params.z;
    float stroke = input.params.w;
    float2 local = (input.uv - 0.5) * float2(input.params.x, input.params.y);

    if (stroke <= 0.0) {
        // Fill
        float d = sdRoundedBox(local, float2(input.params.x, input.params.y), radius);
        float alpha = 1.0 - saturate(d * 0.5);
        return input.col * alpha;
    } else {
        // Stroke
        float dOuter = sdRoundedBox(local, float2(input.params.x, input.params.y), radius);
        float dInner = sdRoundedBox(local,
            float2(input.params.x - stroke * 2.0, input.params.y - stroke * 2.0),
            max(radius - stroke, 0.0));
        float alpha = saturate((1.0 - saturate(dOuter * 0.5)) - (1.0 - saturate(dInner * 0.5)));
        return float4(input.col.rgb, input.col.a * alpha);
    }
}
)";

static const char* kEllipseShader = R"(
struct VS_IN { float2 pos : POSITION; float2 uv : TEXCOORD; float4 col : COLOR; float4 params : TEXCOORD1; };
struct VS_OUT { float4 pos : SV_POSITION; float2 uv : TEXCOORD; float4 col : COLOR; float4 params : TEXCOORD1; };

VS_OUT VSMain(VS_IN input) {
    VS_OUT output;
    output.pos = float4(input.pos, 0.0, 1.0);
    output.uv = input.uv;
    output.col = input.col;
    output.params = input.params;
    return output;
}

float4 PSMain(VS_OUT input) : SV_TARGET {
    float2 center = float2(input.params.x * 0.5, input.params.y * 0.5);
    float rx = input.params.x * 0.5;
    float ry = input.params.y * 0.5;
    float stroke = input.params.w;
    float2 local = (input.uv - 0.5) * float2(input.params.x, input.params.y);
    float dist = (local.x * local.x) / (rx * rx) + (local.y * local.y) / (ry * ry);

    if (stroke <= 0.0) {
        float alpha = 1.0 - saturate((dist - 1.0) * 4.0);
        return input.col * alpha;
    } else {
        float outer = dist;
        float inner = (local.x * local.x) / max(rx - stroke, 0.001f) / max(rx - stroke, 0.001f)
                    + (local.y * local.y) / max(ry - stroke, 0.001f) / max(ry - stroke, 0.001f);
        float alpha = saturate((1.0 - saturate((outer - 1.0) * 4.0))
                             - (1.0 - saturate((inner - 1.0) * 4.0)));
        return float4(input.col.rgb, input.col.a * alpha);
    }
}
)";

static const char* kTextureShader = R"(
struct VS_IN { float2 pos : POSITION; float2 uv : TEXCOORD; float4 col : COLOR; };
struct VS_OUT { float4 pos : SV_POSITION; float2 uv : TEXCOORD; float4 col : COLOR; };

SamplerState smp : register(s0);
Texture2D tex : register(t0);

VS_OUT VSMain(VS_IN input) {
    VS_OUT output;
    output.pos = float4(input.pos, 0.0, 1.0);
    output.uv = input.uv;
    output.col = input.col;
    return output;
}

float4 PSMain(VS_OUT input) : SV_TARGET {
    return tex.Sample(smp, input.uv) * input.col;
}
)";

// ---- D3D11 Texture ----

class D3D11Texture : public GpuTexture {
public:
    D3D11Texture(ID3D11Device* dev, ID3D11DeviceContext* ctx, int w, int h, const uint8_t* rgba)
        : device_(dev), context_(ctx), width_(w), height_(h) {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = w;
        desc.Height = h;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem = rgba;
        init.SysMemPitch = w * 4;

        HRESULT hr = dev->CreateTexture2D(&desc, rgba ? &init : nullptr, &tex_);
        if (FAILED(hr)) {
            LOG_ERROR("D3D11", "CreateTexture2D failed: 0x%08lx", hr);
            tex_ = nullptr;
            return;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        hr = dev->CreateShaderResourceView(tex_, &srvDesc, &srv_);
        if (FAILED(hr)) {
            LOG_ERROR("D3D11", "CreateShaderResourceView failed: 0x%08lx", hr);
            srv_ = nullptr;
        }
    }

    ~D3D11Texture() override {
        if (srv_) srv_->Release();
        if (tex_) tex_->Release();
    }

    int width() const override { return width_; }
    int height() const override { return height_; }

    void update(const uint8_t* rgba, int x, int y, int w, int h) override {
        D3D11_BOX box = {(UINT)x, (UINT)y, 0, (UINT)(x + w), (UINT)(y + h), 1};
        context_->UpdateSubresource(tex_, 0, &box, rgba, w * 4, 0);
    }

    void bind(int slot) override {
        context_->PSSetShaderResources(slot, 1, &srv_);
    }

    ID3D11Texture2D* tex() const { return tex_; }

private:
    ID3D11Device* device_;
    ID3D11DeviceContext* context_;
    ID3D11Texture2D* tex_ = nullptr;
    ID3D11ShaderResourceView* srv_ = nullptr;
    int width_, height_;
};

// ---- D3D11 Device ----

class D3D11Device : public GpuDevice {
public:
    D3D11Device() = default;

    const char* name() const override { return "D3D11"; }
    int width() const override { return width_; }
    int height() const override { return height_; }
    bool isValid() const { return device_ != nullptr; }

    bool initialize(void* windowHandle, int w, int h) override {
        HWND hwnd = static_cast<HWND>(windowHandle);
        width_ = w;
        height_ = h;

        // Build orthographic projection matrix for 2D: [0, w] x [0, h]
        float L = 0.0f, R = (float)w, T = 0.0f, B = (float)h;
        orthoProj_[0] = 2.0f / (R - L);
        orthoProj_[4] = -2.0f / (B - T);
        orthoProj_[12] = -(R + L) / (R - L);
        orthoProj_[13] = (B + T) / (B - T);
        orthoProj_[10] = 1.0f;
        orthoProj_[15] = 1.0f;

        // Create device and swap chain
        DXGI_SWAP_CHAIN_DESC scDesc = {};
        scDesc.BufferCount = 2;
        scDesc.BufferDesc.Width = w;
        scDesc.BufferDesc.Height = h;
        scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scDesc.BufferDesc.RefreshRate.Numerator = 60;
        scDesc.BufferDesc.RefreshRate.Denominator = 1;
        scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scDesc.OutputWindow = hwnd;
        scDesc.SampleDesc.Count = 1;
        scDesc.Windowed = TRUE;

        UINT flags = 0;
#ifdef _DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_FEATURE_LEVEL featureLevel;
        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
            nullptr, 0, D3D11_SDK_VERSION,
            &scDesc, &swapChain_, &device_, &featureLevel, &context_);

        if (FAILED(hr)) {
            // Try WARP (software) as fallback
            hr = D3D11CreateDeviceAndSwapChain(
                nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags,
                nullptr, 0, D3D11_SDK_VERSION,
                &scDesc, &swapChain_, &device_, &featureLevel, &context_);
        }

        if (FAILED(hr)) return false;

        // Compile shaders
        if (!compileShaders()) return false;

        // Create render target view
        createRenderTarget();

        // Set up blend state
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        device_->CreateBlendState(&blendDesc, &blendState_);

        // Default sampler
        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        device_->CreateSamplerState(&sampDesc, &sampler_);

        // Disable back-face culling — 2D rendering has no "back" faces.
        D3D11_RASTERIZER_DESC rsDesc = {};
        rsDesc.FillMode = D3D11_FILL_SOLID;
        rsDesc.CullMode = D3D11_CULL_NONE;
        rsDesc.ScissorEnable = FALSE;
        device_->CreateRasterizerState(&rsDesc, &rasterizerState_);

        return true;
    }

    void shutdown() override {
        if (rtView_) { rtView_->Release(); rtView_ = nullptr; }
        if (vertexBuffer_) { vertexBuffer_->Release(); vertexBuffer_ = nullptr; vertexBufferSize_ = 0; }
        if (blendState_) { blendState_->Release(); blendState_ = nullptr; }
        if (rasterizerState_) { rasterizerState_->Release(); rasterizerState_ = nullptr; }
        if (sampler_) { sampler_->Release(); sampler_ = nullptr; }
        if (solidVS_) { solidVS_->Release(); solidVS_ = nullptr; }
        if (solidPS_) { solidPS_->Release(); solidPS_ = nullptr; }
        if (roundedVS_) { roundedVS_->Release(); roundedVS_ = nullptr; }
        if (roundedPS_) { roundedPS_->Release(); roundedPS_ = nullptr; }
        if (ellipseVS_) { ellipseVS_->Release(); ellipseVS_ = nullptr; }
        if (ellipsePS_) { ellipsePS_->Release(); ellipsePS_ = nullptr; }
        if (texVS_) { texVS_->Release(); texVS_ = nullptr; }
        if (texPS_) { texPS_->Release(); texPS_ = nullptr; }
        if (ilSolid_) { ilSolid_->Release(); ilSolid_ = nullptr; }
        if (ilRounded_) { ilRounded_->Release(); ilRounded_ = nullptr; }
        if (context_) { context_->ClearState(); context_->Release(); context_ = nullptr; }
        if (swapChain_) { swapChain_->Release(); swapChain_ = nullptr; }
        if (device_) { device_->Release(); device_ = nullptr; }
    }

    void resize(int w, int h) override {
        width_ = w; height_ = h;
        if (rtView_) { rtView_->Release(); rtView_ = nullptr; }
        if (swapChain_) {
            HRESULT hr = swapChain_->ResizeBuffers(2, w, h, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
            if (SUCCEEDED(hr)) {
                createRenderTarget();
            }
        }

        float L = 0.0f, R = (float)w, T = 0.0f, B = (float)h;
        orthoProj_[0] = 2.0f / (R - L);
        orthoProj_[4] = -2.0f / (B - T);
        orthoProj_[12] = -(R + L) / (R - L);
        orthoProj_[13] = (B + T) / (B - T);
    }

    void beginFrame() override {
        if (!rtView_) return;
        float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // white — widget paint covers it
        context_->ClearRenderTargetView(rtView_, clearColor);
        context_->OMSetRenderTargets(1, &rtView_, nullptr);

        D3D11_VIEWPORT vp = { 0, 0, (float)width_, (float)height_, 0, 1 };
        context_->RSSetViewports(1, &vp);
        context_->PSSetSamplers(0, 1, &sampler_);

        float blendFactor[4] = { 1, 1, 1, 1 };
        context_->OMSetBlendState(blendState_, blendFactor, 0xFFFFFFFF);
        context_->RSSetState(rasterizerState_);
    }

    void endFrame() override {
        swapChain_->Present(1, 0);
    }

    void drawTriangles(const Vertex2D* verts, int count) override {
        mapVertices(verts, count, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void drawTriangleStrip(const Vertex2D* verts, int count) override {
        mapVertices(verts, count, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    }

    void drawLines(const Vertex2D* verts, int count) override {
        mapVertices(verts, count, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    }

    void selectShader(int type) override {
        switch (type) {
        case 0: useShader(ShaderType::Solid); break;
        case 1: useShader(ShaderType::Rounded); break;
        case 2: useShader(ShaderType::Ellipse); break;
        case 3: useShader(ShaderType::Texture); break;
        default: useShader(ShaderType::Solid); break;
        }
    }

    void bindTexture(int slot, GpuTexture* tex) override {
        if (tex) {
            tex->bind(slot);
        }
    }

    GpuTexture* createTexture(int w, int h, const uint8_t* rgba) override {
        return new D3D11Texture(device_, context_, w, h, rgba);
    }

    void destroyTexture(GpuTexture* tex) override {
        delete static_cast<D3D11Texture*>(tex);
    }

    void setBlend(bool enable) override {
        float factor[4] = { 1, 1, 1, 1 };
        context_->OMSetBlendState(enable ? blendState_ : nullptr, factor, 0xFFFFFFFF);
    }

    void setScissor(int x, int y, int w, int h) override {
        D3D11_RECT r = { (LONG)x, (LONG)y, (LONG)(x + w), (LONG)(y + h) };
        context_->RSSetScissorRects(1, &r);
    }

    void clearScissor() override {
        context_->RSSetScissorRects(0, nullptr);
    }

private:
    enum class ShaderType { Solid, Rounded, Ellipse, Texture };

    bool compileShaders() {
        auto compile = [this](const char* src, const char* entry, const char* target,
                              ID3DBlob** blob) -> bool {
            ID3DBlob* err = nullptr;
            HRESULT hr = D3DCompile(src, strlen(src), nullptr, nullptr, nullptr,
                                    entry, target, 0, 0, blob, &err);
            if (FAILED(hr) && err) {
                LOG_ERROR("D3D11", "Shader compile error (%s): %s",
                        entry, (const char*)err->GetBufferPointer());
                err->Release();
                return false;
            }
            return SUCCEEDED(hr);
        };

        ID3DBlob *vsBlob = nullptr, *psBlob = nullptr;

        // Solid shader
        if (compile(kSolidShader, "VSMain", "vs_4_0", &vsBlob) &&
            compile(kSolidShader, "PSMain", "ps_4_0", &psBlob)) {
            device_->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &solidVS_);
            device_->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &solidPS_);

            D3D11_INPUT_ELEMENT_DESC layout[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            device_->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &ilSolid_);
            vsBlob->Release(); psBlob->Release();
        } else { return false; }

        // Rounded shader
        if (compile(kRoundedShader, "VSMain", "vs_4_0", &vsBlob) &&
            compile(kRoundedShader, "PSMain", "ps_4_0", &psBlob)) {
            device_->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &roundedVS_);
            device_->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &roundedPS_);

            D3D11_INPUT_ELEMENT_DESC layout[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            device_->CreateInputLayout(layout, 4, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &ilRounded_);
            vsBlob->Release(); psBlob->Release();
        } else { return false; }

        // Ellipse shader
        if (compile(kEllipseShader, "VSMain", "vs_4_0", &vsBlob) &&
            compile(kEllipseShader, "PSMain", "ps_4_0", &psBlob)) {
            device_->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &ellipseVS_);
            device_->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &ellipsePS_);
            vsBlob->Release(); psBlob->Release();
        } else { return false; }

        // Texture shader
        if (compile(kTextureShader, "VSMain", "vs_4_0", &vsBlob) &&
            compile(kTextureShader, "PSMain", "ps_4_0", &psBlob)) {
            device_->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &texVS_);
            device_->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &texPS_);
            vsBlob->Release(); psBlob->Release();
        } else { return false; }

        return true;
    }

    void createRenderTarget() {
        ID3D11Texture2D* backBuffer = nullptr;
        HRESULT hr = swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        if (FAILED(hr) || !backBuffer) {
            LOG_ERROR("D3D11", "GetBuffer failed: 0x%08lx", hr);
            return;
        }
        hr = device_->CreateRenderTargetView(backBuffer, nullptr, &rtView_);
        backBuffer->Release();
        if (FAILED(hr)) {
            LOG_ERROR("D3D11", "CreateRenderTargetView failed: 0x%08lx", hr);
            rtView_ = nullptr;
        }
    }

    void useShader(ShaderType type) {
        switch (type) {
        case ShaderType::Solid:
            context_->VSSetShader(solidVS_, nullptr, 0);
            context_->PSSetShader(solidPS_, nullptr, 0);
            context_->IASetInputLayout(ilSolid_);
            break;
        case ShaderType::Rounded:
            context_->VSSetShader(roundedVS_, nullptr, 0);
            context_->PSSetShader(roundedPS_, nullptr, 0);
            context_->IASetInputLayout(ilRounded_);
            break;
        case ShaderType::Ellipse:
            context_->VSSetShader(ellipseVS_, nullptr, 0);
            context_->PSSetShader(ellipsePS_, nullptr, 0);
            context_->IASetInputLayout(ilRounded_);
            break;
        case ShaderType::Texture:
            context_->VSSetShader(texVS_, nullptr, 0);
            context_->PSSetShader(texPS_, nullptr, 0);
            context_->IASetInputLayout(ilRounded_);
            break;
        }
    }

    void mapVertices(const Vertex2D* verts, int count, D3D11_PRIMITIVE_TOPOLOGY topology) {
        UINT stride = sizeof(Vertex2D);
        UINT byteSize = count * stride;

        // Grow the reusable dynamic vertex buffer if needed
        if (!vertexBuffer_ || vertexBufferSize_ < byteSize) {
            if (vertexBuffer_) {
                vertexBuffer_->Release();
                vertexBuffer_ = nullptr;
            }
            vertexBufferSize_ = byteSize + byteSize / 2; // allocate 1.5x headroom

            D3D11_BUFFER_DESC desc = {};
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.ByteWidth = vertexBufferSize_;
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            device_->CreateBuffer(&desc, nullptr, &vertexBuffer_);
        }

        // Map and update
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        context_->Map(vertexBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, verts, byteSize);
        context_->Unmap(vertexBuffer_, 0);

        UINT offset = 0;
        context_->IASetVertexBuffers(0, 1, &vertexBuffer_, &stride, &offset);
        context_->IASetPrimitiveTopology(topology);
        context_->Draw(count, 0);
    }

    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    IDXGISwapChain* swapChain_ = nullptr;
    ID3D11RenderTargetView* rtView_ = nullptr;
    ID3D11BlendState* blendState_ = nullptr;
    ID3D11RasterizerState* rasterizerState_ = nullptr;
    ID3D11SamplerState* sampler_ = nullptr;

    // Dynamic vertex buffer (reused across frames)
    ID3D11Buffer* vertexBuffer_ = nullptr;
    UINT vertexBufferSize_ = 0;

    // Shaders
    ID3D11VertexShader* solidVS_ = nullptr;
    ID3D11PixelShader* solidPS_ = nullptr;
    ID3D11VertexShader* roundedVS_ = nullptr;
    ID3D11PixelShader* roundedPS_ = nullptr;
    ID3D11VertexShader* ellipseVS_ = nullptr;
    ID3D11PixelShader* ellipsePS_ = nullptr;
    ID3D11VertexShader* texVS_ = nullptr;
    ID3D11PixelShader* texPS_ = nullptr;

    // Input layouts
    ID3D11InputLayout* ilSolid_ = nullptr;
    ID3D11InputLayout* ilRounded_ = nullptr;

    float orthoProj_[16] = {};
    int width_ = 0, height_ = 0;
};

// ---- Factory ----

GpuDevice* CreateD3D11Device() {
    auto* dev = new D3D11Device();
    return dev;
}

} // namespace gpu
} // namespace ltgui

#endif // LTGUI_PLATFORM_WINDOWS
