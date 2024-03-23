#include "tiny3d.h"

uint32_t screen[SCREEN_WIDTH*SCREEN_HEIGHT];
char *assertPath;

void fatal_error(char *format, ...){
	va_list args;
	va_start(args,format);

	static char msg[4096];
	vsnprintf(msg,COUNT(msg),format,args);
	fprintf(stderr,"%s\n",msg);
	error_box(msg);

	va_end(args);
	
	exit(1);
}

static char fmtStr[4096];
char *format_string(char *format, ...){
	va_list args;
	va_start(args,format);

	int len = vsnprintf(fmtStr,COUNT(fmtStr),format,args);
	ASSERT(len > 0 && len < COUNT(fmtStr));

	va_end(args);

	return fmtStr;
}

static char root[4096];
static char *rootEnd = 0;
static char *local_path_to_absolute_internal(char *format, va_list args){
	int len;
	if (!rootEnd){
		len = wai_getExecutablePath(0,0,0);
		ASSERT(len < COUNT(root));
		ASSERT(0 < wai_getExecutablePath(root,len,0));
		root[len] = 0;

		rootEnd = strrchr(root,'/');
		if (!rootEnd){
			rootEnd = strrchr(root,'\\');
			ASSERT(rootEnd);
		}
		rootEnd++;
		rootEnd[0] = 0;
	}

	size_t remaining = root+COUNT(root)-rootEnd;
	len = vsnprintf(rootEnd,remaining,format,args);
	ASSERT(len > 0 && len < remaining);

	return root;
}

char *local_path_to_absolute(char *format, ...){
	va_list args;
	va_start(args,format);

	char *path = local_path_to_absolute_internal(format,args);

	va_end(args);

	return path;
}

FILE *fopen_relative(char *format, ...){
    va_list args;
	va_start(args,format);

	assertPath = local_path_to_absolute_internal(format,args);
	FILE *f = fopen(assertPath,"rb");
	ASSERT_FILE(f);

    va_end(args);

    return f;
}

unsigned char *load_file(int *size, char *format, ...){
	va_list args;
	va_start(args,format);

	assertPath = local_path_to_absolute_internal(format,args);
	FILE *f = fopen(assertPath,"rb");
	ASSERT_FILE(f);
	fseek(f,0,SEEK_END);
	long len = ftell(f);
	ASSERT_FILE(len > 0);
	fseek(f,0,SEEK_SET);
	char *buf = malloc(len);
	ASSERT_FILE(buf);
	fread(buf,1,len,f);
	fclose(f);

	va_end(args);

	*size = len;

	return buf;
}

char *load_file_as_cstring(char *format, ...){
	va_list args;
	va_start(args,format);

	assertPath = local_path_to_absolute_internal(format,args);
	FILE *f = fopen(assertPath,"rb");
	ASSERT_FILE(f);
	fseek(f,0,SEEK_END);
	long len = ftell(f);
	ASSERT_FILE(len > 0);
	fseek(f,0,SEEK_SET);
	char *str = malloc(len+1);
	ASSERT_FILE(str);
	fread(str,1,len,f);
	str[len] = 0;
	fclose(f);

	va_end(args);

	return str;
}

void clear_screen(uint32_t color){
  	for (int i = 0; i < SCREEN_WIDTH*SCREEN_HEIGHT; i++){
		screen[i] = color;
	}
}

void draw_line(int x0, int y0, int x1, int y1, uint32_t color){
	int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
	int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
	int err = (dx>dy ? dx : -dy)/2, e2;
	while (1){
		screen[y0*SCREEN_WIDTH+x0] = color;
		if (x0==x1 && y0==y1) break;
		e2 = err;
		if (e2 >-dx) { err -= dy; x0 += sx; }
		if (e2 < dy) { err += dx; y0 += sy; }
	}
}

