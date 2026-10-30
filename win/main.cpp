// Conflict: Middle East Political Simulator - Windows front end.
// A Win32 window that presents the game's 640x350 screen with Direct3D 12 and feeds
// the keyboard to the game core (the C files in ../port).
#include <windows.h>
#include <shlobj.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <stdio.h>
#include <string.h>

extern "C" {
int engine_start(const char *game_dir, const char *save_dir);
void engine_post_key(unsigned short bios_key);
int engine_copy_frame(unsigned char *bgra);
int engine_has_exited(void);
void engine_request_quit(void);
}

static const int GAME_W = 640, GAME_H = 350;
static const float DISPLAY_ASPECT = 4.0f / 3.0f;
static bool g_smooth = true, smooth_ok = true;          // the picture's look (see the shaders)       // EGA 640x350 was shown on a 4:3 monitor
static const UINT FRAMES = 2;

template <class T> static void release(T *&p) { if (p) { p->Release(); p = nullptr; } }

// ---------------------------------------------------------------- renderer
struct Renderer {
    HWND hwnd = nullptr;
    ID3D12Device *device = nullptr;
    ID3D12CommandQueue *queue = nullptr;
    IDXGISwapChain3 *swap = nullptr;
    ID3D12DescriptorHeap *rtvHeap = nullptr, *srvHeap = nullptr;
    ID3D12Resource *backbuf[FRAMES] = {}, *texture = nullptr, *upload = nullptr;
    ID3D12CommandAllocator *alloc = nullptr;
    ID3D12GraphicsCommandList *list = nullptr;
    ID3D12RootSignature *rootsig = nullptr;
    ID3D12PipelineState *pso = nullptr;
    ID3D12Fence *fence = nullptr;
    HANDLE fenceEvent = nullptr;
    UINT64 fenceValue = 0;
    UINT rtvSize = 0, width = 0, height = 0;
    UINT8 *uploadPtr = nullptr;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    unsigned char pixels[GAME_W * GAME_H * 4];
    char error[256] = "";

    bool fail(const char *what, HRESULT hr) { snprintf(error, sizeof error, "%s failed (0x%08lX)", what, (unsigned long)hr); return false; }

    void waitForGpu() {
        if (!queue || !fence) return;
        const UINT64 v = ++fenceValue;
        if (FAILED(queue->Signal(fence, v))) return;
        if (fence->GetCompletedValue() < v) { fence->SetEventOnCompletion(v, fenceEvent); WaitForSingleObject(fenceEvent, INFINITE); }
    }

