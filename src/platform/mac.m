#include <tiny3d.h>

#include <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>
#import <CoreVideo/CVDisplayLink.h>
#import <CoreText/CoreText.h>

#include <AudioToolbox/AudioQueue.h>
#include <AudioToolbox/ExtendedAudioFile.h>
struct fenster_audio {
	AudioQueueRef queue;
	size_t pos;
	int16_t buf[TINY3D_AUDIO_BUFSZ*2];
	dispatch_semaphore_t drained;
	dispatch_semaphore_t full;
} audioState;
static void fenster_audio_cb(void *p, AudioQueueRef q, AudioQueueBufferRef b) {
	struct fenster_audio *fa = (struct fenster_audio *)p;
	dispatch_semaphore_wait(fa->full, DISPATCH_TIME_FOREVER);
	memmove(b->mAudioData, fa->buf, sizeof(fa->buf));
	dispatch_semaphore_signal(fa->drained);
	AudioQueueEnqueueBuffer(q, b, 0, NULL);
}
int fenster_audio_open(struct fenster_audio *fa) {
	AudioStreamBasicDescription fmt = {0};
	fmt.mSampleRate = TINY3D_SAMPLE_RATE;
    fmt.mFormatID = kAudioFormatLinearPCM;
    fmt.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger;
    fmt.mBytesPerFrame = (16 * 2) / 8;
    fmt.mFramesPerPacket = 1;
    fmt.mBytesPerPacket = fmt.mFramesPerPacket * fmt.mBytesPerFrame;
    fmt.mChannelsPerFrame = 2;
    fmt.mBitsPerChannel = 16;
    fmt.mReserved = 0;
	fa->drained = dispatch_semaphore_create(1);
	fa->full = dispatch_semaphore_create(0);
	AudioQueueNewOutput(&fmt, fenster_audio_cb, fa, NULL, NULL, 0, &fa->queue);
	for (int i = 0; i < 2; i++) {
		AudioQueueBufferRef buffer = NULL;
		AudioQueueAllocateBuffer(fa->queue, sizeof(fa->buf), &buffer);
		buffer->mAudioDataByteSize = sizeof(fa->buf);
		memset(buffer->mAudioData, 0, buffer->mAudioDataByteSize);
		AudioQueueEnqueueBuffer(fa->queue, buffer, 0, NULL);
	}
	AudioQueueStart(fa->queue, NULL);
	return 0;
}
void fenster_audio_close(struct fenster_audio *fa) {
	AudioQueueStop(fa->queue, false);
	AudioQueueDispose(fa->queue, false);
}
int fenster_audio_available(struct fenster_audio *fa) {
	if (dispatch_semaphore_wait(fa->drained, DISPATCH_TIME_NOW))
		return 0;
	return TINY3D_AUDIO_BUFSZ - fa->pos;
}

#include <time.h>
uint64_t get_time(void){
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t) ts.tv_sec * 1000000000 + (uint64_t) ts.tv_nsec;
}