#ifdef _WIN32
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
#elif __unix__ // all unices, not all compilers
	#include<X11/X.h>
    #include<X11/Xlib.h>

    #include<GL/gl.h>
    #include<GL/glx.h>

    //Messagebox code from https://github.com/Eleobert/MessageBox-X11/tree/master
    #include <X11/Xatom.h>
    #include <X11/Xutil.h>
    #include <locale.h>
    #include <wchar.h>
    #include <stdbool.h>
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
    #include <time.h>

    struct Dimensions{
        //window
        unsigned int winMinWidth;
        unsigned int winMinHeight;
        //vertical space between lines
        unsigned int lineSpacing;
        unsigned int barHeight;
        //padding
        unsigned int pad_up;
        unsigned int pad_down;
        unsigned int pad_left;
        unsigned int pad_right;
        //button
        unsigned int btSpacing;
        unsigned int btMinWidth;
        unsigned int btMinHeight;
        unsigned int btLateralPad;

    };

    typedef struct Button{
        wchar_t *label;
        int result;
    }Button;

    typedef struct ButtonData{ //<----see static
        const Button* button;
        GC *gc;
        XRectangle rect;
    }ButtonData;

    //these values can be changed to whatever you prefer
    struct Dimensions dim = {400, 0, 5, 50, 30, 10, 30, 30, 20, 75, 25, 8};


    void setWindowTitle(const char* title, const Window *win, Display *dpy){
        Atom wm_Name = XInternAtom(dpy, "_NET_WM_NAME", False);
        Atom utf8Str = XInternAtom(dpy, "UTF8_STRING", False);

        Atom winType = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False );
        Atom typeDialog = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False );

        XChangeProperty(dpy, *win, wm_Name, utf8Str, 8, PropModeReplace, (const unsigned char*)title, (int)strlen(title));

        XChangeProperty( dpy,*win, winType, XA_ATOM,
                        32, PropModeReplace, (unsigned char *)&typeDialog,
                        1);
    }

    void split(const wchar_t* text, const wchar_t* seps, wchar_t ***str, int *count){
        wchar_t *last, *tok, *data;
        int i;
        *count = 0;
        data = wcsdup(text);

        for (tok = wcstok(data, seps, &last); tok != NULL; tok = wcstok(NULL, seps, &last))
            (*count)++;

        free(data);
        fflush(stdout);
        data = wcsdup(text);
        *str = (wchar_t **)malloc((size_t)(*count)*sizeof(wchar_t*));


        for (i = 0, tok = wcstok(data, seps, &last); tok != NULL; tok = wcstok(NULL, seps, &last), i++)
            (*str)[i] = wcsdup(tok);
        free(data);
    }

    void computeTextSize(XFontSet* fs, wchar_t** texts, int size, unsigned int spaceBetweenLines,
                        unsigned int *w,  unsigned  int *h)
    {
        int i;
        XRectangle rect  = {0,0,0,0};
        *h = 0;
        *w = 0;
        for(i = 0; i < size; i++){
            XwcTextExtents(*fs, texts[i], (int)wcslen(texts[i]), &rect, NULL);
            *w = (rect.width > *w) ? (rect.width): *w;
            *h += rect.height + spaceBetweenLines;
            fflush(stdin);
        }
    }

    void createGC(GC* gc, const Colormap* cmap, Display* dpy, const  Window* win,
                unsigned char red, unsigned char green, unsigned char blue){
        float coloratio = (float) 65535 / 255;
        XColor color;
        *gc = XCreateGC (dpy, *win, 0,0);
        memset(&color, 0, sizeof(color));
        color.red   = (unsigned short)(coloratio * red  );
        color.green = (unsigned short)(coloratio * green);
        color.blue  = (unsigned short)(coloratio * blue );
        color.flags = DoRed | DoGreen | DoBlue;
        XAllocColor (dpy, *cmap, &color);
        XSetForeground (dpy, *gc, color.pixel);
    }

    bool isInside(int x, int y, XRectangle rect){
        if(x < rect.x || x > (rect.x + rect.width) || y < rect.y || y > (rect.y + rect.height))
            return false;
        return true;
    }

    int Messagebox(const char* title, const wchar_t* text, const Button* buttons, int numButtons)
    {
        setlocale(LC_ALL,"");

        //---- convert the text in list (to draw in multiply lines)---------------------------------------------------------
        wchar_t** text_splitted = NULL;
        int textLines = 0;
        split(text, L"\n" , &text_splitted, &textLines);
        //------------------------------------------------------------------------------------------------------------------

        Display* dpy = NULL;
        dpy = XOpenDisplay(0);
        if(dpy == NULL){
            fprintf(stderr, "Error opening display display.");
        }

        int ds = DefaultScreen(dpy);
        Window win = XCreateSimpleWindow(dpy, RootWindow(dpy, ds), 0, 0, 800, 100, 1,
                                        BlackPixel(dpy, ds), WhitePixel(dpy, ds));


        XSelectInput(dpy, win, ExposureMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask);
        XMapWindow(dpy, win);

        //allow windows to be closed by pressing cross button (but it wont close - see ClientMessage on switch)
        Atom WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(dpy, win, &WM_DELETE_WINDOW, 1);

        //--create the gc for drawing text----------------------------------------------------------------------------------
        XGCValues gcValues;
        gcValues.font = XLoadFont(dpy, "7x13");
        gcValues.foreground = BlackPixel(dpy, 0);
        GC textGC = XCreateGC(dpy, win, GCFont + GCForeground, &gcValues);
        XUnmapWindow( dpy, win );
        //------------------------------------------------------------------------------------------------------------------

        //----create fontset-----------------------------------------------------------------------------------------------
        char **missingCharset_list = NULL;
        int missingCharset_count = 0;
        XFontSet fs;
        fs = XCreateFontSet(dpy,
                            "-*-*-medium-r-*-*-*-140-75-75-*-*-*-*" ,
                            &missingCharset_list, &missingCharset_count, NULL);

        if (missingCharset_count) {
            fprintf(stderr, "Missing charsets :\n");
            for(int i = 0; i < missingCharset_count; i++){
                fprintf(stderr, "%s\n", missingCharset_list[i]);
            }
            XFreeStringList(missingCharset_list);
            missingCharset_list = NULL;
        }
        //------------------------------------------------------------------------------------------------------------------

        Colormap cmap = DefaultColormap (dpy, ds);

        //resize the window according to the text size-----------------------------------------------------------------------
        unsigned int winW, winH;
        unsigned int textW, textH;

        //calculate the ideal window's size
        computeTextSize(&fs, text_splitted, textLines, dim.lineSpacing, &textW, &textH);
        unsigned int newWidth = textW + dim.pad_left + dim.pad_right;
        unsigned int newHeight = textH + dim.pad_up + dim.pad_down + dim.barHeight;
        winW = (newWidth > dim.winMinWidth)? newWidth: dim.winMinWidth;
        winH = (newHeight > dim.winMinHeight)? newHeight: dim.winMinHeight;

        //set windows hints
        XSizeHints hints;
        hints.flags      = PSize | PMinSize | PMaxSize;
        hints.min_width  = hints.max_width  = hints.base_width  = winW;
        hints.min_height = hints.max_height = hints.base_height = winH;

        XSetWMNormalHints( dpy, win, &hints );
        XMapRaised( dpy, win );

        int swidth = XDisplayWidth(dpy,ds);
        int sheight = XDisplayHeight(dpy,ds);
        XMoveWindow(dpy,win,swidth/2-winW/2,sheight/2-winH/2);
        //------------------------------------------------------------------------------------------------------------------

        GC barGC;
        GC buttonGC;
        GC buttonGC_underPointer;
        GC buttonGC_onClick;                               // GC colors
        createGC(&barGC, &cmap, dpy, &win,                 242, 242, 242);
        createGC(&buttonGC, &cmap, dpy, &win,              202, 202, 202);
        createGC(&buttonGC_underPointer, &cmap, dpy, &win, 192, 192, 192);
        createGC(&buttonGC_onClick, &cmap, dpy, &win,      182, 182, 182);

        //---setup the buttons data-----------------------------------------------------------------------------------------
        ButtonData *btsData;
        btsData = (ButtonData*)malloc((size_t)numButtons * sizeof(ButtonData));

        int pass = 0;
        for(int i = 0; i < numButtons; i++){
            btsData[i].button = &buttons[i];
            btsData[i].gc = &buttonGC;
            XRectangle btTextDim;
            XwcTextExtents(fs, btsData[i].button->label, (int)wcslen(btsData[i].button->label),
                        &btTextDim, NULL);
            btsData[i].rect.width = (btTextDim.width < dim.btMinWidth) ? dim.btMinWidth:
                                    (btTextDim.width + 2 * dim.btLateralPad);
            btsData[i].rect.height = dim.btMinHeight;
            btsData[i].rect.x = winW - dim.pad_left - btsData[i].rect.width - pass;
            btsData[i].rect.y = textH + dim.pad_up + dim.pad_down + ((dim.barHeight - dim.btMinHeight)/ 2) ;
            pass += btsData[i].rect.width + dim.btSpacing;
        }
        //------------------------------------------------------------------------------------------------------------------

        setWindowTitle(title, &win, dpy);

        XFlush( dpy );


        bool quit = false;
        int res = -1;

        while( !quit ) {
            XEvent e;
            XNextEvent( dpy, &e );

            switch (e.type){

                case MotionNotify:
                case ButtonPress:
                case ButtonRelease:
                    for(int i = 0; i < numButtons; i++) {
                        btsData[i].gc = &buttonGC;
                        if (isInside(e.xmotion.x, e.xmotion.y, btsData[i].rect)) {
                            btsData[i].gc = &buttonGC_underPointer;
                            if(e.type == ButtonPress && e.xbutton.button == Button1) {
                                btsData[i].gc = &buttonGC_onClick;
                                res = btsData[i].button->result;
                                quit = true;
                            }
                        }
                    }

                case Expose:
                    //draw the text in multiply lines----------------------------------------------------------------------
                    for(int i = 0; i < textLines; i++){

                        XwcDrawString( dpy, win, fs, textGC, dim.pad_left, dim.pad_up + i * (dim.lineSpacing + 18),
                                    text_splitted[i], (int)wcslen(text_splitted[i]));
                    }
                    //------------------------------------------------------------------------------------------------------
                    XFillRectangle(dpy, win, barGC, 0, textH + dim.pad_up + dim.pad_down, winW, dim.barHeight);

                    for(int i = 0; i < numButtons; i++){
                        XFillRectangle(dpy, win, *btsData[i].gc, btsData[i].rect.x, btsData[i].rect.y,
                                    btsData[i].rect.width, btsData[i].rect.height);

                        XRectangle btTextDim;
                        XwcTextExtents(fs, btsData[i].button->label, (int)wcslen(btsData[i].button->label),
                                    &btTextDim, NULL);
                        XwcDrawString( dpy, win, fs, textGC,
                                    btsData[i].rect.x + (btsData[i].rect.width  - btTextDim.width ) / 2,
                                    btsData[i].rect.y + (btsData[i].rect.height + btTextDim.height) / 2,
                                    btsData[i].button->label, (int)wcslen(btsData[i].button->label));
                    }
                    XFlush(dpy);

                    break;

                case ClientMessage:
                    //if window's cross button pressed does nothing
                    //if((unsigned int)(e.xclient.data.l[0]) == WM_DELETE_WINDOW)
                        //quit = true;
                    break;
                default:
                    break;
            }
        }

        for(int i = 0; i < textLines; i++){
            free(text_splitted[i]);
        }
        free(text_splitted);
        free(btsData);
        if (missingCharset_list)
            XFreeStringList(missingCharset_list);
        XDestroyWindow(dpy, win);
        XFreeFontSet(dpy, fs);
        XFreeGC(dpy, textGC);
        XFreeGC(dpy, barGC);
        XFreeGC(dpy, buttonGC);
        XFreeGC(dpy, buttonGC_underPointer);
        XFreeGC(dpy, buttonGC_onClick);
        XFreeColormap(dpy, cmap);
        XCloseDisplay(dpy);

        return res;
    }

	void error_box(char *msg){
        Button button = {
            .label = L"Ok",
            .result = 0
        };
        size_t len = strlen(msg);
        wchar_t *w = malloc((len+1)*sizeof(*w));
        for (size_t i = 0; i < len; i++){
            w[i] = msg[i];
        }
        w[len] = 0;
        Messagebox("Error",w,&button,1);
        free(w);
    }

    uint64_t get_time(void){
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t) ts.tv_sec * 1000000000 + (uint64_t) ts.tv_nsec;
    }

	void open_window(int width, int height, int fbWidth, int fbHeight, uint32_t *framebuffer){
		Display *display = XOpenDisplay(NULL);
        int screen = DefaultScreen(display);
        int swidth = XDisplayWidth(display,screen);
        int sheight = XDisplayHeight(display,screen);
        Window window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, width, height, 0, 0, 0);
        XStoreName(display, window, "tiny3d");
        XSelectInput(display, window, KeyPressMask | KeyReleaseMask);
        int visual_hints[] = {
            GLX_RGBA,
            GLX_DEPTH_SIZE, 24,
            GLX_DOUBLEBUFFER,
            None
        };
        XVisualInfo *visual_info = glXChooseVisual(display, screen, visual_hints);
        GLXContext context = glXCreateContext(display, visual_info, NULL, True);
        glXMakeCurrent(display, window, context);
        typedef void (*PFNGLXSWAPINTERVALEXTPROC)(Display *, GLXDrawable, int);
        PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddress("glXSwapIntervalEXT");
        ASSERT(glXSwapIntervalEXT);
        glXSwapIntervalEXT(display,window,1);

        XMapWindow(display, window);
        XMoveWindow(display, window, swidth/2-width/2, sheight/2-height/2);

        GLuint texture;
        glGenTextures(1,&texture);
        glBindTexture(GL_TEXTURE_2D,texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        uint64_t tstart, t0, t1;
        tstart = get_time();
        t0 = tstart;
        
        for(;;)
        {
            XEvent event;
            while(XPending(display))
            {
                XNextEvent(display, &event);
                if(event.type == KeyPress)
                {
                    switch(XKeycodeToKeysym(display, event.xkey.keycode, 0))
                    {
                        case XK_Escape: return;
                    }
                }
            }

            t1 = get_time();
            update((double)(t1-tstart) / 1000000000.0, (double)(t1-t0) / 1000000000.0);
            t0 = t1;

            XWindowAttributes attribs;
            XGetWindowAttributes(display, window, &attribs);

            glClearColor(0.0f,0.0f,1.0f,1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fbWidth, fbHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, framebuffer);
            int scale = 1;
            while (fbWidth*scale <= attribs.width && fbHeight*scale <= attribs.height){
                scale++;
            }
            scale--;
            int scaledWidth = scale * fbWidth;
            int scaledHeight = scale * fbHeight;
            int x = attribs.width/2-scaledWidth/2;
            int y = attribs.height/2-scaledHeight/2;
            glViewport(x,y,scaledWidth,scaledHeight);
            glEnable(GL_TEXTURE_2D);
            glBegin(GL_QUADS);
            glTexCoord2f(0,0); glVertex2f(-1,-1);
            glTexCoord2f(1,0); glVertex2f(1,-1);
            glTexCoord2f(1,1); glVertex2f(1,1);
            glTexCoord2f(0,1); glVertex2f(-1,1);
            glEnd();

            glXSwapBuffers(display, window);
        }
	}
