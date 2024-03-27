#include <tiny3d.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define COBJMACROS
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dwmapi.h>
#include <wincodec.h>
static IDXGISwapChain *swapChain;
static ID3D11Device *device;
static ID3D11DeviceContext *deviceContext;
static ID3D11RenderTargetView *renderTargetView;
static ID3D11DepthStencilView *depthStencilView;
static ID3D11Texture2D *texture;
static ID3D11ShaderResourceView *textureView;
static ID3D11SamplerState *sampler;
static ID3D11BlendState *blendState;
static ID3D11RasterizerState *rasterizerState;
static ID3D11InputLayout *layout;
static ID3D11VertexShader *vshader;
static ID3D11PixelShader *pshader;
static ID3D11DepthStencilState *depthStencilState;
typedef struct {
    float position[2];
    float texcoord[2];
} Vertex;
static LARGE_INTEGER freq, tstart, t0, t1;
void error_box(char *msg){
    MessageBoxA(0,msg,"Error",MB_ICONERROR);
}
uint32_t *load_image(bool flip_vertically, int *width, int *height, char *format, ...){
    va_list args;
    va_start(args,format);
    assertPath = local_path_to_absolute_internal(format,args);

    static IWICImagingFactory2 *ifactory = 0;
    if (!ifactory){
        ASSERT(SUCCEEDED(CoCreateInstance(&CLSID_WICImagingFactory2,0,CLSCTX_INPROC_SERVER,&IID_IWICImagingFactory2,&ifactory)));
        ASSERT(ifactory);
    }
    IWICBitmapDecoder *pDecoder = 0;
    size_t len = strlen(assertPath)+1;
    WCHAR *wpath = malloc(len*sizeof(*wpath));
    ASSERT(wpath);
    mbstowcs(wpath,assertPath,len);
    ASSERT_FILE(SUCCEEDED(ifactory->lpVtbl->CreateDecoderFromFilename(ifactory,wpath,0,GENERIC_READ,WICDecodeMetadataCacheOnDemand,&pDecoder)));
    free(wpath);
    IWICBitmapFrameDecode *pFrame = 0;
    ASSERT_FILE(SUCCEEDED(pDecoder->lpVtbl->GetFrame(pDecoder,0,&pFrame)));
    IWICBitmapSource *convertedSrc = 0;
    ASSERT_FILE(SUCCEEDED(WICConvertBitmapSource(&GUID_WICPixelFormat32bppRGBA,(IWICBitmapSource *)pFrame,&convertedSrc)));
    ASSERT_FILE(SUCCEEDED(convertedSrc->lpVtbl->GetSize(convertedSrc,width,height)));
    uint32_t size = width[0]*height[0]*sizeof(uint32_t);
    UINT rowPitch = width[0]*sizeof(uint32_t);
    uint32_t *pixels = malloc(size);
    ASSERT_FILE(pixels);
    if (flip_vertically){
        IWICBitmapFlipRotator *pFlipRotator;
        ASSERT_FILE(SUCCEEDED(ifactory->lpVtbl->CreateBitmapFlipRotator(ifactory,&pFlipRotator)));
        ASSERT_FILE(SUCCEEDED(pFlipRotator->lpVtbl->Initialize(pFlipRotator,convertedSrc,WICBitmapTransformFlipVertical)));
        ASSERT_FILE(SUCCEEDED(pFlipRotator->lpVtbl->CopyPixels(pFlipRotator,0,rowPitch,size,(BYTE *)pixels)));
        pFlipRotator->lpVtbl->Release(pFlipRotator);
    } else {
        ASSERT(SUCCEEDED(convertedSrc->lpVtbl->CopyPixels(convertedSrc,0,rowPitch,size,(BYTE *)pixels)));
    }
    convertedSrc->lpVtbl->Release(convertedSrc);
    pFrame->lpVtbl->Release(pFrame);
    pDecoder->lpVtbl->Release(pDecoder);
    
    va_end(args);

    return pixels;
}
void CreateRenderTargets(){
    ID3D11Texture2D *backBuffer = 0;
    ASSERT(SUCCEEDED(swapChain->lpVtbl->GetBuffer(swapChain, 0, &IID_ID3D11Texture2D, (void**)&backBuffer)));

    ASSERT(SUCCEEDED(device->lpVtbl->CreateRenderTargetView(device, (ID3D11Resource *)backBuffer, 0, &renderTargetView)));

    D3D11_TEXTURE2D_DESC depthStencilDesc;
    backBuffer->lpVtbl->GetDesc(backBuffer, &depthStencilDesc);
    backBuffer->lpVtbl->Release(backBuffer);

    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* depthStencil;

    ASSERT(SUCCEEDED(device->lpVtbl->CreateTexture2D(device, &depthStencilDesc, 0, &depthStencil)));
    ASSERT(SUCCEEDED(device->lpVtbl->CreateDepthStencilView(device, (ID3D11Resource *)depthStencil, 0, &depthStencilView)));

    depthStencil->lpVtbl->Release(depthStencil);
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam){
    switch(msg){
        case WM_CREATE:{
            DWORD darkTitlebar = 1;
            int DwmwaUseImmersiveDarkMode = 20,
                DwmwaUseImmersiveDarkModeBefore20h1 = 19;
            SUCCEEDED(DwmSetWindowAttribute(hwnd, DwmwaUseImmersiveDarkMode, &darkTitlebar, sizeof(darkTitlebar))) ||
                SUCCEEDED(DwmSetWindowAttribute(hwnd, DwmwaUseImmersiveDarkModeBefore20h1, &darkTitlebar, sizeof(darkTitlebar)));

            RECT wr;
            GetWindowRect(hwnd,&wr);
            SetWindowPos(hwnd,0,wr.left,wr.top,wr.right-wr.left,wr.bottom-wr.top,SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE); //prevent initial white frame

            UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifndef _DEBUG
            creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
            D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0};
            DXGI_SWAP_CHAIN_DESC swapChainDesc = {
                .BufferDesc.Width = 0,
                .BufferDesc.Height = 0,
                .BufferDesc.RefreshRate.Numerator = 0,
                .BufferDesc.RefreshRate.Denominator = 0,
                .BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM,
                .BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                .BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED,

                .SampleDesc.Count = 1,
                .SampleDesc.Quality = 0,

                .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                .BufferCount = 2,

                .OutputWindow = hwnd,
                .Windowed = TRUE,
                .SwapEffect = DXGI_SWAP_EFFECT_DISCARD,
                .Flags = 0
            };
            ASSERT(SUCCEEDED(D3D11CreateDeviceAndSwapChain(
                0,
                D3D_DRIVER_TYPE_HARDWARE,
                0,
                creationFlags,
                featureLevels,
                ARRAYSIZE(featureLevels),
                D3D11_SDK_VERSION,
                &swapChainDesc,
                &swapChain,
                &device,
                0,
                &deviceContext
            )));

            {
                IDXGIDevice* dxgiDevice;
                device->lpVtbl->QueryInterface(device,&IID_IDXGIDevice,&dxgiDevice);
                IDXGIAdapter* dxgiAdapter;
                dxgiDevice->lpVtbl->GetAdapter(dxgiDevice,&dxgiAdapter);
                IDXGIFactory* factory;
                dxgiAdapter->lpVtbl->GetParent(dxgiAdapter,&IID_IDXGIFactory,&factory);
                // disable silly Alt+Enter changing monitor resolution to match window size
                factory->lpVtbl->MakeWindowAssociation(factory,hwnd,DXGI_MWA_NO_ALT_ENTER);
                factory->lpVtbl->Release(factory);
                dxgiAdapter->lpVtbl->Release(dxgiAdapter);
                dxgiDevice->lpVtbl->Release(dxgiDevice);
            }

            {
                D3D11_TEXTURE2D_DESC desc ={
                    .Width = SCREEN_WIDTH,
                    .Height = SCREEN_HEIGHT,
                    .MipLevels = 1,
                    .ArraySize = 1,
                    .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
                    .SampleDesc = { 1, 0 },
                    .Usage = D3D11_USAGE_DEFAULT,
                    .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
                    .BindFlags = D3D11_BIND_SHADER_RESOURCE,
                };
                device->lpVtbl->CreateTexture2D(device,&desc,0,&texture);
                device->lpVtbl->CreateShaderResourceView(device,(ID3D11Resource *)texture,0,&textureView);
            }

            {
                D3D11_SAMPLER_DESC desc =
                {
                    .Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
                    .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
                    .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
                    .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
                };

                device->lpVtbl->CreateSamplerState(device,&desc,&sampler);
            }

            {
                // enable alpha blending
                D3D11_BLEND_DESC desc =
                {
                    .RenderTarget[0] =
                    {
                        .BlendEnable = 1,
                        .SrcBlend = D3D11_BLEND_SRC_ALPHA,
                        .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
                        .BlendOp = D3D11_BLEND_OP_ADD,
                        .SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA,
                        .DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
                        .BlendOpAlpha = D3D11_BLEND_OP_ADD,
                        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
                },
                };
                device->lpVtbl->CreateBlendState(device,&desc,&blendState);
            }

            {
                // disable culling
                D3D11_RASTERIZER_DESC desc =
                {
                    .FillMode = D3D11_FILL_SOLID,
                    .CullMode = D3D11_CULL_NONE,
                };
                device->lpVtbl->CreateRasterizerState(device,&desc,&rasterizerState);
            }

            {
                // disable depth & stencil test
                D3D11_DEPTH_STENCIL_DESC desc =
                {
                    .DepthEnable = FALSE,
                    .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
                    .DepthFunc = D3D11_COMPARISON_LESS,
                    .StencilEnable = FALSE,
                    .StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK,
                    .StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK,
                    // .FrontFace = ... 
                    // .BackFace = ...
                };
                device->lpVtbl->CreateDepthStencilState(device,&desc,&depthStencilState);
            }

            CreateRenderTargets();

            
            {
                // these must match vertex shader input layout (VS_INPUT in vertex shader source below)
                D3D11_INPUT_ELEMENT_DESC desc[] =
                {
                    {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, texcoord), D3D11_INPUT_PER_VERTEX_DATA, 0},
                };

        #define STR2(x) #x
        #define STR(x) STR2(x)
                const char hlsl[] =
                    "#line " STR(__LINE__) "                                  \n\n" // actual line number in this file for nicer error messages
                    "                                                           \n"
                    "struct VS_INPUT                                            \n"
                    "{                                                          \n"
                    "     float2 pos   : POSITION;                              \n" // these names must match D3D11_INPUT_ELEMENT_DESC array
                    "     float2 uv    : TEXCOORD;                              \n"
                    "};                                                         \n"
                    "                                                           \n"
                    "struct PS_INPUT                                            \n"
                    "{                                                          \n"
                    "  float4 pos   : SV_POSITION;                              \n" // these names do not matter, except SV_... ones
                    "  float2 uv    : TEXCOORD;                                 \n"
                    "};                                                         \n"
                    "                                                           \n"
                    "sampler sampler0 : register(s0);                           \n" // s0 = sampler bound to slot 0
                    "                                                           \n"
                    "Texture2D<float4> texture0 : register(t0);                 \n" // t0 = shader resource bound to slot 0
                    "                                                           \n"
                    "PS_INPUT vs(VS_INPUT input)                                \n"
                    "{                                                          \n"
                    "    PS_INPUT output;                                       \n"
                    "    output.pos = float4(input.pos, 0, 1);                  \n"
                    "    output.uv = input.uv;                                  \n"
                    "    return output;                                         \n"
                    "}                                                          \n"
                    "                                                           \n"
                    "float4 ps(PS_INPUT input) : SV_TARGET                      \n"
                    "{                                                          \n"
                    "    return texture0.Sample(sampler0, input.uv);            \n"
                    "}                                                          \n";
                ;

                UINT flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
        #if _DEBUG
                flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        #else
                flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
        #endif

                ID3DBlob* error;

                ID3DBlob* vblob;
                HRESULT hr = D3DCompile(hlsl, sizeof(hlsl), 0, 0, 0, "vs", "vs_5_0", flags, 0, &vblob, &error);
                if (FAILED(hr))
                {
                    char *message = error->lpVtbl->GetBufferPointer(error);
                    fatal_error(message);
                }

                ID3DBlob* pblob;
                hr = D3DCompile(hlsl, sizeof(hlsl), 0, 0, 0, "ps", "ps_5_0", flags, 0, &pblob, &error);
                if (FAILED(hr))
                {
                    char *message = error->lpVtbl->GetBufferPointer(error);
                    fatal_error(message);
                }

                device->lpVtbl->CreateVertexShader(device,vblob->lpVtbl->GetBufferPointer(vblob),vblob->lpVtbl->GetBufferSize(vblob),0,&vshader);
                device->lpVtbl->CreatePixelShader(device,pblob->lpVtbl->GetBufferPointer(pblob),pblob->lpVtbl->GetBufferSize(pblob),0,&pshader);
                device->lpVtbl->CreateInputLayout(device,desc,ARRAYSIZE(desc),vblob->lpVtbl->GetBufferPointer(vblob),vblob->lpVtbl->GetBufferSize(vblob),&layout);

                pblob->lpVtbl->Release(pblob);
                vblob->lpVtbl->Release(vblob);
            }
            break;
        }
        case WM_PAINT:{
            QueryPerformanceCounter(&t1);
            update((double)(t1.QuadPart-tstart.QuadPart) / (double)freq.QuadPart, (double)(t1.QuadPart-t0.QuadPart) / (double)freq.QuadPart);
            t0 = t1;

            deviceContext->lpVtbl->OMSetRenderTargets(deviceContext, 1, &renderTargetView, depthStencilView);
            FLOAT clearColor[4] = {0.1f, 0.2f, 0.6f, 1.0f};
            deviceContext->lpVtbl->ClearRenderTargetView(deviceContext, renderTargetView, clearColor);
            deviceContext->lpVtbl->ClearDepthStencilView(deviceContext, depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

            {
                ID3D11Buffer* vbuffer;
                Vertex data[] =
                {
                    { {-1.0f, 1.0f }, { 0.0f, 1.0f } },
                    { {-1.0f, -1.0f }, {  0.0f,  0.0f }},
                    { { 1.0f, -1.0f }, { 1.0f,  0.0f }},

                    { {1.0f, -1.0f}, { 1.0f, 0.0f }},
                    { {1.0f, 1.0f}, {  1.0f,  1.0f }},
                    { {-1.0f, 1.0f}, { 0.0f,  1.0f }},
                };

                D3D11_BUFFER_DESC desc =
                {
                    .ByteWidth = sizeof(data),
                    .Usage = D3D11_USAGE_IMMUTABLE,
                    .BindFlags = D3D11_BIND_VERTEX_BUFFER,
                };

                D3D11_SUBRESOURCE_DATA initial = { .pSysMem = data };
                device->lpVtbl->CreateBuffer(device,&desc,&initial,&vbuffer);

                deviceContext->lpVtbl->IASetInputLayout(deviceContext,layout);
                deviceContext->lpVtbl->IASetPrimitiveTopology(deviceContext,D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                UINT stride = sizeof(Vertex);
                UINT offset = 0;
                deviceContext->lpVtbl->IASetVertexBuffers(deviceContext,0,1,&vbuffer,&stride,&offset);

                deviceContext->lpVtbl->VSSetShader(deviceContext,vshader,0,0);

                RECT cr;
                GetClientRect(hwnd,&cr);
                int cwidth = cr.right-cr.left;
                int cheight = cr.bottom-cr.top;
                int scale = 1;
                while (SCREEN_WIDTH*scale <= cwidth && SCREEN_HEIGHT*scale <= cheight){
                    scale++;
                }
                scale--;
                int scaledWidth = scale * SCREEN_WIDTH;
                int scaledHeight = scale * SCREEN_HEIGHT;
                D3D11_VIEWPORT viewport = {
                    .TopLeftX = (FLOAT)(cwidth/2-scaledWidth/2),
                    .TopLeftY = (FLOAT)(cheight/2-scaledHeight/2),
                    .Width = (FLOAT)scaledWidth,
                    .Height = (FLOAT)scaledHeight,
                    .MinDepth = 0,
                    .MaxDepth = 1,
                };
                deviceContext->lpVtbl->RSSetViewports(deviceContext,1,&viewport);
                deviceContext->lpVtbl->RSSetState(deviceContext,rasterizerState);

                deviceContext->lpVtbl->PSSetSamplers(deviceContext,0,1,&sampler);
                deviceContext->lpVtbl->UpdateSubresource(deviceContext,(ID3D11Resource *)texture,0,0,screen,SCREEN_WIDTH*sizeof(*screen),0);
                deviceContext->lpVtbl->PSSetShaderResources(deviceContext,0,1,&textureView);
                deviceContext->lpVtbl->PSSetShader(deviceContext,pshader,0,0);

                //deviceContext->lpVtbl->OMSetBlendState(deviceContext,blendState,0,~0U);
                deviceContext->lpVtbl->OMSetDepthStencilState(deviceContext,depthStencilState,0);

                deviceContext->lpVtbl->Draw(deviceContext,6,0);

                vbuffer->lpVtbl->Release(vbuffer); //TODO: find out if this is shit
            }

            swapChain->lpVtbl->Present(swapChain, 1, 0);
            return 0;
        }
        case WM_DESTROY:{
            PostQuitMessage(0);
            break;
        }
        case WM_SIZE:{
            deviceContext->lpVtbl->OMSetRenderTargets(deviceContext,0,0,0);
            renderTargetView->lpVtbl->Release(renderTargetView);
            depthStencilView->lpVtbl->Release(depthStencilView);
            ASSERT(SUCCEEDED(swapChain->lpVtbl->ResizeBuffers(swapChain,0,0,0,DXGI_FORMAT_UNKNOWN,0)));
            CreateRenderTargets();
            break;
        }
        case WM_KEYDOWN:{
            if (!(HIWORD(lparam) & KF_REPEAT)){
                keydown((int)wparam);
            }
            break;
        }
        case WM_KEYUP:{
            keyup((int)wparam);
            break;
        }
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}
void open_window(int scale){
    WNDCLASSEXW wcex = {
        .cbSize = sizeof(wcex),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = WndProc,
        .hInstance = GetModuleHandleW(0),
        //.hIcon = LoadIconW(GetModuleHandleW(0),MAKEINTRESOURCEW(RID_ICON)),
        .hCursor = LoadCursorW(0,IDC_ARROW),
        .lpszClassName = L"tiny3d",
        .hIconSm = 0,
    };
    ASSERT(RegisterClassExW(&wcex));

    HWND hwnd;
    ASSERT(scale >= 0);
    if (scale > 0){

        RECT initialRect = {0, 0, scale*SCREEN_WIDTH, scale*SCREEN_HEIGHT};
        AdjustWindowRect(&initialRect,WS_OVERLAPPEDWINDOW,FALSE);
        LONG initialWidth = initialRect.right - initialRect.left;
        LONG initialHeight = initialRect.bottom - initialRect.top;

        hwnd = CreateWindowExW(
            0, //WS_EX_OVERLAPPEDWINDOW fucks up the borders when maximized
            wcex.lpszClassName,
            wcex.lpszClassName,
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            GetSystemMetrics(SM_CXSCREEN)/2-initialWidth/2,
            GetSystemMetrics(SM_CYSCREEN)/2-initialHeight/2,
            initialWidth, 
            initialHeight,
            0, 0, wcex.hInstance, 0
        );
    } else {
        hwnd = CreateWindowExW(
            0, //WS_EX_OVERLAPPEDWINDOW fucks up the borders when maximized
            wcex.lpszClassName,
            wcex.lpszClassName,
            WS_POPUP | WS_VISIBLE,
            0,
            0,
            GetSystemMetrics(SM_CXSCREEN), 
            GetSystemMetrics(SM_CYSCREEN),
            0, 0, wcex.hInstance, 0
        );
    }
    ASSERT(hwnd);

    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&tstart);
    t0 = tstart;

    MSG msg;
    while (GetMessageW(&msg,0,0,0)){
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}