uint32_t *load_image(bool flip_vertically, int *width, int *height, char *format, ...){
	va_list args;
    va_start(args,format);
    assertPath = local_path_to_absolute_vararg(format,args);

	NSImage* img = [[NSImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String:assertPath]];
	ASSERT_FILE(img);
	if (img.representations && img.representations.count > 0) {
        long lastSquare = 0, curSquare;
        NSImageRep *imageRep;
        for (imageRep in img.representations) {
            curSquare = imageRep.pixelsWide * imageRep.pixelsHigh;
            if (curSquare > lastSquare) {
                img.size = NSMakeSize(imageRep.pixelsWide, imageRep.pixelsHigh);
                lastSquare = curSquare;
            }
        }
    }
	*width = (int)img.size.width;
	*height = (int)img.size.height;
	uint32_t *pixels = malloc((*width)*(*height)*sizeof(*pixels));
	ASSERT_FILE(pixels);

    CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
    CGContextRef ctx = CGBitmapContextCreate(
		pixels,
		*width,
		*height,
		8,
		(*width)*sizeof(*pixels),
		colorSpace,
		kCGImageAlphaPremultipliedLast
	);
	CGContextSetInterpolationQuality(ctx,kCGInterpolationNone);
    NSGraphicsContext* gctx = [NSGraphicsContext graphicsContextWithCGContext:ctx flipped:flip_vertically];
    [NSGraphicsContext setCurrentContext:gctx];
    [img drawInRect:NSMakeRect(0, 0, *width, *height)];
    [NSGraphicsContext setCurrentContext:nil];
    CGContextRelease(ctx);
    CGColorSpaceRelease(colorSpace);

	va_end(args);

	return pixels;
}
int16_t *load_audio(int *nFrames, char *format, ...){
	va_list args;
    va_start(args,format);
    assertPath = local_path_to_absolute_vararg(format,args);

    ExtAudioFileRef af = NULL;
    OSStatus err = ExtAudioFileOpenURL((__bridge CFURLRef)[NSURL URLWithString:[NSString stringWithUTF8String:assertPath]], &af);
    if(err != noErr){
		fatal_error("joj");
	}
	AudioStreamBasicDescription fmt = {0};
	fmt.mSampleRate = TINY3D_SAMPLE_RATE;
    fmt.mFormatID = kAudioFormatLinearPCM;
    fmt.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger;
    fmt.mBytesPerFrame = (16 * 2) / 8;
    fmt.mFramesPerPacket = 1;
    fmt.mBytesPerPacket = fmt.mFramesPerPacket * fmt.mBytesPerFrame;
    fmt.mChannelsPerFrame = 2;
    fmt.mBitsPerChannel = 16;
    fmt.mReserved = 0;
	err = ExtAudioFileSetProperty(af, kExtAudioFileProperty_ClientDataFormat, sizeof(fmt), &fmt);
    if(err != noErr){
		fatal_error("joj");
	}
	int bufferFrames = 4096;
    int16_t data[bufferFrames*2];
	AudioBuffer buffer = {
        .mNumberChannels = 2,
        .mDataByteSize = sizeof(data),
        .mData = data
    };
    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 1;
    bufferList.mBuffers[0] = buffer;
	*nFrames = 0;
	while (1){
		UInt32 ioFrames = bufferFrames; /* Request reading 4096 frames. */
		err = ExtAudioFileRead(af, &ioFrames, &bufferList);
		if(err != noErr){
			fatal_error("joj");
		}
		if (!ioFrames){
			break;
		}
		*nFrames += ioFrames;
	}
	ExtAudioFileSeek(af, 0);
	int16_t *frames = malloc((*nFrames)*2*sizeof(*frames));
	ASSERT_FILE(frames);
	buffer.mDataByteSize = (*nFrames)*2*sizeof(*frames);
	buffer.mData = frames;
	bufferList.mBuffers[0] = buffer; //have to do this again because this is probably a multi-value copy operation. Aren't high level langs great? /s
	{
		UInt32 ioFrames = *nFrames;
		err = ExtAudioFileRead(af, &ioFrames, &bufferList);
		if(err != noErr){
			fatal_error("joj");
		}
	}
	if(af)
    {
        err = ExtAudioFileDispose(af);
        if(err != noErr){
			fatal_error("joj");
		}
            // How do you handle an error from a dispose function?
    }

	va_end(args);

	return frames;
}
struct {
	uint32_t *pixels;
	int width, height;
	float r,g,b;
	int fontHeight;
} cgImg = {.fontHeight = 12};
void text_set_target_image(uint32_t *pixels, int width, int height){
	cgImg.pixels = pixels;
	cgImg.width = width;
	cgImg.height = height;
}
void text_set_font(char *ttfPathFormat, ...);
void text_set_font_height(int height){
	cgImg.fontHeight = height;
}
void text_set_color(float r, float g, float b){
	cgImg.r = r;
	cgImg.g = g;
	cgImg.b = b;
}
void text_draw(int left, int right, int bottom, int top, char *str){
	CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
    CGContextRef ctx = CGBitmapContextCreate(
		cgImg.pixels,
		cgImg.width,
		cgImg.height,
		8,
		(cgImg.width)*sizeof(*cgImg.pixels),
		colorSpace,
		kCGImageAlphaPremultipliedLast
	);
	CGContextSetInterpolationQuality(ctx,kCGInterpolationNone);
	
	CTFontRef font = CTFontCreateWithName(CFSTR("Comic Sans MS"), (float)cgImg.fontHeight, nil);
	NSDictionary* attributes = [NSDictionary dictionaryWithObjectsAndKeys:
                            (id)font, NSFontAttributeName,
                            [NSColor colorWithCalibratedRed:cgImg.r green:cgImg.g blue:cgImg.b alpha:1.0f], NSForegroundColorAttributeName,
                            nil];
	NSAttributedString* as = [[NSAttributedString alloc] initWithString:[NSString stringWithUTF8String:str] attributes:attributes];
	CFRelease(font);
	CTFramesetterRef framesetter = CTFramesetterCreateWithAttributedString((CFAttributedStringRef)as);
	CGRect rect = CGRectMake(left, bottom, right-left, top-bottom);
	CGPathRef path = CGPathCreateWithRect(rect, NULL);
	CTFrameRef frame = CTFramesetterCreateFrame(framesetter, CFRangeMake(0, 0), path, NULL);
	//CGContextSetRGBFillColor(ctx, 1.0, 1.0, 1.0, 1.0);
	//CGContextFillRect(ctx, CGRectMake(0.0, 0.0, cgImg.width, cgImg.height));
	CGContextSetTextMatrix(ctx, CGAffineTransformIdentity);
	CGContextTranslateCTM(ctx, 0, cgImg.height);
	CGContextScaleCTM(ctx, 1.0, -1.0);
	//CGContextSetTextPosition(ctx, 0, -14);
	CTFrameDraw(frame,ctx);

	CFRelease(frame);
	CFRelease(framesetter);
	CGPathRelease(path);

	CGContextRelease(ctx);
    CGColorSpaceRelease(colorSpace);
}