#elif __APPLE__
    // Mac OS, not sure if this is covered by __posix__ and/or __unix__ though...
#endif

// (‑●‑●)> dual licensed under the WTFPL v2 and MIT licenses
//   without any warranty.
//   by Gregory Pakosz (@gpakosz)
// https://github.com/gpakosz/whereami

// in case you want to #include "whereami.c" in a larger compilation unit

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__linux__) || defined(__CYGWIN__)
#undef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#elif defined(__APPLE__)
#undef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE
#define _DARWIN_BETTER_REALPATH
#endif

#if !defined(WAI_MALLOC) || !defined(WAI_FREE) || !defined(WAI_REALLOC)
#include <stdlib.h>
#endif

#if !defined(WAI_MALLOC)
#define WAI_MALLOC(size) malloc(size)
#endif

#if !defined(WAI_FREE)
#define WAI_FREE(p) free(p)
#endif

#if !defined(WAI_REALLOC)
#define WAI_REALLOC(p, size) realloc(p, size)
#endif

#ifndef WAI_NOINLINE
#if defined(_MSC_VER)
#define WAI_NOINLINE __declspec(noinline)
#elif defined(__GNUC__)
#define WAI_NOINLINE __attribute__((noinline))
#else
#error unsupported compiler
#endif
#endif

