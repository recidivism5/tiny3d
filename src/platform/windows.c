#include <tiny3d.h>

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

#include <mmdeviceapi.h>
#include <audioclient.h>

HWND gwnd;

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
    ValidateRect(gwnd,0);
    MessageBoxA(0,msg,"Error",MB_ICONERROR);
}

unsigned char *load_image(bool flip_vertically, int *width, int *height, char *format, ...){
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

wchar_t *get_keyboard_layout_name(){
    char name[KL_NAMELENGTH+2];
    GetKeyboardLayoutNameA(name+2);
    name[0] = '0';
    name[1] = 'X';
    int code;
    StrToIntExA(name,STIF_SUPPORT_HEX,&code);
    switch (code){
        case 0x00140C00: return L"ADLaM";
        case 0x0000041C: return L"Albanian";
        case 0x00000401: return L"Arabic (101)";
        case 0x00010401: return L"Arabic (102)";
        case 0x00020401: return L"Arabic (102) AZERTY";
        case 0x0000042B: return L"Armenian Eastern (Legacy)";
        case 0x0002042B: return L"Armenian Phonetic";
        case 0x0003042B: return L"Armenian Typewriter";
        case 0x0001042B: return L"Armenian Western (Legacy)";
        case 0x0000044D: return L"Assamese - INSCRIPT";
        case 0x0001042C: return L"Azerbaijani (Standard)";
        case 0x0000082C: return L"Azerbaijani Cyrillic";
        case 0x0000042C: return L"Azerbaijani Latin";
        case 0x00000445: return L"Bangla";
        case 0x00020445: return L"Bangla - INSCRIPT";
        case 0x00010445: return L"Bangla - INSCRIPT (Legacy)";
        case 0x0000046D: return L"Bashkir";
        case 0x00000423: return L"Belarusian";
        case 0x0001080C: return L"Belgian (Comma)";
        case 0x00000813: return L"Belgian (Period)";
        case 0x0000080C: return L"Belgian French";
        case 0x0000201A: return L"Bosnian (Cyrillic)";
        case 0x000B0C00: return L"Buginese";
        case 0x00030402: return L"Bulgarian";
        case 0x00010402: return L"Bulgarian (Latin)";
        case 0x00040402: return L"Bulgarian (Phonetic Traditional)";
        case 0x00020402: return L"Bulgarian (Phonetic)";
        case 0x00000402: return L"Bulgarian (Typewriter)";
        case 0x00001009: return L"Canadian French";
        case 0x00000C0C: return L"Canadian French (Legacy)";
        case 0x00011009: return L"Canadian Multilingual Standard";
        case 0x0000085F: return L"Central Atlas Tamazight";
        case 0x00000492: return L"Central Kurdish";
        case 0x0000045C: return L"Cherokee Nation";
        case 0x0001045C: return L"Cherokee Phonetic";
        case 0x00000804: return L"Chinese (Simplified) - US";
        case 0x00001004: return L"Chinese (Simplified, Singapore) - US";
        case 0x00000404: return L"Chinese (Traditional) - US";
        case 0x00000C04: return L"Chinese (Traditional, Hong Kong S.A.R.) - US";
        case 0x00001404: return L"Chinese (Traditional, Macao S.A.R.) - US";
        case 0x00000405: return L"Czech";
        case 0x00010405: return L"Czech (QWERTY)";
        case 0x00020405: return L"Czech Programmers";
        case 0x00000406: return L"Danish";
        case 0x00000439: return L"Devanagari - INSCRIPT";
        case 0x00000465: return L"Divehi Phonetic";
        case 0x00010465: return L"Divehi Typewriter";
        case 0x00000413: return L"Dutch";
        case 0x00000C51: return L"Dzongkha";
        case 0x00004009: return L"English (India)";
        case 0x00000425: return L"Estonian";
        case 0x00000438: return L"Faeroese";
        case 0x0000040B: return L"Finnish";
        case 0x0001083B: return L"Finnish with Sami";
        case 0x0000040C: return L"French";
        case 0x00120C00: return L"Futhark";
        case 0x00020437: return L"Georgian (Ergonomic)";
        case 0x00000437: return L"Georgian (Legacy)";
        case 0x00030437: return L"Georgian (MES)";
        case 0x00040437: return L"Georgian (Old Alphabets)";
        case 0x00010437: return L"Georgian (QWERTY)";
        case 0x00000407: return L"German";
        case 0x00010407: return L"German (IBM)";
        case 0x000C0C00: return L"Gothic";
        case 0x00000408: return L"Greek";
        case 0x00010408: return L"Greek (220)";
        case 0x00030408: return L"Greek (220) Latin";
        case 0x00020408: return L"Greek (319)";
        case 0x00040408: return L"Greek (319) Latin";
        case 0x00050408: return L"Greek Latin";
        case 0x00060408: return L"Greek Polytonic";
        case 0x0000046F: return L"Greenlandic";
        case 0x00000474: return L"Guarani";
        case 0x00000447: return L"Gujarati";
        case 0x00000468: return L"Hausa";
        case 0x00000475: return L"Hawaiian";
        case 0x0000040D: return L"Hebrew";
        case 0x0002040D: return L"Hebrew (Standard)";
        case 0x00010439: return L"Hindi Traditional";
        case 0x0000040E: return L"Hungarian";
        case 0x0001040E: return L"Hungarian 101-key";
        case 0x0000040F: return L"Icelandic";
        case 0x00000470: return L"Igbo";
        case 0x0000085D: return L"Inuktitut - Latin";
        case 0x0001045D: return L"Inuktitut - Naqittaut";
        case 0x00001809: return L"Irish";
        case 0x00000410: return L"Italian";
        case 0x00010410: return L"Italian (142)";
        case 0x00000411: return L"Japanese";
        case 0x00110C00: return L"Javanese";
        case 0x0000044B: return L"Kannada";
        case 0x0000043F: return L"Kazakh";
        case 0x00000453: return L"Khmer";
        case 0x00010453: return L"Khmer (NIDA)";
        case 0x00000412: return L"Korean";
        case 0x00000440: return L"Kyrgyz Cyrillic";
        case 0x00000454: return L"Lao";
        case 0x0000080A: return L"Latin American";
        case 0x00000426: return L"Latvian";
        case 0x00010426: return L"Latvian (QWERTY)";
        case 0x00020426: return L"Latvian (Standard)";
        case 0x00070C00: return L"Lisu (Basic)";
        case 0x00080C00: return L"Lisu (Standard)";
        case 0x00010427: return L"Lithuanian";
        case 0x00000427: return L"Lithuanian IBM";
        case 0x00020427: return L"Lithuanian Standard";
        case 0x0000046E: return L"Luxembourgish";
        case 0x0000042F: return L"Macedonian";
        case 0x0001042F: return L"Macedonian - Standard";
        case 0x0000044C: return L"Malayalam";
        case 0x0000043A: return L"Maltese 47-Key";
        case 0x0001043A: return L"Maltese 48-Key";
        case 0x00000481: return L"Maori";
        case 0x0000044E: return L"Marathi";
        case 0x00000850: return L"Mongolian (Mongolian Script)";
        case 0x00000450: return L"Mongolian Cyrillic";
        case 0x00010C00: return L"Myanmar (Phonetic order)";
        case 0x00130C00: return L"Myanmar (Visual order)";
        case 0x00001409: return L"NZ Aotearoa";
        case 0x00000461: return L"Nepali";
        case 0x00020C00: return L"New Tai Lue";
        case 0x00000414: return L"Norwegian";
        case 0x0000043B: return L"Norwegian with Sami";
        case 0x00090C00: return L"N’Ko";
        case 0x00000448: return L"Odia";
        case 0x00040C00: return L"Ogham";
        case 0x000D0C00: return L"Ol Chiki";
        case 0x000F0C00: return L"Old Italic";
        case 0x00150C00: return L"Osage";
        case 0x000E0C00: return L"Osmanya";
        case 0x00000463: return L"Pashto (Afghanistan)";
        case 0x00000429: return L"Persian";
        case 0x00050429: return L"Persian (Standard)";
        case 0x000A0C00: return L"Phags-pa";
        case 0x00010415: return L"Polish (214)";
        case 0x00000415: return L"Polish (Programmers)";
        case 0x00000816: return L"Portuguese";
        case 0x00000416: return L"Portuguese (Brazil ABNT)";
        case 0x00010416: return L"Portuguese (Brazil ABNT2)";
        case 0x00000446: return L"Punjabi";
        case 0x00000418: return L"Romanian (Legacy)";
        case 0x00020418: return L"Romanian (Programmers)";
        case 0x00010418: return L"Romanian (Standard)";
        case 0x00000419: return L"Russian";
        case 0x00010419: return L"Russian (Typewriter)";
        case 0x00020419: return L"Russian - Mnemonic";
        case 0x00000485: return L"Sakha";
        case 0x0002083B: return L"Sami Extended Finland-Sweden";
        case 0x0001043B: return L"Sami Extended Norway";
        case 0x00011809: return L"Scottish Gaelic";
        case 0x00000C1A: return L"Serbian (Cyrillic)";
        case 0x0000081A: return L"Serbian (Latin)";
        case 0x0000046C: return L"Sesotho sa Leboa";
        case 0x00000432: return L"Setswana";
        case 0x0000045B: return L"Sinhala";
        case 0x0001045B: return L"Sinhala - Wij 9";
        case 0x0000041B: return L"Slovak";
        case 0x0001041B: return L"Slovak (QWERTY)";
        case 0x00000424: return L"Slovenian";
        case 0x00100C00: return L"Sora";
        case 0x0001042E: return L"Sorbian Extended";
        case 0x0002042E: return L"Sorbian Standard";
        case 0x0000042E: return L"Sorbian Standard (Legacy)";
        case 0x0000040A: return L"Spanish";
        case 0x0001040A: return L"Spanish Variation";
        case 0x0000041A: return L"Standard";
        case 0x0000041D: return L"Swedish";
        case 0x0000083B: return L"Swedish with Sami";
        case 0x0000100C: return L"Swiss French";
        case 0x00000807: return L"Swiss German";
        case 0x0000045A: return L"Syriac";
        case 0x0001045A: return L"Syriac Phonetic";
        case 0x00030C00: return L"Tai Le";
        case 0x00000428: return L"Tajik";
        case 0x00000449: return L"Tamil";
        case 0x00020449: return L"Tamil 99";
        case 0x00030449: return L"Tamil Anjal";
        case 0x00010444: return L"Tatar";
        case 0x00000444: return L"Tatar (Legacy)";
        case 0x0000044A: return L"Telugu";
        case 0x0000041E: return L"Thai Kedmanee";
        case 0x0002041E: return L"Thai Kedmanee (non-ShiftLock)";
        case 0x0001041E: return L"Thai Pattachote";
        case 0x0003041E: return L"Thai Pattachote (non-ShiftLock)";
        case 0x00000451: return L"Tibetan (PRC)";
        case 0x00010451: return L"Tibetan (PRC) - Updated";
        case 0x0000105F: return L"Tifinagh (Basic)";
        case 0x0001105F: return L"Tifinagh (Extended)";
        case 0x00010850: return L"Traditional Mongolian (Standard)";
        case 0x0001041F: return L"Turkish F";
        case 0x0000041F: return L"Turkish Q";
        case 0x00000442: return L"Turkmen";
        case 0x00000409: return L"US";
        case 0x00050409: return L"US English Table for IBM Arabic 238_L";
        case 0x00000422: return L"Ukrainian";
        case 0x00020422: return L"Ukrainian (Enhanced)";
        case 0x00000809: return L"United Kingdom";
        case 0x00000452: return L"United Kingdom Extended";
        case 0x00010409: return L"United States-Dvorak";
        case 0x00030409: return L"United States-Dvorak for left hand";
        case 0x00040409: return L"United States-Dvorak for right hand";
        case 0x00020409: return L"United States-International";
        case 0x00000420: return L"Urdu";
        case 0x00010480: return L"Uyghur";
        case 0x00000480: return L"Uyghur (Legacy)";
        case 0x00000843: return L"Uzbek Cyrillic";
        case 0x0000042A: return L"Vietnamese";
        case 0x00000488: return L"Wolof";
        case 0x0000046A: return L"Yoruba";
        default: return L"Unknown";
    }
}

void get_key_text(int scancode, wchar_t *buf, int bufcount){
    GetKeyNameTextW(scancode<<16,buf,bufcount);
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
    int fontHeight;
    char fontName[MAX_PATH];
} gdiImg = {.fontHeight = 12};

static void ensure_hdcBmp(){
    if (!gdiImg.hdcBmp){
        HDC hdcScreen = GetDC(0);
        gdiImg.hdcBmp = CreateCompatibleDC(hdcScreen);
        ReleaseDC(0,hdcScreen);
        SetBkMode(gdiImg.hdcBmp,TRANSPARENT);
    }
}
void text_set_target_image(uint32_t *pixels, int width, int height){
    gdiImg.width = width;
    gdiImg.height = height;
    gdiImg.pixels = (unsigned char *)pixels;

    ensure_hdcBmp();
}
void register_font(char *path){
    ASSERT(1 == AddFontResourceExA(path,FR_PRIVATE,NULL));
}
void text_set_font(char *font_family, int size){
    HFONT font = CreateFontA(size*2,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY, VARIABLE_PITCH,font_family);

    ensure_hdcBmp();

    HFONT old = SelectObject(gdiImg.hdcBmp,font);
	if (!gdiImg.fontOld){
		gdiImg.fontOld = old;
	} else {
        DeleteObject(old);
    }
}
void text_set_color(float r, float g, float b){
    SetTextColor(gdiImg.hdcBmp,RGB((int)(255*r),(int)(255*g),(int)(255*b)));
}
void text_get_bounds(char *str, int *width, int *height){
    RECT r;
    DrawTextA(gdiImg.hdcBmp,str,-1,&r,DT_LEFT|DT_NOPREFIX|DT_EXPANDTABS|DT_CALCRECT);
    *width = r.right-r.left;
    *height = r.bottom-r.top;
}
void text_draw(int x, int y, char *str){
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

    ExtTextOutA(gdiImg.hdcBmp,x,gdiImg.height-1-y,0,0,str,strlen(str),0);
    GdiFlush();

    for (size_t i = 0; i < gdiImg.width*gdiImg.height; i++){
        gdiImg.pixels[i*4+0] = pixels[i*4+2];
        gdiImg.pixels[i*4+1] = pixels[i*4+1];
        gdiImg.pixels[i*4+2] = pixels[i*4+0];
    }

    SelectObject(gdiImg.hdcBmp,hbmOld);
    DeleteObject(hbm);
}

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

static int local_min_width, local_min_height;
static RECT prev_rect;

static bool mouse_locked = false;
bool is_mouse_locked(void){
	return mouse_locked;
}
void toggle_mouse_lock(){
	if (mouse_locked){
        releaseMouse();
	} else {
		captureMouse();
	}
    mouse_locked = !mouse_locked;
}

static bool fullscreen = false;
bool is_fullscreen(){
    return fullscreen;
}
void toggle_fullscreen(){
    if (fullscreen){
        SetWindowLongPtr(gwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        MoveWindow(
            gwnd,
            prev_rect.left,
            prev_rect.top,
            prev_rect.right-prev_rect.left,
            prev_rect.bottom-prev_rect.top,
            FALSE
        );
    } else {
        MONITORINFO monitor = {0};
        monitor.cbSize = sizeof(monitor);
        GetMonitorInfo(MonitorFromWindow(gwnd, MONITOR_DEFAULTTONEAREST), &monitor);
        GetWindowRect(gwnd,&prev_rect);
        SetWindowLongPtr(gwnd, GWL_STYLE, WS_SYSMENU | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE);
        MoveWindow(gwnd, 0, 0, monitor.rcMonitor.right-monitor.rcMonitor.left, monitor.rcMonitor.bottom-monitor.rcMonitor.top, FALSE);
    }
    fullscreen = !fullscreen;
}

float get_dpi_scale(){
    return 1.0f;
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
        case WM_GETMINMAXINFO:{
            MINMAXINFO *mmi = (MINMAXINFO *)lparam;
            mmi->ptMinTrackSize.x = local_min_width;
            mmi->ptMinTrackSize.y = local_min_height;
            return 0;
        }
        case WM_PAINT:{
            RECT cr = {0};
            GetClientRect(hwnd,&cr);
            window_width = cr.right-cr.left;
            window_height = cr.bottom-cr.top;

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
            update((double)(t1.QuadPart-tstart.QuadPart) / (double)freq.QuadPart, (double)(t1.QuadPart-t0.QuadPart) / (double)freq.QuadPart, (int)remaining, samples);
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
            if (!mouse_locked){
                RECT cr;
                GetClientRect(hwnd,&cr);
                mousemove(GET_X_LPARAM(lparam),(cr.bottom-cr.top)-1-GET_Y_LPARAM(lparam));
            }
            return 0;
        }
        case WM_INPUT:{
            UINT size = sizeof(RAWINPUT);
			static RAWINPUT raw[sizeof(RAWINPUT)];
			GetRawInputData((HRAWINPUT)lparam, RID_INPUT, raw, &size, sizeof(RAWINPUTHEADER));
            if (raw->header.dwType == RIM_TYPEMOUSE){
                if (mouse_locked){
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
        case WM_LBUTTONUP:{
            keyup(KEY_MOUSE_LEFT);
            return 0;
        }
        case WM_RBUTTONUP:{
            keyup(KEY_MOUSE_RIGHT);
            return 0;
        }
        case WM_KEYDOWN:{
            if (!(HIWORD(lparam) & KF_REPEAT)){
                keydown((lparam & 0xff0000)>>16);
            }
            break;
        }
        case WM_KEYUP:{
            keyup((lparam & 0xff0000)>>16);
            break;
        }
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void open_window(int min_width, int min_height, char *name){
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    size_t ncount = strlen(name)+1;
    WCHAR *wname = malloc(ncount*sizeof(*wname));
    mbstowcs(wname,name,ncount);
    WNDCLASSEXW wcex = {
        .cbSize = sizeof(wcex),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = WndProc,
        .hInstance = GetModuleHandleW(0),
        .hIcon = LoadIconW(GetModuleHandleW(0),MAKEINTRESOURCEW(69)),
        .hCursor = LoadCursorW(0,IDC_ARROW),
        .lpszClassName = wname,
        .hIconSm = 0,
    };
    ASSERT(RegisterClassExW(&wcex));

    ASSERT(min_width > 0 && min_height > 0);
    RECT initialRect = {0, 0, min_width, min_height};
    AdjustWindowRect(&initialRect,WS_OVERLAPPEDWINDOW,FALSE);
    local_min_width = initialRect.right - initialRect.left;
    local_min_height = initialRect.bottom - initialRect.top;
    gwnd = CreateWindowExW(
        0, //WS_EX_OVERLAPPEDWINDOW fucks up the borders when maximized
        wcex.lpszClassName,
        wcex.lpszClassName,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        GetSystemMetrics(SM_CXSCREEN)/2-local_min_width/2,
        GetSystemMetrics(SM_CYSCREEN)/2-local_min_height/2,
        local_min_width, 
        local_min_height,
        0, 0, wcex.hInstance, 0
    );
    ASSERT(gwnd);

    init();

    MSG msg;
    while (GetMessageW(&msg,0,0,0)){
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}