    bool createBackbuffers() {
        D3D12_CPU_DESCRIPTOR_HANDLE h = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < FRAMES; i++) {
            HRESULT hr = swap->GetBuffer(i, IID_PPV_ARGS(&backbuf[i]));
            if (FAILED(hr)) return fail("GetBuffer", hr);
            device->CreateRenderTargetView(backbuf[i], nullptr, h);
            h.ptr += rtvSize;
        }
        return true;
    }

    bool init(HWND window, UINT w, UINT h) {
        hwnd = window; width = w ? w : 1; height = h ? h : 1;
        HRESULT hr;
        IDXGIFactory4 *factory = nullptr;
        if (FAILED(hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return fail("CreateDXGIFactory1", hr);
        hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
        if (FAILED(hr)) {                             // no suitable GPU: use the software rasteriser
            IDXGIAdapter *warp = nullptr;
            if (SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)))) { hr = D3D12CreateDevice(warp, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)); warp->Release(); }
        }
        if (FAILED(hr)) { factory->Release(); return fail("D3D12CreateDevice", hr); }

        D3D12_COMMAND_QUEUE_DESC qd = {}; qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        if (FAILED(hr = device->CreateCommandQueue(&qd, IID_PPV_ARGS(&queue)))) { factory->Release(); return fail("CreateCommandQueue", hr); }

        DXGI_SWAP_CHAIN_DESC1 sd = {};
        sd.Width = width; sd.Height = height; sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM; sd.SampleDesc.Count = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.BufferCount = FRAMES; sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        IDXGISwapChain1 *sc1 = nullptr;
        hr = factory->CreateSwapChainForHwnd(queue, hwnd, &sd, nullptr, nullptr, &sc1);
        if (SUCCEEDED(hr)) { factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER); hr = sc1->QueryInterface(IID_PPV_ARGS(&swap)); sc1->Release(); }
        factory->Release();
        if (FAILED(hr)) return fail("CreateSwapChainForHwnd", hr);

        D3D12_DESCRIPTOR_HEAP_DESC hd = {}; hd.NumDescriptors = FRAMES; hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        if (FAILED(hr = device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&rtvHeap)))) return fail("CreateDescriptorHeap(RTV)", hr);
        rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        hd.NumDescriptors = 1; hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (FAILED(hr = device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&srvHeap)))) return fail("CreateDescriptorHeap(SRV)", hr);
        if (!createBackbuffers()) return false;

        // The game screen as a texture, plus an upload buffer refreshed every frame.
        D3D12_HEAP_PROPERTIES hp = {}; hp.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC td = {};
        td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; td.Width = GAME_W; td.Height = GAME_H; td.DepthOrArraySize = 1; td.MipLevels = 1;
        td.Format = DXGI_FORMAT_B8G8R8A8_UNORM; td.SampleDesc.Count = 1;
        if (FAILED(hr = device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &td, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&texture)))) return fail("CreateCommittedResource(texture)", hr);
        UINT64 uploadBytes = 0;
        device->GetCopyableFootprints(&td, 0, 1, 0, &footprint, nullptr, nullptr, &uploadBytes);
        hp.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC bd = {};
        bd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; bd.Width = uploadBytes; bd.Height = 1; bd.DepthOrArraySize = 1; bd.MipLevels = 1;
        bd.SampleDesc.Count = 1; bd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        if (FAILED(hr = device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &bd, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload)))) return fail("CreateCommittedResource(upload)", hr);
        D3D12_RANGE noRead = {0, 0};
        if (FAILED(hr = upload->Map(0, &noRead, (void **)&uploadPtr))) return fail("Map", hr);
        D3D12_SHADER_RESOURCE_VIEW_DESC sv = {};
        sv.Format = td.Format; sv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; sv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; sv.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(texture, &sv, srvHeap->GetCPUDescriptorHandleForHeapStart());

        // Root signature: the texture, four float constants, one linear sampler.
        D3D12_DESCRIPTOR_RANGE range = {}; range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; range.NumDescriptors = 1;
        D3D12_ROOT_PARAMETER params[2] = {};
        params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; params[0].DescriptorTable.NumDescriptorRanges = 1;
        params[0].DescriptorTable.pDescriptorRanges = &range; params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS; params[1].Constants.Num32BitValues = 8; params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        D3D12_STATIC_SAMPLER_DESC samp = {};
        samp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samp.AddressU = samp.AddressV = samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; samp.MaxLOD = D3D12_FLOAT32_MAX; samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_ROOT_SIGNATURE_DESC rs = {}; rs.NumParameters = 2; rs.pParameters = params; rs.NumStaticSamplers = 1; rs.pStaticSamplers = &samp;
        ID3DBlob *blob = nullptr, *err = nullptr;
        if (FAILED(hr = D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &err))) { if (err) err->Release(); return fail("D3D12SerializeRootSignature", hr); }
        hr = device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootsig));
        blob->Release();
        if (FAILED(hr)) return fail("CreateRootSignature", hr);

        // Shaders, two looks (registry Software\Conflict SmoothPicture; the window menu switches): Smooth, xBR-style
        // edge smoothing with four samples per screen pixel (the algorithm of the Pool of Radiance port, documented
        // there and in app/App/main.swift); Sharp, "sharp bilinear". The plain sharp shader is kept as a fallback in
        // case a PC cannot compile the smooth one (its message goes to shader-error.txt beside the program).
        static const char hlsl[] =
            "cbuffer C : register(b0) { float2 scale; float2 pixPerTexel; float mode; float3 pad; };\n"
            "Texture2D tex : register(t0); SamplerState smp : register(s0);\n"
            "struct V { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
            "V vmain(uint id : SV_VertexID) {\n"
            "  float2 p = float2((id & 1) ? 1.0 : -1.0, (id & 2) ? 1.0 : -1.0);\n"
            "  V o; o.pos = float4(p * scale, 0, 1); o.uv = float2((p.x + 1) * 0.5, (1 - p.y) * 0.5); return o; }\n"
            "float3 tx(int x, int y) { return tex.Load(int3(clamp(x, 0, 639), clamp(y, 0, 349), 0)).rgb; }\n"
            "float cdist(float3 a, float3 b) { float3 d = a - b;\n"
            "  return 48 * abs(dot(d, float3(0.299, 0.587, 0.114))) + 7 * abs(dot(d, float3(-0.169, -0.331, 0.5))) + 6 * abs(dot(d, float3(0.5, -0.419, -0.081))); }\n"
            "bool same(float3 a, float3 b) { return cdist(a, b) < 0.1; }\n"
            "float3 xbr(float2 s, float d) {\n"
            "  int ix = (int)floor(s.x), iy = (int)floor(s.y); float fx = s.x - ix, fy = s.y - iy;\n"
            "  int dx = fx < 0.5 ? -1 : 1, dy = fy < 0.5 ? -1 : 1;\n"
            "  if (dx < 0) fx = 1 - fx; if (dy < 0) fy = 1 - fy;\n"
            "  float3 E = tx(ix, iy), F = tx(ix + dx, iy), H = tx(ix, iy + dy), I = tx(ix + dx, iy + dy);\n"
            "  float3 B = tx(ix, iy - dy), D = tx(ix - dx, iy), C = tx(ix + dx, iy - dy), G = tx(ix - dx, iy + dy);\n"
            "  float3 F4 = tx(ix + 2 * dx, iy), I4 = tx(ix + 2 * dx, iy + dy), H5 = tx(ix, iy + 2 * dy), I5 = tx(ix + dx, iy + 2 * dy);\n"
            "  float e = cdist(E, C) + cdist(E, G) + cdist(I, F4) + cdist(I, H5) + 4 * cdist(H, F);\n"
            "  float i = cdist(H, D) + cdist(H, I5) + cdist(F, I4) + cdist(F, B) + 4 * cdist(E, I);\n"
            "  bool edge = e < i && !same(E, F) && !same(E, H) && (!(same(F, B) && same(F, C)) || !(same(H, D) && same(H, G)));\n"
            "  if (!edge) return E;\n"
            "  bool shallow = 2 * cdist(F, G) <= cdist(H, C) && !same(E, G) && !same(D, G);\n"
            "  bool steep = 2 * cdist(H, C) <= cdist(F, G) && !same(E, C) && !same(B, C);\n"
            "  float a = smoothstep(1.5 - d, 1.5 + d, fx + fy);\n"
            "  if (shallow) a = max(a, smoothstep(2.0 - 1.5 * d, 2.0 + 1.5 * d, fx + 2 * fy));\n"
            "  if (steep) a = max(a, smoothstep(2.0 - 1.5 * d, 2.0 + 1.5 * d, 2 * fx + fy));\n"
            "  return lerp(E, cdist(E, F) <= cdist(E, H) ? F : H, a); }\n"
            "float4 pmain(V i) : SV_Target {\n"
            "  float2 size = float2(640.0, 350.0); float2 t = i.uv * size; float2 k = max(pixPerTexel, float2(1, 1));\n"
            "  if (mode > 0.5) { float2 o = 0.25 / k; float d = 0.5 / min(k.x, k.y);\n"
            "    float3 c = xbr(t + float2(-o.x, -o.y), d) + xbr(t + float2(o.x, -o.y), d) + xbr(t + float2(-o.x, o.y), d) + xbr(t + float2(o.x, o.y), d);\n"
            "    return float4(c * 0.25, 1); }\n"
            "  float2 b = floor(t); float2 f = saturate((t - b - 0.5) * k + 0.5);\n"
            "  return float4(tex.Sample(smp, (b + f) / size).rgb, 1.0); }\n";
        static const char hlsl_fallback[] =
            "cbuffer C : register(b0) { float2 scale; float2 pixPerTexel; float mode; float3 pad; };\n"
            "Texture2D tex : register(t0); SamplerState smp : register(s0);\n"
            "struct V { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
            "V vmain(uint id : SV_VertexID) {\n"
            "  float2 p = float2((id & 1) ? 1.0 : -1.0, (id & 2) ? 1.0 : -1.0);\n"
            "  V o; o.pos = float4(p * scale, 0, 1); o.uv = float2((p.x + 1) * 0.5, (1 - p.y) * 0.5); return o; }\n"
            "float4 pmain(V i) : SV_Target {\n"
            "  float2 size = float2(640.0, 350.0);\n"
            "  float2 t = i.uv * size; float2 b = floor(t); float2 f = t - b;\n"
            "  f = saturate((f - 0.5) * max(pixPerTexel, float2(1.0, 1.0)) + 0.5);\n"
            "  return float4(tex.Sample(smp, (b + f) / size).rgb, 1.0); }\n";
        ID3DBlob *vs = nullptr, *ps = nullptr;
        if (FAILED(D3DCompile(hlsl, sizeof hlsl - 1, "conflict", nullptr, nullptr, "vmain", "vs_5_0", 0, 0, &vs, &err)) ||
            FAILED(D3DCompile(hlsl, sizeof hlsl - 1, "conflict", nullptr, nullptr, "pmain", "ps_5_0", 0, 0, &ps, &err))) {
            char path[MAX_PATH]; GetModuleFileNameA(nullptr, path, sizeof path); char *sl = strrchr(path, '\\'); if (sl) *sl = 0;
            strncat(path, "\\shader-error.txt", sizeof path - strlen(path) - 1);
            if (FILE *f = fopen(path, "wb")) { if (err) fwrite(err->GetBufferPointer(), 1, err->GetBufferSize(), f); fclose(f); }
            if (err) { err->Release(); err = nullptr; } if (vs) { vs->Release(); vs = nullptr; }
            smooth_ok = false;
            if (FAILED(hr = D3DCompile(hlsl_fallback, sizeof hlsl_fallback - 1, "conflict", nullptr, nullptr, "vmain", "vs_5_0", 0, 0, &vs, &err))) { if (err) err->Release(); return fail("D3DCompile(vertex)", hr); }
            if (FAILED(hr = D3DCompile(hlsl_fallback, sizeof hlsl_fallback - 1, "conflict", nullptr, nullptr, "pmain", "ps_5_0", 0, 0, &ps, &err))) { if (err) err->Release(); vs->Release(); return fail("D3DCompile(pixel)", hr); }
        }
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pd = {};
        pd.pRootSignature = rootsig;
        pd.VS.pShaderBytecode = vs->GetBufferPointer(); pd.VS.BytecodeLength = vs->GetBufferSize();
        pd.PS.pShaderBytecode = ps->GetBufferPointer(); pd.PS.BytecodeLength = ps->GetBufferSize();
        pd.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        pd.SampleMask = 0xffffffffu;
        pd.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; pd.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; pd.RasterizerState.DepthClipEnable = TRUE;
        pd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pd.NumRenderTargets = 1; pd.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM; pd.SampleDesc.Count = 1;
        hr = device->CreateGraphicsPipelineState(&pd, IID_PPV_ARGS(&pso));
        vs->Release(); ps->Release();
        if (FAILED(hr)) return fail("CreateGraphicsPipelineState", hr);

        if (FAILED(hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&alloc)))) return fail("CreateCommandAllocator", hr);
        if (FAILED(hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, alloc, pso, IID_PPV_ARGS(&list)))) return fail("CreateCommandList", hr);
        list->Close();
        if (FAILED(hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) return fail("CreateFence", hr);
        fenceEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
        return true;
    }

    void resize(UINT w, UINT h) {
        if (!swap || w == 0 || h == 0 || (w == width && h == height)) return;
        waitForGpu();
        for (UINT i = 0; i < FRAMES; i++) release(backbuf[i]);
        if (FAILED(swap->ResizeBuffers(FRAMES, w, h, DXGI_FORMAT_B8G8R8A8_UNORM, 0))) return;
        width = w; height = h;
        createBackbuffers();
    }

    static D3D12_RESOURCE_BARRIER transition(ID3D12Resource *r, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to) {
        D3D12_RESOURCE_BARRIER b = {};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; b.Transition.pResource = r;
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES; b.Transition.StateBefore = from; b.Transition.StateAfter = to;
        return b;
    }

    void draw() {
        if (!list || !backbuf[0]) return;
        engine_copy_frame(pixels);
        for (int y = 0; y < GAME_H; y++)
            memcpy(uploadPtr + footprint.Offset + (size_t)y * footprint.Footprint.RowPitch, pixels + (size_t)y * GAME_W * 4, GAME_W * 4);

        alloc->Reset();
        list->Reset(alloc, pso);
        D3D12_RESOURCE_BARRIER b = transition(texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
        list->ResourceBarrier(1, &b);
        D3D12_TEXTURE_COPY_LOCATION dst = {}, src = {};
        dst.pResource = texture; dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX; dst.SubresourceIndex = 0;
        src.pResource = upload; src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; src.PlacedFootprint = footprint;
        list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        b = transition(texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        list->ResourceBarrier(1, &b);

        const UINT frame = swap->GetCurrentBackBufferIndex();
        b = transition(backbuf[frame], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        list->ResourceBarrier(1, &b);
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtv.ptr += (SIZE_T)frame * rtvSize;
        const float black[4] = {0, 0, 0, 1};
        list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        list->ClearRenderTargetView(rtv, black, 0, nullptr);
        D3D12_VIEWPORT vp = {0, 0, (float)width, (float)height, 0, 1};
        D3D12_RECT sc = {0, 0, (LONG)width, (LONG)height};
        list->RSSetViewports(1, &vp);
        list->RSSetScissorRects(1, &sc);

        float c[8] = {1, 1, 0, 0, g_smooth && smooth_ok ? 1.f : 0.f, 0, 0, 0};   // letterbox to 4:3; the look
        const float aspect = (float)width / (float)height;
        if (aspect > DISPLAY_ASPECT) c[0] = DISPLAY_ASPECT / aspect; else c[1] = aspect / DISPLAY_ASPECT;
        c[2] = (float)width * c[0] / GAME_W; c[3] = (float)height * c[1] / GAME_H;
        list->SetGraphicsRootSignature(rootsig);
        list->SetDescriptorHeaps(1, &srvHeap);
        list->SetGraphicsRootDescriptorTable(0, srvHeap->GetGPUDescriptorHandleForHeapStart());
        list->SetGraphicsRoot32BitConstants(1, 8, c, 0);
        list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        list->DrawInstanced(4, 1, 0, 0);

        b = transition(backbuf[frame], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        list->ResourceBarrier(1, &b);
        list->Close();
        ID3D12CommandList *lists[] = {list};
        queue->ExecuteCommandLists(1, lists);
        swap->Present(1, 0);
        waitForGpu();
    }

    void shutdown() {
        waitForGpu();
        if (fenceEvent) CloseHandle(fenceEvent);
        release(fence); release(list); release(alloc); release(pso); release(rootsig);
        release(upload); release(texture);
        for (UINT i = 0; i < FRAMES; i++) release(backbuf[i]);
        release(srvHeap); release(rtvHeap); release(swap); release(queue); release(device);
    }
};

static Renderer g_renderer;

// Fallback presenter using plain GDI, in case Direct3D 12 is unavailable (or "-gdi" is given).
static bool g_use_gdi = false;
static void gdi_draw(HWND hwnd) {
    static unsigned char pixels[GAME_W * GAME_H * 4];
    engine_copy_frame(pixels);
    RECT rc; GetClientRect(hwnd, &rc);
    int w = rc.right, h = rc.bottom;
    if (w <= 0 || h <= 0) return;
    int dw = w, dh = h;
    if ((float)w / (float)h > DISPLAY_ASPECT) dw = (int)(h * DISPLAY_ASPECT); else dh = (int)(w / DISPLAY_ASPECT);
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof bi.bmiHeader; bi.bmiHeader.biWidth = GAME_W; bi.bmiHeader.biHeight = -GAME_H;
    bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
    HDC dc = GetDC(hwnd);
    SetStretchBltMode(dc, COLORONCOLOR);
    StretchDIBits(dc, (w - dw) / 2, (h - dh) / 2, dw, dh, 0, 0, GAME_W, GAME_H, pixels, &bi, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(hwnd, dc);
}

// ---------------------------------------------------------------- keyboard
// Windows key messages -> the BIOS scancode/ASCII pairs the game expects.
static unsigned short special_key(WPARAM vk) {
    switch (vk) {
    case VK_UP: return 0x4800; case VK_DOWN: return 0x5000; case VK_LEFT: return 0x4B00; case VK_RIGHT: return 0x4D00;
    case VK_HOME: return 0x4700; case VK_END: return 0x4F00; case VK_PRIOR: return 0x4900; case VK_NEXT: return 0x5100;
    case VK_DELETE: return 0x5300;
    }
    if (vk >= VK_F1 && vk <= VK_F10) return (unsigned short)((0x3B + (vk - VK_F1)) << 8);
    return 0;
}
static unsigned short char_key(WPARAM ch) {
    static const char *rows[4] = {"1234567890-=", "qwertyuiop[]", "asdfghjkl;'`", "\\zxcvbnm,./"};
    static const unsigned char start[4] = {0x02, 0x10, 0x1E, 0x2B};
    if (ch == '\r') return 0x1C0D;
    if (ch == 27) return 0x011B;
    if (ch == '\t') return 0x0F09;
    if (ch == 8) return 0x0E08;
    if (ch == ' ') return 0x3920;
    if (ch < 32 || ch > 126) return 0;
    char lower = (char)((ch >= 'A' && ch <= 'Z') ? ch + 32 : ch);
    unsigned scan = 0;
    for (int r = 0; r < 4; r++) { const char *p = strchr(rows[r], lower); if (p) { scan = start[r] + (unsigned)(p - rows[r]); break; } }
    return (unsigned short)((scan << 8) | (unsigned)ch);
}

// ---------------------------------------------------------------- window
static bool g_fullscreen = false;
static WINDOWPLACEMENT g_placement = {sizeof(WINDOWPLACEMENT)};

static void toggle_fullscreen(HWND hwnd) {            // borderless full screen on the current monitor
    DWORD style = (DWORD)GetWindowLongPtrA(hwnd, GWL_STYLE);
    if (!g_fullscreen) {
        MONITORINFO mi = {sizeof mi};
        if (GetWindowPlacement(hwnd, &g_placement) && GetMonitorInfoA(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
            SetWindowLongPtrA(hwnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            g_fullscreen = true;
        }
    } else {
        SetWindowLongPtrA(hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hwnd, &g_placement);
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        g_fullscreen = false;
    }
}

// The picture's look, in HKEY_CURRENT_USER\Software\Conflict (SmoothPicture), switched from the window menu.
enum { IDM_SMOOTH = 0x0100, IDM_SHARP = 0x0110 };          // system menu commands: multiples of 16, below F000h
static DWORD reg_get(const char *name, DWORD dflt) { DWORD v = dflt, n = sizeof v; RegGetValueA(HKEY_CURRENT_USER, "Software\\Conflict", name, RRF_RT_REG_DWORD, nullptr, &v, &n); return v; }
static void reg_set(const char *name, DWORD v) {
    HKEY k; if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Conflict", 0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr) == ERROR_SUCCESS) { RegSetValueExA(k, name, 0, REG_DWORD, (const BYTE *)&v, sizeof v); RegCloseKey(k); }
}
static void picture_menu(HWND hwnd) {
    HMENU m = GetSystemMenu(hwnd, FALSE);
    CheckMenuRadioItem(m, IDM_SMOOTH, IDM_SHARP, g_smooth && smooth_ok ? IDM_SMOOTH : IDM_SHARP, MF_BYCOMMAND);
    if (!smooth_ok) EnableMenuItem(m, IDM_SMOOTH, MF_BYCOMMAND | MF_GRAYED);
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_SIZE:
        if (wp != SIZE_MINIMIZED && !g_use_gdi) g_renderer.resize(LOWORD(lp), HIWORD(lp));
        if (g_use_gdi) InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    case WM_SYSKEYDOWN:
        if (wp == VK_RETURN && (lp & (1 << 29))) { toggle_fullscreen(hwnd); return 0; }     // Alt+Enter
        break;
    case WM_SYSCHAR:
        if (wp == VK_RETURN) return 0;                // swallow the Alt+Enter beep
        break;
    case WM_KEYDOWN:
        if (wp == VK_F11) { toggle_fullscreen(hwnd); return 0; }
        if (unsigned short k = special_key(wp)) engine_post_key(k);
        return 0;
    case WM_CHAR:
        if (unsigned short k = char_key(wp)) engine_post_key(k);
        return 0;
    case WM_SYSCOMMAND:
        if ((wp & 0xfff0) == IDM_SMOOTH || (wp & 0xfff0) == IDM_SHARP) {
            g_smooth = (wp & 0xfff0) == IDM_SMOOTH; reg_set("SmoothPicture", g_smooth); picture_menu(hwnd); return 0;
        }
        break;
    case WM_GETMINMAXINFO: { MINMAXINFO *m = (MINMAXINFO *)lp; m->ptMinTrackSize.x = 660; m->ptMinTrackSize.y = 520; return 0; }
    case WM_CLOSE: DestroyWindow(hwnd); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static int fatal(const char *text) { MessageBoxA(nullptr, text, "Conflict cannot start", MB_OK | MB_ICONERROR); return 1; }

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR cmdline, int show) {
    SetProcessDPIAware();
    g_use_gdi = cmdline && strstr(cmdline, "-gdi") != nullptr;
    // Game data sits in a "data" folder next to the program; saves go to %APPDATA%\Conflict.
    char exe[MAX_PATH], game_dir[MAX_PATH], save_dir[MAX_PATH], appdata[MAX_PATH];
    GetModuleFileNameA(nullptr, exe, sizeof exe);
    char *slash = strrchr(exe, '\\'); if (slash) *slash = 0;
    snprintf(game_dir, sizeof game_dir, "%s\\data", exe);
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, appdata))) snprintf(save_dir, sizeof save_dir, "%s\\Conflict", appdata);
    else snprintf(save_dir, sizeof save_dir, "%s", exe);
    CreateDirectoryA(save_dir, nullptr);

    WNDCLASSA wc = {};
    wc.lpfnWndProc = wndproc; wc.hInstance = inst; wc.lpszClassName = "ConflictWindow";
    wc.hCursor = LoadCursorA(nullptr, (LPCSTR)IDC_ARROW); wc.hIcon = LoadIconA(inst, MAKEINTRESOURCEA(1));
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassA(&wc);
    RECT r = {0, 0, 960, 720};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hwnd = CreateWindowA("ConflictWindow", "Conflict: Middle East Political Simulator", WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, nullptr, nullptr, inst, nullptr);
    if (!hwnd) return fatal("Could not create the window.");
    RECT client; GetClientRect(hwnd, &client);
    if (!g_use_gdi && !g_renderer.init(hwnd, (UINT)client.right, (UINT)client.bottom)) {
        g_renderer.shutdown();                        // carry on with the GDI presenter
        g_use_gdi = true;
        char title[400]; snprintf(title, sizeof title, "Conflict (GDI fallback: %s)", g_renderer.error);
        SetWindowTextA(hwnd, title);
    }
    g_smooth = reg_get("SmoothPicture", 1) != 0;
    { HMENU m = GetSystemMenu(hwnd, FALSE); AppendMenuA(m, MF_SEPARATOR, 0, nullptr);
      AppendMenuA(m, MF_STRING, IDM_SMOOTH, "Smooth Picture"); AppendMenuA(m, MF_STRING, IDM_SHARP, "Sharp Pixels"); picture_menu(hwnd); }
    if (engine_start(game_dir, save_dir) != 0) return fatal("The game data is missing. Keep the \"data\" folder next to Conflict.exe.");
    ShowWindow(hwnd, show);

    MSG msg = {};
    for (;;) {
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) goto done;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        if (engine_has_exited()) { DestroyWindow(hwnd); continue; }
        if (IsIconic(hwnd)) { Sleep(50); continue; }
        if (g_use_gdi) { gdi_draw(hwnd); Sleep(16); }
        else g_renderer.draw();                       // Present(1) paces this loop to the display
    }
done:
    engine_request_quit();
    if (!g_use_gdi) g_renderer.shutdown();
    return 0;
}