#if defined(_MSC_VER)
#define WAI_RETURN_ADDRESS() _ReturnAddress()
#elif defined(__GNUC__)
#define WAI_RETURN_ADDRESS() __builtin_extract_return_addr(__builtin_return_address(0))
#else
#error unsupported compiler
#endif

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#if defined(_MSC_VER)
#pragma warning(push, 3)
#endif
#include <windows.h>
#include <intrin.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#include <stdbool.h>

static int WAI_PREFIX(getModulePath_)(HMODULE module, char* out, int capacity, int* dirname_length)
{
  wchar_t buffer1[MAX_PATH];
  wchar_t buffer2[MAX_PATH];
  wchar_t* path = NULL;
  int length = -1;
  bool ok;

  for (ok = false; !ok; ok = true)
  {
    DWORD size;
    int length_, length__;

    size = GetModuleFileNameW(module, buffer1, sizeof(buffer1) / sizeof(buffer1[0]));

    if (size == 0)
      break;
    else if (size == (DWORD)(sizeof(buffer1) / sizeof(buffer1[0])))
    {
      DWORD size_ = size;
      do
      {
        wchar_t* path_;

        path_ = (wchar_t*)WAI_REALLOC(path, sizeof(wchar_t) * size_ * 2);
        if (!path_)
          break;
        size_ *= 2;
        path = path_;
        size = GetModuleFileNameW(module, path, size_);
      }
      while (size == size_);

      if (size == size_)
        break;
    }
    else
      path = buffer1;

    if (!_wfullpath(buffer2, path, MAX_PATH))
      break;
    length_ = (int)wcslen(buffer2);
    length__ = WideCharToMultiByte(CP_UTF8, 0, buffer2, length_ , out, capacity, NULL, NULL);

    if (length__ == 0)
      length__ = WideCharToMultiByte(CP_UTF8, 0, buffer2, length_, NULL, 0, NULL, NULL);
    if (length__ == 0)
      break;

    if (length__ <= capacity && dirname_length)
    {
      int i;

      for (i = length__ - 1; i >= 0; --i)
      {
        if (out[i] == '\\')
        {
          *dirname_length = i;
          break;
        }
      }
    }

    length = length__;
  }

  if (path != buffer1)
    WAI_FREE(path);

  return ok ? length : -1;
}