#if defined(MAC_OS_X_VERSION_10_12) && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
   static const NSAlertStyle kInformationalStyle = NSAlertStyleInformational;
   static const NSAlertStyle kWarningStyle = NSAlertStyleWarning;
   static const NSAlertStyle kCriticalStyle = NSAlertStyleCritical;
#else
   static const NSAlertStyle kInformationalStyle = NSInformationalAlertStyle;
   static const NSAlertStyle kWarningStyle = NSWarningAlertStyle;
   static const NSAlertStyle kCriticalStyle = NSCriticalAlertStyle;
#endif
void error_box(char *msg){
   NSAlert* alert = [[NSAlert alloc] init];

   [alert setMessageText:[NSString stringWithUTF8String:"Error"]];
   [alert setInformativeText:[NSString stringWithUTF8String:msg]];

   [alert setAlertStyle:kCriticalStyle];
   [alert addButtonWithTitle:@"Ok"];

   // Force the alert to appear on top of any other windows
   [[alert window] setLevel:NSModalPanelWindowLevel];

   [alert runModal];
   [alert release];
}

static bool mouse_locked = false;
bool is_mouse_locked(void){
	return mouse_locked;
}
void toggle_mouse_lock(){
	if (mouse_locked){
		[NSCursor unhide];
		CGAssociateMouseAndMouseCursorPosition(true);
	} else {
		[NSCursor hide];
		CGAssociateMouseAndMouseCursorPosition(false);
	}
	mouse_locked = !mouse_locked;
}

