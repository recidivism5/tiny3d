#include <tiny3d.h>

#include <X11/X.h>
#include <X11/Xlib.h>

#include <GL/gl.h>
#include <GL/glx.h>

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

bool is_mouse_locked(void){}
void lock_mouse(bool locked){}

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>

uint32_t *load_image(bool flip_vertically, int *width, int *height, char *format, ...) {
    va_list args;
    va_start(args,format);
    assertPath = local_path_to_absolute_vararg(format,args);
    va_end(args);

    AVFormatContext *format_ctx = NULL;
    AVCodecContext *codec_ctx = NULL;
    AVCodec *codec = NULL;
    AVFrame *frame = NULL;
    AVPacket packet;
    int ret;

    // Open input file and initialize format context
    if ((ret = avformat_open_input(&format_ctx, assertPath, NULL, NULL)) < 0) {
        fprintf(stderr, "Error opening input file: %s\n", av_err2str(ret));
        return NULL;
    }

    // Find stream info
    if ((ret = avformat_find_stream_info(format_ctx, NULL)) < 0) {
        fprintf(stderr, "Error finding stream info: %s\n", av_err2str(ret));
        avformat_close_input(&format_ctx);
        return NULL;
    }

    // Find the first video stream
    int video_stream_index = av_find_best_stream(format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (video_stream_index < 0) {
        fprintf(stderr, "Could not find video stream in input file\n");
        avformat_close_input(&format_ctx);
        return NULL;
    }

    // Find and initialize the codec context
    codec_ctx = avcodec_alloc_context3(NULL);
    if (!codec_ctx) {
        fprintf(stderr, "Failed to allocate codec context\n");
        avformat_close_input(&format_ctx);
        return NULL;
    }
    avcodec_parameters_to_context(codec_ctx, format_ctx->streams[video_stream_index]->codecpar);
    codec = avcodec_find_decoder(codec_ctx->codec_id);
    if (!codec) {
        fprintf(stderr, "Unsupported codec!\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return NULL;
    }
    if ((ret = avcodec_open2(codec_ctx, codec, NULL)) < 0) {
        fprintf(stderr, "Error opening codec: %s\n", av_err2str(ret));
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return NULL;
    }

    // Allocate video frame
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return NULL;
    }

    // Initialize packet
    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;

    // Read frames
    if (av_read_frame(format_ctx, &packet) < 0) {
        fprintf(stderr, "Error reading frame\n");
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return NULL;
    }

    ret = avcodec_send_packet(codec_ctx, &packet);
    if (ret < 0) {
        fprintf(stderr, "Error sending packet to decoder: %s\n", av_err2str(ret));
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return NULL;
    }

    ret = avcodec_receive_frame(codec_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error receiving frame from decoder: %s\n", av_err2str(ret));
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return NULL;
    }

    // Allocate RGBA buffer
    uint32_t *rgba_buffer = (uint32_t *)malloc(frame->width * frame->height * sizeof(uint32_t));
    if (!rgba_buffer) {
        fprintf(stderr, "Failed to allocate RGBA buffer\n");
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return NULL;
    }

    // Convert frame to RGBA
    struct SwsContext *sws_ctx = sws_getContext(
        frame->width, frame->height, codec_ctx->pix_fmt,
        frame->width, frame->height, AV_PIX_FMT_RGBA,
        SWS_BILINEAR, NULL, NULL, NULL);

    if (!sws_ctx) {
        fprintf(stderr, "Failed to initialize swscale context\n");
        free(rgba_buffer);
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return NULL;
    }

    uint8_t *rgba_data[1] = { (uint8_t *)rgba_buffer };
    int rgba_linesize[1] = { frame->width * sizeof(uint32_t) };
    if (flip_vertically){
        frame->data[0] += frame->linesize[0]*(frame->height-1);
        frame->linesize[0] = -frame->linesize[0];
    }
    sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, rgba_data, rgba_linesize);

    *width = frame->width;
    *height = frame->height;

    // Clean up
    sws_freeContext(sws_ctx);
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);

    return rgba_buffer;
}
#include <libswresample/swresample.h>
#define INITIAL_BUFFER_SIZE 4096 // Initial buffer size, can be adjusted
#define TARGET_SAMPLE_FORMAT AV_SAMPLE_FMT_S16
int16_t *load_audio(int *nFrames, char *format, ...){
    va_list args;
    va_start(args,format);
    assertPath = local_path_to_absolute_vararg(format,args);
    va_end(args);

    AVFormatContext *format_ctx = NULL;
    AVCodecContext *codec_ctx = NULL;
    AVCodec *codec = NULL;
    AVPacket packet;
    AVFrame *frame = NULL;
    SwrContext *swr_ctx = NULL;
    int16_t *audio_data = NULL;
    int ret, frame_size, channels;
    int16_t *output_ptr;
    int buffer_size = INITIAL_BUFFER_SIZE;
    
    // Open the audio file
    if ((ret = avformat_open_input(&format_ctx, assertPath, NULL, NULL)) < 0) {
        fprintf(stderr, "Error opening input file: %s\n", av_err2str(ret));
        return NULL;
    }
    
    // Find stream info
    if ((ret = avformat_find_stream_info(format_ctx, NULL)) < 0) {
        fprintf(stderr, "Error finding stream info: %s\n", av_err2str(ret));
        avformat_close_input(&format_ctx);
        return NULL;
    }
    
    // Find the audio stream
    int audio_stream_index = av_find_best_stream(format_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
    if (audio_stream_index < 0) {
        fprintf(stderr, "No audio stream found in the input file\n");
        avformat_close_input(&format_ctx);
        return NULL;
    }
    
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        fprintf(stderr, "Failed to allocate codec context\n");
        avformat_close_input(&format_ctx);
        return NULL;
    }
    
    avcodec_parameters_to_context(codec_ctx, format_ctx->streams[audio_stream_index]->codecpar);
    
    // Open codec
    if ((ret = avcodec_open2(codec_ctx, codec, NULL)) < 0) {
        fprintf(stderr, "Failed to open codec: %s\n", av_err2str(ret));
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return NULL;
    }
    
    // Allocate frame
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Failed to allocate frame\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return NULL;
    }
    
    // Allocate resampler context
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        fprintf(stderr, "Failed to allocate resampler context\n");
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return NULL;
    }
    
    // Set resampler parameters
    av_opt_set_int(swr_ctx, "in_channel_layout", codec_ctx->channel_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", codec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", codec_ctx->sample_fmt, 0);
    av_opt_set_int(swr_ctx, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", TINY3D_SAMPLE_RATE, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", TARGET_SAMPLE_FORMAT, 0);
    
    // Initialize resampler context
    if ((ret = swr_init(swr_ctx)) < 0) {
        fprintf(stderr, "Failed to initialize resampler context: %s\n", av_err2str(ret));
        swr_free(&swr_ctx);
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return NULL;
    }
    
    // Determine audio parameters after resampling
    channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    frame_size = av_rescale_rnd(codec_ctx->frame_size, TINY3D_SAMPLE_RATE, codec_ctx->sample_rate, AV_ROUND_UP);
    
    // Allocate memory for decoded audio
    audio_data = (int16_t *)malloc(buffer_size * channels * sizeof(int16_t));
    if (!audio_data) {
        fprintf(stderr, "Failed to allocate memory for audio data\n");
        swr_free(&swr_ctx);
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return NULL;
    }
    
    // Decode and resample audio
    while (av_read_frame(format_ctx, &packet) >= 0) {
        if (packet.stream_index == audio_stream_index) {
            ret = avcodec_send_packet(codec_ctx, &packet);
            if (ret < 0) {
                fprintf(stderr, "Error sending a packet for decoding: %s\n", av_err2str(ret));
                break;
            }
            
            while (ret >= 0) {
                ret = avcodec_receive_frame(codec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                else if (ret < 0) {
                    fprintf(stderr, "Error during decoding: %s\n", av_err2str(ret));
                    break;
                }
                
                // Resize buffer if needed
                if ((*nFrames + frame_size) * channels > buffer_size) {
                    buffer_size *= 2;
                    audio_data = (int16_t *)realloc(audio_data, buffer_size * channels * sizeof(int16_t));
                    if (!audio_data) {
                        fprintf(stderr, "Failed to reallocate memory for audio data\n");
                        swr_free(&swr_ctx);
                        av_frame_free(&frame);
                        avcodec_free_context(&codec_ctx);
                        avformat_close_input(&format_ctx);
                        return NULL;
                    }
                }
                
                // Resample audio
                output_ptr = audio_data + (*nFrames * channels);
                ret = swr_convert(swr_ctx, (uint8_t **)&output_ptr, frame_size, (const uint8_t **)frame->data, frame->nb_samples);
                if (ret < 0) {
                    fprintf(stderr, "Error during resampling: %s\n", av_err2str(ret));
                    break;
                }
                
                *nFrames += frame_size;
            }
        }
        av_packet_unref(&packet);
    }
    
    // Clean up
    swr_free(&swr_ctx);
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    
    return audio_data;
}