WAI_NOINLINE WAI_FUNCSPEC
int WAI_PREFIX(getExecutablePath)(char* out, int capacity, int* dirname_length)
{
  return WAI_PREFIX(getModulePath_)(NULL, out, capacity, dirname_length);
}

WAI_NOINLINE WAI_FUNCSPEC
int WAI_PREFIX(getModulePath)(char* out, int capacity, int* dirname_length)
{
  HMODULE module;
  int length = -1;

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4054)
#endif
  if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCTSTR)WAI_RETURN_ADDRESS(), &module))
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
  {
    length = WAI_PREFIX(getModulePath_)(module, out, capacity, dirname_length);
  }

  return length;
}

#elif defined(__linux__) || defined(__CYGWIN__) || defined(__sun) || defined(WAI_USE_PROC_SELF_EXE)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__)
#include <linux/limits.h>
#else
#include <limits.h>
#endif
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <stdbool.h>

#if !defined(WAI_PROC_SELF_EXE)
#if defined(__sun)
#define WAI_PROC_SELF_EXE "/proc/self/path/a.out"
#else
#define WAI_PROC_SELF_EXE "/proc/self/exe"
#endif
#endif

WAI_FUNCSPEC
int WAI_PREFIX(getExecutablePath)(char* out, int capacity, int* dirname_length)
{
  char buffer[PATH_MAX];
  char* resolved = NULL;
  int length = -1;
  bool ok;

  for (ok = false; !ok; ok = true)
  {
    resolved = realpath(WAI_PROC_SELF_EXE, buffer);
    if (!resolved)
      break;

    length = (int)strlen(resolved);
    if (length <= capacity)
    {
      memcpy(out, resolved, length);

      if (dirname_length)
      {
        int i;

        for (i = length - 1; i >= 0; --i)
        {
          if (out[i] == '/')
          {
            *dirname_length = i;
            break;
          }
        }
      }
    }
  }

  return ok ? length : -1;
}