static int keycode_to_scancode(int keycode){ //credit chatgpt
    switch (keycode){
        case kVK_ANSI_A:              return 0x1E;
        case kVK_ANSI_S:              return 0x1F;
        case kVK_ANSI_D:              return 0x20;
        case kVK_ANSI_F:              return 0x21;
        case kVK_ANSI_H:              return 0x23;
        case kVK_ANSI_G:              return 0x22;
        case kVK_ANSI_Z:              return 0x2C;
        case kVK_ANSI_X:              return 0x2D;
        case kVK_ANSI_C:              return 0x2E;
        case kVK_ANSI_V:              return 0x2F;
        case kVK_ANSI_B:              return 0x30;
        case kVK_ANSI_Q:              return 0x10;
        case kVK_ANSI_W:              return 0x11;
        case kVK_ANSI_E:              return 0x12;
        case kVK_ANSI_R:              return 0x13;
        case kVK_ANSI_Y:              return 0x15;
        case kVK_ANSI_T:              return 0x14;
        case kVK_ANSI_1:              return 0x02;
        case kVK_ANSI_2:              return 0x03;
        case kVK_ANSI_3:              return 0x04;
        case kVK_ANSI_4:              return 0x05;
        case kVK_ANSI_6:              return 0x07;
        case kVK_ANSI_5:              return 0x06;
        case kVK_ANSI_Equal:          return 0x0D;
        case kVK_ANSI_9:              return 0x0A;
        case kVK_ANSI_7:              return 0x08;
        case kVK_ANSI_Minus:          return 0x0C;
        case kVK_ANSI_8:              return 0x09;
        case kVK_ANSI_0:              return 0x0B;
        case kVK_ANSI_RightBracket:   return 0x1B;
        case kVK_ANSI_O:              return 0x18;
        case kVK_ANSI_U:              return 0x16;
        case kVK_ANSI_LeftBracket:    return 0x1A;
        case kVK_ANSI_I:              return 0x17;
        case kVK_ANSI_P:              return 0x19;
        case kVK_ANSI_L:              return 0x26;
        case kVK_ANSI_J:              return 0x24;
        case kVK_ANSI_Quote:          return 0x28;
        case kVK_ANSI_K:              return 0x25;
        case kVK_ANSI_Semicolon:      return 0x27;
        case kVK_ANSI_Backslash:      return 0x2B;
        case kVK_ANSI_Comma:          return 0x33;
        case kVK_ANSI_Slash:          return 0x35;
        case kVK_ANSI_N:              return 0x31;
        case kVK_ANSI_M:              return 0x32;
        case kVK_ANSI_Period:         return 0x34;
        case kVK_ANSI_Grave:          return 0x29;
        case kVK_ANSI_KeypadDecimal:  return 0x53;
        case kVK_ANSI_KeypadMultiply: return 0x37;
        case kVK_ANSI_KeypadPlus:     return 0x4E;
        case kVK_ANSI_KeypadClear:    return 0x4A; // PC scancode for Num Lock
        case kVK_ANSI_KeypadDivide:   return 0xB5;
        case kVK_ANSI_KeypadEnter:    return 0x9C;
        case kVK_ANSI_KeypadMinus:    return 0x4A;
        case kVK_ANSI_KeypadEquals:   return 0x59;
        case kVK_ANSI_Keypad0:        return 0x52;
        case kVK_ANSI_Keypad1:        return 0x4F;
        case kVK_ANSI_Keypad2:        return 0x50;
        case kVK_ANSI_Keypad3:        return 0x51;
        case kVK_ANSI_Keypad4:        return 0x4B;
        case kVK_ANSI_Keypad5:        return 0x4C;
        case kVK_ANSI_Keypad6:        return 0x4D;
        case kVK_ANSI_Keypad7:        return 0x47;
        case kVK_ANSI_Keypad8:        return 0x48;
        case kVK_ANSI_Keypad9:        return 0x49;
        case kVK_Return:              return 0x1C;
        case kVK_Tab:                 return 0x0F;
        case kVK_Space:               return 0x39;
        case kVK_Delete:              return 0x0E;
        case kVK_Escape:              return 0x01;
        case kVK_Command:             return 0xE3; // PC scancode for left Meta
        case kVK_Shift:               return 0x2A;
        case kVK_CapsLock:            return 0x3A;
        case kVK_Option:              return 0x38;
        case kVK_Control:             return 0x1D;
        case kVK_RightShift:          return 0x36;
        case kVK_RightOption:         return 0xE038; // PC scancode for right Alt
        case kVK_RightControl:        return 0xE01D; // PC scancode for right Ctrl
        case kVK_Function:            return 0x63;
        case kVK_F17:                 return 0x64;
        case kVK_VolumeUp:            return 0x32; // not a standard PC scancode
        case kVK_VolumeDown:          return 0xE0; // not a standard PC scancode
        case kVK_Mute:                return 0x20; // not a standard PC scancode
        case kVK_F18:                 return 0x65;
        case kVK_F19:                 return 0x66;
        case kVK_F20:                 return 0x67;
        case kVK_F5:                  return 0x3F;
        case kVK_F6:                  return 0x40;
        case kVK_F7:                  return 0x41;
        case kVK_F3:                  return 0x3D;
        case kVK_F8:                  return 0x42;
        case kVK_F9:                  return 0x43;
        case kVK_F11:                 return 0x57;
        case kVK_F13:                 return 0x68;
        case kVK_F16:                 return 0x6A;
        case kVK_F14:                 return 0x69;
        case kVK_F10:                 return 0x44;
        case kVK_F12:                 return 0x58;
        case kVK_F15:                 return 0x6B;
        case kVK_Help:                return 0xE035; // PC scancode for Help
        case kVK_Home:                return 0xE047;
        case kVK_PageUp:              return 0xE049;
        case kVK_ForwardDelete:       return 0xE053;
        case kVK_F4:                  return 0x3E;
        case kVK_End:                 return 0xE04F;
        case kVK_F2:                  return 0x3C;
        case kVK_PageDown:            return 0xE051;
        case kVK_F1:                  return 0x3B;
        case kVK_LeftArrow:           return 0xE04B;
        case kVK_RightArrow:          return 0xE04D;
        case kVK_DownArrow:           return 0xE050;
        case kVK_UpArrow:             return 0xE048;
        default:                      return -1; // return -1 if keycode is not recognized
    }
}