#include <pulse/error.h>
#include <pulse/simple.h>

void text_set_target_image(uint32_t *pixels, int width, int height){}
void text_set_font(char *ttfPathFormat, ...){}
void text_set_font_height(int height){}
void text_set_color(float r, float g, float b){}
void text_draw(int left, int right, int bottom, int top, char *str){}

#include <X11/XKBlib.h>

static XkbDescPtr kbdesc;
static Display *display;
static Window window;
static GLXContext context;

#define WINW 640
#define WINH 480

#define _NET_WM_STATE_REMOVE    0l
#define _NET_WM_STATE_ADD       1l

void toggle_fullscreen(){
    static bool fullscreen = false;
    fullscreen = !fullscreen;
    Atom _NET_WM_STATE = XInternAtom(display, "_NET_WM_STATE", False);
    Atom _NET_WM_STATE_FULLSCREEN = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    XEvent xevent;

    XSizeHints *sizehints = XAllocSizeHints();
    long flags = 0;
    XGetWMNormalHints(display, window, sizehints, &flags);

    if (fullscreen) {
        /* we are going fullscreen so turn the flags off */
        sizehints->flags &= ~(PMinSize | PMaxSize);
    } else {
        /* Reset the min/max width height to make the window non-resizable again */
        sizehints->flags |= PMinSize | PMaxSize;
        sizehints->min_width = sizehints->max_width = WINW;
        sizehints->min_height = sizehints->max_height = WINH;
    }
    XSetWMNormalHints(display, window, sizehints);
    XFree(sizehints);

    memset(&xevent, 0, sizeof (xevent));
    xevent.xany.type = ClientMessage;
    xevent.xclient.message_type = _NET_WM_STATE;
    xevent.xclient.format = 32;
    xevent.xclient.window = window;
    xevent.xclient.data.l[0] = fullscreen ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
    xevent.xclient.data.l[1] = _NET_WM_STATE_FULLSCREEN;
    xevent.xclient.data.l[3] = 0l;
    XSendEvent(display, RootWindow(display, DefaultScreen(display)), 0, SubstructureNotifyMask | SubstructureRedirectMask, &xevent);
    XFlush(display);
}