#if !defined(WAI_PROC_SELF_MAPS_RETRY)
#define WAI_PROC_SELF_MAPS_RETRY 5
#endif

#if !defined(WAI_PROC_SELF_MAPS)
#if defined(__sun)
#define WAI_PROC_SELF_MAPS "/proc/self/map"
#else
#define WAI_PROC_SELF_MAPS "/proc/self/maps"
#endif
#endif

#if defined(__ANDROID__) || defined(ANDROID)
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif
#include <stdbool.h>

WAI_NOINLINE WAI_FUNCSPEC
int WAI_PREFIX(getModulePath)(char* out, int capacity, int* dirname_length)
{
  int length = -1;
  FILE* maps = NULL;

  for (int r = 0; r < WAI_PROC_SELF_MAPS_RETRY; ++r)
  {
    maps = fopen(WAI_PROC_SELF_MAPS, "r");
    if (!maps)
      break;

    for (;;)
    {
      char buffer[PATH_MAX < 1024 ? 1024 : PATH_MAX];
      uint64_t low, high;
      char perms[5];
      uint64_t offset;
      uint32_t major, minor;
      char path[PATH_MAX];
      uint32_t inode;

      if (!fgets(buffer, sizeof(buffer), maps))
        break;

      if (sscanf(buffer, "%" PRIx64 "-%" PRIx64 " %s %" PRIx64 " %x:%x %u %s\n", &low, &high, perms, &offset, &major, &minor, &inode, path) == 8)
      {
        uint64_t addr = (uintptr_t)WAI_RETURN_ADDRESS();
        if (low <= addr && addr <= high)
        {
          char* resolved;

          resolved = realpath(path, buffer);
          if (!resolved)
            break;

          length = (int)strlen(resolved);
#if defined(__ANDROID__) || defined(ANDROID)
          if (length > 4
              &&buffer[length - 1] == 'k'
              &&buffer[length - 2] == 'p'
              &&buffer[length - 3] == 'a'
              &&buffer[length - 4] == '.')
          {
            int fd = open(path, O_RDONLY);
            if (fd == -1)
            {
              length = -1; // retry
              break;
            }

            char* begin = (char*)mmap(0, offset, PROT_READ, MAP_SHARED, fd, 0);
            if (begin == MAP_FAILED)
            {
              close(fd);
              length = -1; // retry
              break;
            }

            char* p = begin + offset - 30; // minimum size of local file header
            while (p >= begin) // scan backwards
            {
              if (*((uint32_t*)p) == 0x04034b50UL) // local file header signature found
              {
                uint16_t length_ = *((uint16_t*)(p + 26));

                if (length + 2 + length_ < (int)sizeof(buffer))
                {
                  memcpy(&buffer[length], "!/", 2);
                  memcpy(&buffer[length + 2], p + 30, length_);
                  length += 2 + length_;
                }

                break;
              }

              --p;
            }

            munmap(begin, offset);
            close(fd);
          }
#endif
          if (length <= capacity)
          {
            memcpy(out, resolved, length);

            if (dirname_length)
            {
              int i;

              for (i = length - 1; i >= 0; --i)
              {
                if (out[i] == '/')
                {
                  *dirname_length = i;
                  break;
                }
              }
            }
          }

          break;
        }
      }
    }

    fclose(maps);
    maps = NULL;

    if (length != -1)
      break;
  }

  return length;
}

