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
uint32_t *load_image(bool flip_vertically, int *width, int *height, char *format, ...){}
int16_t *load_audio(int *nFrames, char *format, ...){}

#include <pulse/error.h>
#include <pulse/simple.h>

#if USE_GL
void open_window(int width, int height){
#else
void open_window(int scale){
	int width = SCREEN_WIDTH * scale;
	int height = SCREEN_HEIGHT * scale;
#endif
    Display *display = XOpenDisplay(NULL);
    int scr = DefaultScreen(display);
    int swidth = XDisplayWidth(display,scr);
    int sheight = XDisplayHeight(display,scr);
    Window window = XCreateSimpleWindow(display, RootWindow(display, scr), 0, 0, width, height, 0, 0, 0);
    XStoreName(display, window, "tiny3d");
    XSelectInput(display, window, KeyPressMask | KeyReleaseMask);
    int visual_hints[] = {
        GLX_RGBA,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER,
        None
    };
    XVisualInfo *visual_info = glXChooseVisual(display, scr, visual_hints);
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

        XWindowAttributes attribs;
        XGetWindowAttributes(display, window, &attribs);
        width = attribs.width;
        height = attribs.height;

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
        //printf("nFrames: %d\n",nFrames);
        #if USE_GL
            update((double)(t1-tstart) / 1000000000.0, dt, width, height, nFrames, audioBuf);
        #else
            update((double)(t1-tstart) / 1000000000.0, dt, nFrames, audioBuf);

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
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_BYTE, screen);
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

        if (nFrames){
            pa_simple_write(stream, audioBuf, nFrames*2*sizeof(*audioBuf), NULL);
        }
        
        t0 = t1;

        glXSwapBuffers(display, window);
    }
}