void open_window(int width, int height){
    display = XOpenDisplay(NULL);

    kbdesc = XkbGetMap(display, 0, XkbUseCoreKbd);
    XkbGetNames(display, XkbKeyNamesMask | XkbKeyAliasesMask, kbdesc);
    for (int scancode = kbdesc->min_key_code; scancode < kbdesc->max_key_code; scancode++){
        printf("%d: %.*s\n",scancode,XkbKeyNameLength,kbdesc->names->keys[scancode].name);
    }

    XSetWindowAttributes xattr;
    XSizeHints *sizehints;
    XWMHints *wmhints;
    XClassHint *classhints;

    memset(&xattr, '\0', sizeof (xattr));
    xattr.event_mask = ExposureMask |
                       ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask |
                       StructureNotifyMask | FocusChangeMask | PointerMotionMask;

    window = XCreateWindow(display, RootWindow(display, DefaultScreen(display)),
                       0, 0, WINW, WINH,
                       0, CopyFromParent, InputOutput, CopyFromParent,
                       CWEventMask, &xattr );

    sizehints = XAllocSizeHints();
    sizehints->flags = 0;
    sizehints->min_width = sizehints->max_width = WINW;
    sizehints->min_height = sizehints->max_height = WINH;
    sizehints->flags |= (PMaxSize | PMinSize);

    sizehints->x = 0;
    sizehints->y = 0;
    sizehints->flags |= USPosition;

    /* Setup the input hints so we get keyboard input */
    wmhints = XAllocWMHints();
    wmhints->input = True;
    wmhints->window_group = (XID) getpid();
    wmhints->flags = InputHint | WindowGroupHint;

    /* Setup the class hints so we can get an icon (AfterStep) */
    classhints = XAllocClassHint();
    classhints->res_name = "SDL_App";
    classhints->res_class = "SDL_App";

    /* Set the size, input and class hints, and define WM_CLIENT_MACHINE and WM_LOCALE_NAME */
    XSetWMProperties(display, window, NULL, NULL, NULL, 0, sizehints, wmhints, classhints);

    XFree(sizehints);
    XFree(wmhints);
    XFree(classhints);

    long pid = (long) getpid();
    Atom _NET_WM_PID = XInternAtom(display, "_NET_WM_PID", False);
    XChangeProperty(display, window, _NET_WM_PID, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &pid, 1);

    Atom _NET_WM_WINDOW_TYPE = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom _NET_WM_WINDOW_TYPE_NORMAL = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", False);
    XChangeProperty(display, window, _NET_WM_WINDOW_TYPE, XA_ATOM, 32, PropModeReplace, (unsigned char *)&_NET_WM_WINDOW_TYPE_NORMAL, 1);

    int visual_hints[] = {
        GLX_RGBA,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER,
        None
    };
    XVisualInfo *visual_info = glXChooseVisual(display, DefaultScreen(display), visual_hints);
    context = glXCreateContext(display, visual_info, NULL, True);
    glXMakeCurrent(display, window, context);
    typedef void (*PFNGLXSWAPINTERVALEXTPROC)(Display *, GLXDrawable, int);
    PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddress("glXSwapIntervalEXT");
    ASSERT(glXSwapIntervalEXT);
    glXSwapIntervalEXT(display,window,1);

    XMapWindow(display, window);
    XRaiseWindow(display, window);
    //XMoveWindow(display, window, swidth/2-width/2, sheight/2-height/2);
    Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wm_delete_window, 1);

    static int16_t audioBuf[TINY3D_AUDIO_BUFSZ*2];
    pa_sample_spec spec = {
        .format = PA_SAMPLE_S16NE,
        .channels = 2,
        .rate = TINY3D_SAMPLE_RATE
    };
    pa_simple *stream = pa_simple_new(
        NULL,               // Use the default server.
        "tiny3d",           // Our application's name.
        PA_STREAM_PLAYBACK,
        NULL,               // Use the default device.
        "Game",            // Description of our stream.
        &spec,                // Our sample format.
        NULL,               // Use default channel map
        NULL,               // Use default buffering attributes.
        NULL               // Ignore error code.
    );
    ASSERT(stream);
    
    while (1){
        XEvent event;
        while (XPending(display)){
            XNextEvent(display, &event);
            switch (event.type){
                case ClientMessage:{
                    if ((Atom)event.xclient.data.l[0] == wm_delete_window){
                        goto EXIT;
                    }
                    break;
                }
                case KeyPress: keydown(event.xkey.keycode-kbdesc->min_key_code); break;
                case KeyRelease: keyup(event.xkey.keycode-kbdesc->min_key_code); break;
                case ButtonPress: switch (event.xbutton.button){
                    case 1: keydown(KEY_MOUSE_LEFT); break;
                    case 3: keydown(KEY_MOUSE_RIGHT); break;
                } break;
            }/*
            if(event.type == KeyPress)
            {
                switch(XKeycodeToKeysym(display, event.xkey.keycode, 0))
                {
                    case XK_Escape: return;
                }
            }*/
        }

        XWindowAttributes attribs;
        XGetWindowAttributes(display, window, &attribs);
        window_width = attribs.width;
        window_height = attribs.height;

        static uint64_t tstart, t0, t1;
        static int timeSet = 0;
        if (!timeSet){
            tstart = get_time();
            t0 = tstart;
            timeSet = 1;
        }

        t1 = get_time();

        double dt = (double)(t1-t0) / 1000000000.0;
        int nFrames = MIN(TINY3D_AUDIO_BUFSZ,(int)(dt * TINY3D_SAMPLE_RATE));
        update((double)(t1-tstart) / 1000000000.0, dt, nFrames, audioBuf);
        if (nFrames){
            pa_simple_write(stream, audioBuf, nFrames*2*sizeof(*audioBuf), NULL);
        }
        
        t0 = t1;

        glXSwapBuffers(display, window);
    }
    EXIT:;
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}