@interface MyOpenGLView : NSOpenGLView
@end
@implementation MyOpenGLView
- (void)drawRect:(NSRect)dirtyRect {
	static uint64_t tstart, t0, t1;
	static int timeSet = 0;
	if (!timeSet){
		tstart = get_time();
		t0 = tstart;
		timeSet = 1;
	}

	//https://stackoverflow.com/questions/2440089/nsopenglview-resize-on-window-resize
	window_width = (int)[self convertRectToBacking:[self bounds]].size.width;
	window_height = (int)[self convertRectToBacking:[self bounds]].size.height;

	t1 = get_time();

	int nFrames = fenster_audio_available(&audioState);
	update((double)(t1-tstart) / 1000000000.0, (double)(t1-t0) / 1000000000.0, nFrames, audioState.buf+audioState.pos*2);

	audioState.pos += nFrames;
	if (audioState.pos >= TINY3D_AUDIO_BUFSZ){
		audioState.pos = 0;
		dispatch_semaphore_signal(audioState.full);
	}
	
	t0 = t1;

	[[self openGLContext] flushBuffer];
}
- (BOOL)acceptsFirstResponder {
	return YES;
}

- (void)mouseMoved:(NSEvent*) event {
	if (mouse_is_locked){
		mousemove([event deltaX],[event deltaY]);
	} else {
		NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
		mousemove(point.x,point.y);
	}
}

- (void) mouseDragged: (NSEvent*) event {
	NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
	NSLog(@"Mouse pos: %lf, %lf", point.x, point.y);
}

- (void)scrollWheel: (NSEvent*) event  {
	NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
	NSLog(@"Mouse wheel at: %lf, %lf. Delta: %lf", point.x, point.y, [event deltaY]);
}

- (void) mouseDown: (NSEvent*) event {
	keydown(KEY_MOUSE_LEFT);
	NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
	//NSLog(@"Left mouse down: %lf, %lf", point.x, point.y);
}

- (void) mouseUp: (NSEvent*) event {
	keyup(KEY_MOUSE_LEFT);
	NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
	//NSLog(@"Left mouse up: %lf, %lf", point.x, point.y);
}

