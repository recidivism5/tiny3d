#include <tiny3d.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define COBJMACROS
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <wincodec.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

#include <vssym32.h>
#include <shellapi.h>
#include <shlobj_core.h>
#include <shobjidl_core.h>
#include <shlguid.h>
#include <commdlg.h>
#include <shlwapi.h>

#include <GL/gl.h>

#include <mmdeviceapi.h>
#include <audioclient.h>
static const GUID _CLSID_MMDeviceEnumerator = {0xbcde0395, 0xe52f, 0x467c, {0x8e,0x3d, 0xc4,0x57,0x92,0x91,0x69,0x2e}};
static const GUID _IID_IMMDeviceEnumerator = {0xa95664d2, 0x9614, 0x4f35, {0xa7,0x46, 0xde,0x8d,0xb6,0x36,0x17,0xe6}};
static const GUID _IID_IAudioClient = {0x1cb9ad4c, 0xdbfa, 0x4c32, {0xb1,0x78, 0xc2,0xf5,0x68,0xa7,0x03,0xb2}};
static const GUID _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT = {0x00000003, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
static const GUID _KSDATAFORMAT_SUBTYPE_PCM = {0x00000001,0x0000,0x0010,{0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
static const GUID _IID_IAudioRenderClient = {0xf294acfc, 0x3146, 0x4483, {0xa7, 0xbf, 0xad, 0xdc, 0xa7, 0xc2, 0x60, 0xe2}};
static IMMDeviceEnumerator *enu = NULL;
static IMMDevice *dev = NULL;
static IAudioClient *client = NULL;
static IAudioRenderClient* renderClient = NULL;

static bool coInitCalled = false;
static bool mfStartupCalled = false;
void safe_coinit(void){
    if (!coInitCalled){
        ASSERT_FILE(SUCCEEDED(CoInitialize(0)));
        coInitCalled = true;
    }
}
void safe_mfstartup(void){
    if (!mfStartupCalled){
        ASSERT_FILE(SUCCEEDED(MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET)));
        mfStartupCalled = true;
    }
}

void error_box(char *msg){
    MessageBoxA(0,msg,"Error",MB_ICONERROR);
}

uint32_t *load_image(bool flip_vertically, int *width, int *height, char *format, ...){
    va_list args;
    va_start(args,format);
    assertPath = local_path_to_absolute_vararg(format,args);

    safe_coinit();

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

int16_t *load_audio(int *nFrames, char *format, ...){
    va_list args;
    va_start(args,format);
    assertPath = local_path_to_absolute_vararg(format,args);

    IMFSourceReader *pReader = NULL;
    
    safe_coinit();

    safe_mfstartup();

    {
        size_t len = strlen(assertPath)+1;
        WCHAR *wpath = malloc(len*sizeof(*wpath));
        ASSERT(wpath);
        mbstowcs(wpath,assertPath,len);
        ASSERT_FILE(SUCCEEDED(MFCreateSourceReaderFromURL(wpath, NULL, &pReader)));
        free(wpath);
    }

    IMFMediaType *pUncompressedAudioType = NULL;
    IMFMediaType *pPartialType = NULL;

    // Select the first audio stream, and deselect all other streams.
    ASSERT_FILE(SUCCEEDED(pReader->lpVtbl->SetStreamSelection(pReader, (DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE)));
    ASSERT_FILE(SUCCEEDED(pReader->lpVtbl->SetStreamSelection(pReader, (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE)));

    // Create a partial media type that specifies uncompressed PCM audio.
    ASSERT_FILE(SUCCEEDED(MFCreateMediaType(&pPartialType)));
    ASSERT_FILE(SUCCEEDED(pPartialType->lpVtbl->SetGUID(pPartialType, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio)));
    ASSERT_FILE(SUCCEEDED(pPartialType->lpVtbl->SetGUID(pPartialType, &MF_MT_SUBTYPE, &MFAudioFormat_PCM)));
    ASSERT_FILE(SUCCEEDED(pPartialType->lpVtbl->SetUINT32(pPartialType, &MF_MT_AUDIO_SAMPLES_PER_SECOND, TINY3D_SAMPLE_RATE)));
    ASSERT_FILE(SUCCEEDED(pPartialType->lpVtbl->SetUINT32(pPartialType, &MF_MT_AUDIO_BITS_PER_SAMPLE, 16)));
    ASSERT_FILE(SUCCEEDED(pPartialType->lpVtbl->SetUINT32(pPartialType, &MF_MT_AUDIO_NUM_CHANNELS, 2)));

    // Set this type on the source reader. The source reader will
    // load the necessary decoder.
    ASSERT_FILE(SUCCEEDED(pReader->lpVtbl->SetCurrentMediaType(pReader, (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, pPartialType)));

    // Get the complete uncompressed format.
    ASSERT_FILE(SUCCEEDED(pReader->lpVtbl->GetCurrentMediaType(pReader, (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pUncompressedAudioType)));

    // Ensure the stream is selected.
    ASSERT_FILE(SUCCEEDED(pReader->lpVtbl->SetStreamSelection(pReader, (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE)));

    int nSamples = 0;
    DWORD cbAudioData = 0;
    DWORD cbBuffer = 0;
    BYTE *pAudioData = NULL;
    IMFSample *pSample = NULL;
    IMFMediaBuffer *pBuffer = NULL;
    while (1){
        DWORD dwFlags = 0;
        ASSERT_FILE(SUCCEEDED(pReader->lpVtbl->ReadSample(pReader, (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, NULL, &dwFlags, NULL, &pSample)));

        if (dwFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
        {
            fatal_error("Type change - not supported by WAVE file format.\n");
            break;
        }
        if (dwFlags & MF_SOURCE_READERF_ENDOFSTREAM)
        {
            break;
        }

        if (pSample == NULL)
        {
            continue;
        }

        DWORD totalLen;
        ASSERT_FILE(SUCCEEDED(pSample->lpVtbl->GetTotalLength(pSample,&totalLen)));

        nSamples += totalLen/sizeof(int16_t);
    }

    PROPVARIANT var;
    var.vt = VT_I8;
    var.hVal.QuadPart = 0;
    ASSERT_FILE(SUCCEEDED(pReader->lpVtbl->SetCurrentPosition(pReader,&GUID_NULL,&var)));
    int16_t *out = malloc(nSamples*sizeof(*out));
    ASSERT_FILE(out);
    BYTE *outp = (BYTE *)out;

    *nFrames = nSamples/2;

    while (1){
        DWORD dwFlags = 0;
        ASSERT_FILE(SUCCEEDED(pReader->lpVtbl->ReadSample(pReader, (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, NULL, &dwFlags, NULL, &pSample)));

        if (dwFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
        {
            fatal_error("Type change - not supported by WAVE file format.\n");
            break;
        }
        if (dwFlags & MF_SOURCE_READERF_ENDOFSTREAM)
        {
            break;
        }

        if (pSample == NULL)
        {
            continue;
        }

        ASSERT_FILE(SUCCEEDED(pSample->lpVtbl->ConvertToContiguousBuffer(pSample, &pBuffer)));

        ASSERT_FILE(SUCCEEDED(pBuffer->lpVtbl->Lock(pBuffer, &pAudioData, NULL, &cbBuffer)));

        memcpy(outp,pAudioData,cbBuffer);
        outp += cbBuffer;
        
        ASSERT_FILE(SUCCEEDED(pBuffer->lpVtbl->Unlock(pBuffer)));

        pBuffer->lpVtbl->Release(pBuffer);
        pSample->lpVtbl->Release(pSample);
    }

    pUncompressedAudioType->lpVtbl->Release(pUncompressedAudioType);
    pPartialType->lpVtbl->Release(pPartialType);

    pReader->lpVtbl->Release(pReader);

    va_end(args);

    return out;
}

typedef struct {
	BITMAPINFOHEADER    bmiHeader;
	RGBQUAD             bmiColors[4];
} BITMAPINFO_TRUECOLOR32;

static struct GdiImage{
	int width, height;
	unsigned char *pixels;
	HDC hdcBmp;
	HFONT fontOld;
} gdiImg;
/*
static void GdiImageNew(GdiImage *img, int width, int height){
	img->width = width;
	img->height = height;
	HDC hdcScreen = GetDC(0);
	img->hdcBmp = CreateCompatibleDC(hdcScreen);
	ReleaseDC(0,hdcScreen);
	BITMAPINFO_TRUECOLOR32 bmi = {
		.bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
		.bmiHeader.biWidth = width,
		.bmiHeader.biHeight = height,
		.bmiHeader.biPlanes = 1,
		.bmiHeader.biCompression = BI_RGB | BI_BITFIELDS,
		.bmiHeader.biBitCount = 32,
		.bmiColors[0].rgbRed = 0xff,
		.bmiColors[1].rgbGreen = 0xff,
		.bmiColors[2].rgbBlue = 0xff,
	};
    img->hbm = CreateDIBitmap(img->hdcBmp,(BITMAPINFOHEADER *)&bmi,CBM_INIT,(void *)img->pixels,(BITMAPINFO *)&bmi,DIB_RGB_COLORS);
	//img->hbm = CreateDIBSection(img->hdcBmp,(BITMAPINFO *)&bmi,DIB_RGB_COLORS,(void **)&img->pixels,0,0);
	ASSERT(img->hbm);
	img->hbmOld = SelectObject(img->hdcBmp,img->hbm);
	img->fontOld = 0;
}

static void GdiImageDestroy(GdiImage *img){
	if (img->fontOld){
		SelectObject(img->hdcBmp,img->fontOld);
	}
	SelectObject(img->hdcBmp,img->hbmOld);
	DeleteDC(img->hdcBmp);
	DeleteObject(img->hbm);
	memset(img,0,sizeof(*img));
}

static void GdiImageSetFont(GdiImage *img, HFONT font){
	HFONT old = SelectObject(img->hdcBmp,font);
	if (!img->fontOld){
		img->fontOld = old;
	}
	SetBkMode(img->hdcBmp,TRANSPARENT);
}

static void GdiImageSetFontColor(GdiImage *img, uint32_t color){
	SetTextColor(img->hdcBmp,color & 0xffffff);
}

static void GdiImageDrawText(GdiImage *img, WCHAR *str, int x, int y){
	ExtTextOutW(img->hdcBmp,x,y,0,0,str,(UINT)wcslen(str),0);
}

static void GdiImageTextDimensions(GdiImage *img, WCHAR *str, int *width, int *height){
	RECT r = {0};
	DrawTextW(img->hdcBmp,str,(UINT)wcslen(str),&r,DT_CALCRECT|DT_NOPREFIX);
	*width = r.right-r.left;
	*height = r.bottom-r.top;
}

static HFONT GetUserChosenFont(){
	LOGFONTW lf;
	CHOOSEFONTW cf = {
		.lStructSize = sizeof(cf),
		.lpLogFont = &lf,
		.Flags = CF_INITTOLOGFONTSTRUCT,
	};
	ASSERT(ChooseFontW(&cf));
	return CreateFontIndirectW(&lf);
}

static HFONT GetSystemUiFont(){
	NONCLIENTMETRICSW ncm = {
		.cbSize = sizeof(ncm)
	};
	SystemParametersInfoW(SPI_GETNONCLIENTMETRICS,sizeof(ncm),&ncm,0);
	return CreateFontIndirectW(&ncm.lfCaptionFont);
}
*/
static void ensure_hdcBmp(){
    if (!gdiImg.hdcBmp){
        HDC hdcScreen = GetDC(0);
        gdiImg.hdcBmp = CreateCompatibleDC(hdcScreen);
        ReleaseDC(0,hdcScreen);
    }
}
void text_set_target_image(uint32_t *pixels, int width, int height){
    gdiImg.width = width;
    gdiImg.height = height;
    gdiImg.pixels = pixels;

    ensure_hdcBmp();
}
void text_set_font(char *ttfPathFormat, ...){
    va_list args;
	va_start(args,ttfPathFormat);
	char *path = local_path_to_absolute_vararg(ttfPathFormat,args);
	va_end(args);

    ASSERT(1 == AddFontResourceExA(path,FR_PRIVATE,NULL));

    char name[256];
    char *start = strrchr(path,'/') ? strrchr(path,'/')+1 : strrchr(path,'\\') ? strrchr(path,'\\')+1 : path;
    char *end = strrchr(path,'.') ? strrchr(path,'.') : start + strlen(start);
    size_t len = end - start;
    ASSERT(len < COUNT(name));
    memcpy(name,start,len);
    name[len] = 0;

    puts(name);
    
    HFONT font = CreateFontA(-48,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY, VARIABLE_PITCH,"Nunito");

    ensure_hdcBmp();

    HFONT old = SelectObject(gdiImg.hdcBmp,font);
	if (!gdiImg.fontOld){
		gdiImg.fontOld = old;
	} else {
        DeleteObject(old);
    }
    //SetBkColor(gdiImg.hdcBmp,RGB(255,0,0));
	//SetBkMode(gdiImg.hdcBmp,TRANSPARENT);
}
void text_set_font_height(int height);
void text_set_color(uint32_t color);
void text_draw(int left, int right, int bottom, int top, char *str){
    BITMAPINFO_TRUECOLOR32 bmi = {
		.bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
		.bmiHeader.biWidth = gdiImg.width,
		.bmiHeader.biHeight = gdiImg.height,
		.bmiHeader.biPlanes = 1,
		.bmiHeader.biCompression = BI_RGB | BI_BITFIELDS,
		.bmiHeader.biBitCount = 32,
		.bmiColors[0].rgbRed = 0xff,
		.bmiColors[1].rgbGreen = 0xff,
		.bmiColors[2].rgbBlue = 0xff,
	};
    HBITMAP hbm, hbmOld;
    unsigned char *pixels;
    hbm = CreateDIBSection(gdiImg.hdcBmp,(BITMAPINFO *)&bmi,DIB_RGB_COLORS,&pixels,0,0);
	ASSERT(hbm);
    for (size_t i = 0; i < gdiImg.width*gdiImg.height; i++){
        pixels[i*4+0] = gdiImg.pixels[i*4+2];
        pixels[i*4+1] = gdiImg.pixels[i*4+1];
        pixels[i*4+2] = gdiImg.pixels[i*4+0];
    }
	hbmOld = SelectObject(gdiImg.hdcBmp,hbm);

    RECT r = {
        .left = left,
        .right = right,
        .bottom = gdiImg.height-bottom,
        .top = gdiImg.height-top
    };
    SetTextColor(gdiImg.hdcBmp,RGB(0,255,0));
    SetBkColor(gdiImg.hdcBmp,RGB(0,0,255));
    DrawTextA(gdiImg.hdcBmp,str,-1,&r,DT_LEFT|DT_NOPREFIX);
    GdiFlush();

    for (size_t i = 0; i < gdiImg.width*gdiImg.height; i++){
        gdiImg.pixels[i*4+0] = pixels[i*4+2];
        gdiImg.pixels[i*4+1] = pixels[i*4+1];
        gdiImg.pixels[i*4+2] = pixels[i*4+0];
    }

    SelectObject(gdiImg.hdcBmp,hbmOld);
    DeleteObject(hbm);
}

HWND gwnd;
void captureMouse(){
    RECT r;
    GetClientRect(gwnd,&r);
    ClientToScreen(gwnd,(POINT*)&r.left);
    ClientToScreen(gwnd,(POINT*)&r.right);
    ClipCursor(&r);
    ShowCursor(0);
}
void releaseMouse(){
    RECT r;
    GetClientRect(gwnd,&r);
    POINT p = {r.right/2,r.bottom/2};
    ClientToScreen(gwnd,&p);
    SetCursorPos(p.x,p.y);
    ClipCursor(NULL);
    ShowCursor(1);
}

static bool mouse_is_locked = false;
bool is_mouse_locked(void){
	return mouse_is_locked;
}
void lock_mouse(bool locked){
	mouse_is_locked = locked;
	if (locked){
		captureMouse();
	} else {
        releaseMouse();
	}
}

static const uint8_t keycodes[] = {
    0,27,49,50,51,52,53,54,55,56,57,48,45,61,8,9,81,87,69,82,84,89,85,73,79,
    80,91,93,10,0,65,83,68,70,71,72,74,75,76,59,39,96,0,92,90,88,67,86,66,78,
    77,44,46,47,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,17,3,0,20,0,19,0,5,18,4,26,127
};

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

            HDC hdc = GetDC(hwnd);
            ASSERT(hdc);

            PIXELFORMATDESCRIPTOR pfd = {0};
            pfd.nSize = sizeof(pfd);
            pfd.nVersion = 1;
            pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
            pfd.iPixelType = PFD_TYPE_RGBA;
            pfd.cColorBits = 32;
            pfd.cDepthBits = 24;
            pfd.cStencilBits = 8;

            int pixelFormat = ChoosePixelFormat(hdc,&pfd);
            ASSERT(pixelFormat);
            ASSERT(SetPixelFormat(hdc,pixelFormat,&pfd));
            HGLRC hglrc = wglCreateContext(hdc);
            ASSERT(hglrc);
            ASSERT(wglMakeCurrent(hdc,hglrc));

            typedef BOOL(WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);
            PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
            ASSERT(wglSwapIntervalEXT);
            wglSwapIntervalEXT(1); //enable vsync

            ReleaseDC(hwnd,hdc);

            //init audio:
            {
                safe_coinit();
                ASSERT(SUCCEEDED(CoCreateInstance(&_CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &_IID_IMMDeviceEnumerator, (void**)&enu)));
                ASSERT(SUCCEEDED(enu->lpVtbl->GetDefaultAudioEndpoint(enu, eRender, eConsole, &dev)));
                ASSERT(SUCCEEDED(dev->lpVtbl->Activate(dev, &_IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&client)));
                WAVEFORMATEXTENSIBLE fmtex = {0};
                fmtex.Format.nChannels = 2;
                fmtex.Format.nSamplesPerSec = TINY3D_SAMPLE_RATE;
                fmtex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
                fmtex.Format.wBitsPerSample = 16;
                fmtex.Format.nBlockAlign = (fmtex.Format.nChannels * fmtex.Format.wBitsPerSample) / 8;
                fmtex.Format.nAvgBytesPerSec = fmtex.Format.nSamplesPerSec * fmtex.Format.nBlockAlign;
                fmtex.Format.cbSize = 22;   /* WORD + DWORD + GUID */
                fmtex.Samples.wValidBitsPerSample = 16;
                fmtex.dwChannelMask = SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT;
                fmtex.SubFormat = _KSDATAFORMAT_SUBTYPE_PCM;
                REFERENCE_TIME dur = (REFERENCE_TIME)(((double)TINY3D_AUDIO_BUFSZ) / (((double)fmtex.Format.nSamplesPerSec) * (1.0/10000000.0)));
                ASSERT(SUCCEEDED(client->lpVtbl->Initialize(
                    client,
                    AUDCLNT_SHAREMODE_SHARED,
                    AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM|AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
                    dur, 0, (WAVEFORMATEX*)&fmtex, 0)));
                ASSERT(SUCCEEDED(client->lpVtbl->GetService(client, &_IID_IAudioRenderClient, (void**)&renderClient)));
                ASSERT(SUCCEEDED(client->lpVtbl->Start(client)));
            }

            //register raw mouse input
            #define HID_USAGE_PAGE_GENERIC ((unsigned short) 0x01)
            #define HID_USAGE_GENERIC_MOUSE ((unsigned short) 0x02)
            RAWINPUTDEVICE rid = {
                .usUsagePage = HID_USAGE_PAGE_GENERIC,
                .usUsage = HID_USAGE_GENERIC_MOUSE,
                .dwFlags = RIDEV_INPUTSINK,
                .hwndTarget = hwnd
            };
            RegisterRawInputDevices(&rid, 1, sizeof(rid));
            break;
        }
        case WM_PAINT:{
            RECT cr = {0};
            GetClientRect(hwnd,&cr);
            int width = cr.right-cr.left;
            int height = cr.bottom-cr.top;

            static LARGE_INTEGER freq,tstart,t0,t1;
            static bool started = false;
            if (!started){
                QueryPerformanceFrequency(&freq);
                QueryPerformanceCounter(&tstart);
                t0 = tstart;
                t1 = tstart;
                started = true;
            } else {
                QueryPerformanceCounter(&t1);
            }

            UINT32 total;
            UINT32 padding;
            ASSERT(SUCCEEDED(client->lpVtbl->GetBufferSize(client, &total)));
            ASSERT(SUCCEEDED(client->lpVtbl->GetCurrentPadding(client, &padding)));
            UINT32 remaining = total - padding;
            int16_t *samples;
            ASSERT(SUCCEEDED(renderClient->lpVtbl->GetBuffer(renderClient, remaining, (BYTE **)&samples)));

            #if USE_GL
                update((double)(t1.QuadPart-tstart.QuadPart) / (double)freq.QuadPart, (double)(t1.QuadPart-t0.QuadPart) / (double)freq.QuadPart, width, height, (int)remaining, samples);
            #else
                update((double)(t1.QuadPart-tstart.QuadPart) / (double)freq.QuadPart, (double)(t1.QuadPart-t0.QuadPart) / (double)freq.QuadPart, (int)remaining, samples);
                
                glViewport(0,0,width,height);

                glClearColor(0.0f,0.0f,1.0f,1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                static GLuint texture = 0;
                if (!texture){
                    glGenTextures(1,&texture);
                    glBindTexture(GL_TEXTURE_2D,texture);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                }
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, screen);
                int scale = 1;
                while (SCREEN_WIDTH*scale <= width && SCREEN_HEIGHT*scale <= height){
                    scale++;
                }
                scale--;
                int scaledWidth = scale * SCREEN_WIDTH;
                int scaledHeight = scale * SCREEN_HEIGHT;
                int x = width/2-scaledWidth/2;
                int y = height/2-scaledHeight/2;
                glViewport(x,y,scaledWidth,scaledHeight);
                glEnable(GL_TEXTURE_2D);
                glBegin(GL_QUADS);
                glTexCoord2f(0,0); glVertex2f(-1,-1);
                glTexCoord2f(1,0); glVertex2f(1,-1);
                glTexCoord2f(1,1); glVertex2f(1,1);
                glTexCoord2f(0,1); glVertex2f(-1,1);
                glEnd();
            #endif
            ASSERT(SUCCEEDED(renderClient->lpVtbl->ReleaseBuffer(renderClient,remaining,0)));
            t0 = t1;

            HDC hdc = GetDC(hwnd);
            ASSERT(hdc);
            SwapBuffers(hdc);
            ReleaseDC(hwnd,hdc);

            return 0;
        }
        case WM_DESTROY:{
            PostQuitMessage(0);
            break;
        }
        case WM_MOUSEMOVE:{
            if (!mouse_is_locked){
                mousemove(GET_X_LPARAM(lparam),GET_Y_LPARAM(lparam));
            }
            return 0;
        }
        case WM_INPUT:{
            UINT size = sizeof(RAWINPUT);
			static RAWINPUT raw[sizeof(RAWINPUT)];
			GetRawInputData((HRAWINPUT)lparam, RID_INPUT, raw, &size, sizeof(RAWINPUTHEADER));
            if (raw->header.dwType == RIM_TYPEMOUSE){
                if (mouse_is_locked){
                    mousemove(raw->data.mouse.lLastX,raw->data.mouse.lLastY);
                }
                //cameraRotate(&cam, raw->data.mouse.lLastX,raw->data.mouse.lLastY, -0.002f);
                //if (raw->data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
                //	input.mouse.wheel = (*(short*)&raw->data.mouse.usButtonData) / WHEEL_DELTA;
            }
		    return 0;
	    }
        case WM_LBUTTONDOWN:{
            keydown(KEY_MOUSE_LEFT);
            return 0;
        }
        case WM_RBUTTONDOWN:{
            keydown(KEY_MOUSE_RIGHT);
            return 0;
        }
        case WM_KEYDOWN:{
            if (!(HIWORD(lparam) & KF_REPEAT)){
                keydown((int)keycodes[HIWORD(lparam) & 0x1ff]);
            }
            break;
        }
        case WM_KEYUP:{
            keyup((int)keycodes[HIWORD(lparam) & 0x1ff]);
            break;
        }
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

#if USE_GL
void open_window(int width, int height){
    int scale = 1;
    int SCREEN_WIDTH = width;
    int SCREEN_HEIGHT = height;
#else
void open_window(int scale){
#endif
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

    ASSERT(scale >= 0);
    if (scale > 0){

        RECT initialRect = {0, 0, scale*SCREEN_WIDTH, scale*SCREEN_HEIGHT};
        AdjustWindowRect(&initialRect,WS_OVERLAPPEDWINDOW,FALSE);
        LONG initialWidth = initialRect.right - initialRect.left;
        LONG initialHeight = initialRect.bottom - initialRect.top;

        gwnd = CreateWindowExW(
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
        gwnd = CreateWindowExW(
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
    ASSERT(gwnd);

    MSG msg;
    while (GetMessageW(&msg,0,0,0)){
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}