#elif defined(__APPLE__)

#include <mach-o/dyld.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdbool.h>

WAI_FUNCSPEC
int WAI_PREFIX(getExecutablePath)(char* out, int capacity, int* dirname_length)
{
  char buffer1[PATH_MAX];
  char buffer2[PATH_MAX];
  char* path = buffer1;
  char* resolved = NULL;
  int length = -1;
  bool ok;

  for (ok = false; !ok; ok = true)
  {
    uint32_t size = (uint32_t)sizeof(buffer1);
    if (_NSGetExecutablePath(path, &size) == -1)
    {
      path = (char*)WAI_MALLOC(size);
      if (!_NSGetExecutablePath(path, &size))
        break;
    }

    resolved = realpath(path, buffer2);
    if (!resolved)
      break;

    length = (int)strlen(resolved);
    if (length <= capacity)
    {
      memcpy(out, resolved, length);

      if (dirname_length)
      {
        int i;

        for (i = length - 1; i >= 0; --i)
        {
          if (out[i] == '/')
          {
            *dirname_length = i;
            break;
          }
        }
      }
    }
  }

  if (path != buffer1)
    WAI_FREE(path);

  return ok ? length : -1;
}

WAI_NOINLINE WAI_FUNCSPEC
int WAI_PREFIX(getModulePath)(char* out, int capacity, int* dirname_length)
{
  char buffer[PATH_MAX];
  char* resolved = NULL;
  int length = -1;

  for(;;)
  {
    Dl_info info;

    if (dladdr(WAI_RETURN_ADDRESS(), &info))
    {
      resolved = realpath(info.dli_fname, buffer);
      if (!resolved)
        break;

      length = (int)strlen(resolved);
      if (length <= capacity)
      {
        memcpy(out, resolved, length);

        if (dirname_length)
        {
          int i;

          for (i = length - 1; i >= 0; --i)
          {
            if (out[i] == '/')
            {
              *dirname_length = i;
              break;
            }
          }
        }
      }
    }

    break;
  }

  return length;
}

#elif defined(__QNXNTO__)

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdbool.h>

#if !defined(WAI_PROC_SELF_EXE)
#define WAI_PROC_SELF_EXE "/proc/self/exefile"
#endif

WAI_FUNCSPEC
int WAI_PREFIX(getExecutablePath)(char* out, int capacity, int* dirname_length)
{
  char buffer1[PATH_MAX];
  char buffer2[PATH_MAX];
  char* resolved = NULL;
  FILE* self_exe = NULL;
  int length = -1;
  bool ok;

  for (ok = false; !ok; ok = true)
  {
    self_exe = fopen(WAI_PROC_SELF_EXE, "r");
    if (!self_exe)
      break;

    if (!fgets(buffer1, sizeof(buffer1), self_exe))
      break;

    resolved = realpath(buffer1, buffer2);
    if (!resolved)
      break;

    length = (int)strlen(resolved);
    if (length <= capacity)
    {
      memcpy(out, resolved, length);

      if (dirname_length)
      {
        int i;

        for (i = length - 1; i >= 0; --i)
        {
          if (out[i] == '/')
          {
            *dirname_length = i;
            break;
          }
        }
      }
    }
  }

  fclose(self_exe);

  return ok ? length : -1;
}

WAI_FUNCSPEC
int WAI_PREFIX(getModulePath)(char* out, int capacity, int* dirname_length)
{
  char buffer[PATH_MAX];
  char* resolved = NULL;
  int length = -1;

  for(;;)
  {
    Dl_info info;

    if (dladdr(WAI_RETURN_ADDRESS(), &info))
    {
      resolved = realpath(info.dli_fname, buffer);
      if (!resolved)
        break;

      length = (int)strlen(resolved);
      if (length <= capacity)
      {
        memcpy(out, resolved, length);

        if (dirname_length)
        {
          int i;

          for (i = length - 1; i >= 0; --i)
          {
            if (out[i] == '/')
            {
              *dirname_length = i;
              break;
            }
          }
        }
      }
    }

    break;
  }

  return length;
}