- (void) rightMouseDown: (NSEvent*) event {
	keydown(KEY_MOUSE_RIGHT);
	NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
	//NSLog(@"Right mouse down: %lf, %lf", point.x, point.y);
}

- (void) rightMouseUp: (NSEvent*) event {
	keyup(KEY_MOUSE_RIGHT);
	NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
	//NSLog(@"Right mouse up: %lf, %lf", point.x, point.y);
}

- (void) keyDown: (NSEvent*) event {
	if ([event isARepeat] == NO) {
		keydown(keycode_to_scancode([event keyCode]));
	}
}

- (void) keyUp: (NSEvent*) event {
	keyup(keycode_to_scancode([event keyCode]));
}
@end

CVReturn displayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext)
{
	[(MyOpenGLView *)displayLinkContext setNeedsDisplay:YES];
	return kCVReturnSuccess;
}

static int swidth, sheight;
static char *local_name;

NSWindow* window;
MyOpenGLView* glView;

@interface AppDelegate : NSObject <NSApplicationDelegate> {
   
}
@end
@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	NSRect screenRect = [[NSScreen mainScreen] frame];
	NSRect viewRect = NSMakeRect(0, 0, swidth, sheight);
	NSRect windowRect = NSMakeRect(NSMidX(screenRect) - NSMidX(viewRect),
									NSMidY(screenRect) - NSMidY(viewRect),
									viewRect.size.width, 
									viewRect.size.height);
	window = [[NSWindow alloc]
		initWithContentRect:windowRect
		styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskResizable
		backing:NSBackingStoreBuffered
		defer:NO];
	[window setTitle:[NSString stringWithUTF8String:local_name]];
	[window setAcceptsMouseMovedEvents:YES];
	[window setMinSize:[window contentLayoutRect].size];

   // Create the OpenGL view
   NSOpenGLPixelFormatAttribute attrs[] = {
	  NSOpenGLPFADoubleBuffer,
	  NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy,
	  NSOpenGLPFAColorSize, 24,
	  NSOpenGLPFAAlphaSize, 8,
	  NSOpenGLPFADepthSize, 24,
	  NSOpenGLPFAStencilSize, 8,
	  0
   };
   NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
   glView = [[MyOpenGLView alloc] initWithFrame:viewRect pixelFormat:pixelFormat];
   [glView setWantsBestResolutionOpenGLSurface:YES];//https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/EnablingOpenGLforHighResolution/EnablingOpenGLforHighResolution.html
   [window setContentView:glView];

   // Show the window
   [window makeKeyAndOrderFront:nil];

   GLint swapInt = 1;
   CVDisplayLinkRef displayLink;
   [[glView openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval]; 
   CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
   CVDisplayLinkSetOutputCallback(displayLink, &displayLinkCallback, glView);
   CGLContextObj cglContext = [[glView openGLContext] CGLContextObj];
   CGLPixelFormatObj cglPixelFormat = [[NSOpenGLView defaultPixelFormat] CGLPixelFormatObj];
   CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);
   CVDisplayLinkStart(displayLink);

   //audio:
   fenster_audio_open(&audioState);

   init();
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
   return YES;
}
@end

static bool fullscreen = false;
bool is_fullscreen(){
	return fullscreen;
}
void toggle_fullscreen(){
	fullscreen = !fullscreen;
	[window toggleFullScreen:nil];
}

float get_dpi_scale(){
	return window.screen.backingScaleFactor;
}

void open_window(int min_width, int min_height, char *name){
	local_name = name;
	swidth = min_width;
	sheight = min_height;
	[NSAutoreleasePool new];
	NSApplication* app = [NSApplication sharedApplication];
	[app setActivationPolicy:NSApplicationActivationPolicyRegular];
	[app activateIgnoringOtherApps:YES];
	AppDelegate* appDelegate = [[AppDelegate alloc] init];
	[app setDelegate:appDelegate];
	[app activateIgnoringOtherApps:true];
	[app run];
}