#elif defined(__DragonFly__) || defined(__FreeBSD__) || \
      defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__OpenBSD__)

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <dlfcn.h>
#include <stdbool.h>

#if defined(__OpenBSD__)

#include <unistd.h>

WAI_FUNCSPEC
int WAI_PREFIX(getExecutablePath)(char* out, int capacity, int* dirname_length)
{
  char buffer1[4096];
  char buffer2[PATH_MAX];
  char buffer3[PATH_MAX];
  char** argv = (char**)buffer1;
  char* resolved = NULL;
  int length = -1;
  bool ok;

  for (ok = false; !ok; ok = true)
  {
    int mib[4] = { CTL_KERN, KERN_PROC_ARGS, getpid(), KERN_PROC_ARGV };
    size_t size;

    if (sysctl(mib, 4, NULL, &size, NULL, 0) != 0)
        break;

    if (size > sizeof(buffer1))
    {
      argv = (char**)WAI_MALLOC(size);
      if (!argv)
        break;
    }

    if (sysctl(mib, 4, argv, &size, NULL, 0) != 0)
        break;

    if (strchr(argv[0], '/'))
    {
      resolved = realpath(argv[0], buffer2);
      if (!resolved)
        break;
    }
    else
    {
      const char* PATH = getenv("PATH");
      if (!PATH)
        break;

      size_t argv0_length = strlen(argv[0]);

      const char* begin = PATH;
      while (1)
      {
        const char* separator = strchr(begin, ':');
        const char* end = separator ? separator : begin + strlen(begin);

        if (end - begin > 0)
        {
          if (*(end -1) == '/')
            --end;

          if (((end - begin) + 1 + argv0_length + 1) <= sizeof(buffer2))
          {
            memcpy(buffer2, begin, end - begin);
            buffer2[end - begin] = '/';
            memcpy(buffer2 + (end - begin) + 1, argv[0], argv0_length + 1);

            resolved = realpath(buffer2, buffer3);
            if (resolved)
              break;
          }
        }

        if (!separator)
          break;

        begin = ++separator;
      }

      if (!resolved)
        break;
    }

    length = (int)strlen(resolved);
    if (length <= capacity)
    {
      memcpy(out, resolved, length);

      if (dirname_length)
      {
        int i;

        for (i = length - 1; i >= 0; --i)
        {
          if (out[i] == '/')
          {
            *dirname_length = i;
            break;
          }
        }
      }
    }
  }

  if (argv != (char**)buffer1)
    WAI_FREE(argv);

  return ok ? length : -1;
}

#else

WAI_FUNCSPEC
int WAI_PREFIX(getExecutablePath)(char* out, int capacity, int* dirname_length)
{
  char buffer1[PATH_MAX];
  char buffer2[PATH_MAX];
  char* path = buffer1;
  char* resolved = NULL;
  int length = -1;
  bool ok;

  for (ok = false; !ok; ok = true)
  {
#if defined(__NetBSD__)
    int mib[4] = { CTL_KERN, KERN_PROC_ARGS, -1, KERN_PROC_PATHNAME };
#else
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
#endif
    size_t size = sizeof(buffer1);

    if (sysctl(mib, 4, path, &size, NULL, 0) != 0)
        break;

    resolved = realpath(path, buffer2);
    if (!resolved)
      break;

    length = (int)strlen(resolved);
    if (length <= capacity)
    {
      memcpy(out, resolved, length);

      if (dirname_length)
      {
        int i;

        for (i = length - 1; i >= 0; --i)
        {
          if (out[i] == '/')
          {
            *dirname_length = i;
            break;
          }
        }
      }
    }
  }

  return ok ? length : -1;
}

#endif

WAI_NOINLINE WAI_FUNCSPEC
int WAI_PREFIX(getModulePath)(char* out, int capacity, int* dirname_length)
{
  char buffer[PATH_MAX];
  char* resolved = NULL;
  int length = -1;

  for(;;)
  {
    Dl_info info;

    if (dladdr(WAI_RETURN_ADDRESS(), &info))
    {
      resolved = realpath(info.dli_fname, buffer);
      if (!resolved)
        break;

      length = (int)strlen(resolved);
      if (length <= capacity)
      {
        memcpy(out, resolved, length);

        if (dirname_length)
        {
          int i;

          for (i = length - 1; i >= 0; --i)
          {
            if (out[i] == '/')
            {
              *dirname_length = i;
              break;
            }
          }
        }
      }
    }

    break;
  }

  return length;
}

#else

#error unsupported platform

#endif

#ifdef __cplusplus
}
#endif
