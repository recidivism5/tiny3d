#include <tiny3d.h>

#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>
#import <CoreVideo/CVDisplayLink.h>

#include <time.h>
uint64_t get_time(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t) ts.tv_sec * 1000000000 + (uint64_t) ts.tv_nsec;
}

/*@interface MyOpenGLView : NSOpenGLView
@end
@implementation MyOpenGLView
- (void)drawRect:(NSRect)dirtyRect {
   static uint64_t tstart, t0, t1;
   static GLuint texture = 0;
   if (!texture){
      tstart = get_time();
      t0 = tstart;

      glGenTextures(1,&texture);
      glBindTexture(GL_TEXTURE_2D,texture);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   }
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_BYTE, screen);

   t1 = get_time();
   update((double)(t1-tstart) / 1000000000.0, (double)(t1-t0) / 1000000000.0);
   t0 = t1;
   
   //https://stackoverflow.com/questions/2440089/nsopenglview-resize-on-window-resize
   int width = (int)[[self superview] bounds].size.width;
   int height = (int)[[self superview] bounds].size.height;
   glViewport(0,0,width,height);
   
   glClearColor(0.0f,0.0f,1.0f,1.0f);
   glClear(GL_COLOR_BUFFER_BIT);

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

   [[self openGLContext] flushBuffer];
}
@end

CVReturn displayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext)
{
   NSAutoreleasePool* pool = [NSAutoreleasePool new];
    if ([self lockFocusIfCanDraw])
    {
        //Your drawing code
        [self unlockFocus];
    }
    [pool drain];

    return kCVReturnSuccess;
}

@interface AppDelegate : NSObject <NSApplicationDelegate> {
   NSWindow* window;
   MyOpenGLView* glView;
}
@end
@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
   // Create the window
   window = [[NSWindow alloc]
      initWithContentRect:NSMakeRect(0, 0, 640, 480)
      styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskResizable
      backing:NSBackingStoreBuffered
      defer:NO];
   [window setTitle:@"OpenGL Window"];

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
   glView = [[MyOpenGLView alloc] initWithFrame:NSMakeRect(0, 0, 640, 480) pixelFormat:pixelFormat];
   [window setContentView:glView];

   // Show the window
   [window makeKeyAndOrderFront:nil];
   [window center];

   GLint swapInt = 1;
   CVDisplayLinkRef displayLink;
   [[glView openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval]; 
   CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
   CVDisplayLinkSetOutputCallback(displayLink, &displayLinkCallback, self);
   CGLContextObj cglContext = [[glView openGLContext] CGLContextObj];
   CGLPixelFormatObj cglPixelFormat = [[NSOpenGLView defaultPixelFormat] CGLPixelFormatObj];
   CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);
   CVDisplayLinkStart(displayLink);
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
   return YES;
}
@end

void open_window(int scale){
   [NSAutoreleasePool new];
   NSApplication* app = [NSApplication sharedApplication];
   [app setActivationPolicy:NSApplicationActivationPolicyRegular];
   [app activateIgnoringOtherApps:YES];
   AppDelegate* appDelegate = [[AppDelegate alloc] init];
   [app setDelegate:appDelegate];
   [app activateIgnoringOtherApps:true];
   [app run];
}*/

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

@class View;
static CVReturn GlobalDisplayLinkCallback(CVDisplayLinkRef, const CVTimeStamp*, const CVTimeStamp*, CVOptionFlags, CVOptionFlags*, void*);

@interface View : NSOpenGLView <NSWindowDelegate> {
@public
	CVDisplayLinkRef displayLink;
	bool running;
	NSRect windowRect;
	NSRecursiveLock* appLock;
}    
@end

@implementation View
// Initialize
- (id) initWithFrame: (NSRect) frame {
	running = true;
	
	// No multisampling
	int samples = 0;

	// Keep multisampling attributes at the start of the attribute lists since code below assumes they are array elements 0 through 4.
	NSOpenGLPixelFormatAttribute windowedAttrs[] = 
	{
		NSOpenGLPFAMultisample,
		NSOpenGLPFASampleBuffers, samples ? 1 : 0,
		NSOpenGLPFASamples, samples,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAColorSize, 32,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy,
		0
	};

	// Try to choose a supported pixel format
	NSOpenGLPixelFormat* pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:windowedAttrs];

	if (!pf) {
		bool valid = false;
		while (!pf && samples > 0) {
			samples /= 2;
			windowedAttrs[2] = samples ? 1 : 0;
			windowedAttrs[4] = samples;
			pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:windowedAttrs];
			if (pf) {
				valid = true;
				break;
			}
		}
		
		if (!valid) {
			NSLog(@"OpenGL pixel format not supported.");
			return nil;
		}
	}
	
	self = [super initWithFrame:frame pixelFormat:[pf autorelease]];
	appLock = [[NSRecursiveLock alloc] init];

	return self;
}

- (void) prepareOpenGL {
	[super prepareOpenGL];
		
	[[self window] setLevel: NSNormalWindowLevel];
	[[self window] makeKeyAndOrderFront: self];
	
	// Make all the OpenGL calls to setup rendering and build the necessary rendering objects
	[[self openGLContext] makeCurrentContext];
	// Synchronize buffer swaps with vertical refresh rate
	GLint swapInt = 1; // Vsynch on!
	[[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
	
	// Create a display link capable of being used with all active displays
	CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
	
	// Set the renderer output callback function
	CVDisplayLinkSetOutputCallback(displayLink, &GlobalDisplayLinkCallback, self);
	
	CGLContextObj cglContext = (CGLContextObj)[[self openGLContext] CGLContextObj];
	CGLPixelFormatObj cglPixelFormat = (CGLPixelFormatObj)[[self pixelFormat] CGLPixelFormatObj];
	CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);
	
	GLint dim[2] = {windowRect.size.width, windowRect.size.height};
	CGLSetParameter(cglContext, kCGLCPSurfaceBackingSize, dim);
	CGLEnable(cglContext, kCGLCESurfaceBackingSize);
	
	[appLock lock];
	CGLLockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);
	NSLog(@"Initialize");

	NSLog(@"GL version:   %s", glGetString(GL_VERSION));
    NSLog(@"GLSL version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
	// Temp
	glClearColor(0.5f, 0.6f, 0.7f, 1.0f);
	glViewport(0, 0, windowRect.size.width, windowRect.size.height);
	glEnable(GL_DEPTH_TEST);
	// End temp
	CGLUnlockContext((CGLContextObj)[[self openGLContext] CGLContextObj]); 
	[appLock unlock];

	// Activate the display link
	CVDisplayLinkStart(displayLink);
}

// Tell the window to accept input events
- (BOOL)acceptsFirstResponder {
	return YES;
}

- (void)mouseMoved:(NSEvent*) event {
	[appLock lock];
	NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
	NSLog(@"Mouse pos: %lf, %lf", point.x, point.y);
	[appLock unlock];
}

- (void) mouseDragged: (NSEvent*) event {
	[appLock lock];
	NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
	NSLog(@"Mouse pos: %lf, %lf", point.x, point.y);
	[appLock unlock];
}

- (void)scrollWheel: (NSEvent*) event  {
	[appLock lock];
	NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
	NSLog(@"Mouse wheel at: %lf, %lf. Delta: %lf", point.x, point.y, [event deltaY]);
	[appLock unlock];
}

- (void) mouseDown: (NSEvent*) event {
	[appLock lock];
	NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
	NSLog(@"Left mouse down: %lf, %lf", point.x, point.y);
	[appLock unlock];
}

- (void) mouseUp: (NSEvent*) event {
	[appLock lock];
	NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
	NSLog(@"Left mouse up: %lf, %lf", point.x, point.y);
	[appLock unlock];
}

- (void) rightMouseDown: (NSEvent*) event {
	[appLock lock];
	NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
	NSLog(@"Right mouse down: %lf, %lf", point.x, point.y);
	[appLock unlock];
}

- (void) rightMouseUp: (NSEvent*) event {
	[appLock lock];
	NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
	NSLog(@"Right mouse up: %lf, %lf", point.x, point.y);
	[appLock unlock];
}

- (void)otherMouseDown: (NSEvent*) event {
	[appLock lock];
	NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
	NSLog(@"Middle mouse down: %lf, %lf", point.x, point.y);
	[appLock unlock];
}

- (void)otherMouseUp: (NSEvent*) event {
	[appLock lock];
	NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
	NSLog(@"Middle mouse up: %lf, %lf", point.x, point.y);
	[appLock unlock];
}

- (void) mouseEntered: (NSEvent*)event {
	[appLock lock];
	NSLog(@"Mouse entered");
	[appLock unlock];
}

- (void) mouseExited: (NSEvent*)event {
	[appLock lock];
	NSLog(@"Mouse left");
	[appLock unlock];
}

- (void) keyDown: (NSEvent*) event {
	[appLock lock];
	if ([event isARepeat] == NO) {
		NSLog(@"Key down: %d", [event keyCode]);
	}
	[appLock unlock];
}

- (void) keyUp: (NSEvent*) event {
	[appLock lock];
	NSLog(@"Key up: %d", [event keyCode]);
	[appLock unlock];
}

// Update
- (CVReturn) getFrameForTime:(const CVTimeStamp*)outputTime {
	[appLock lock];
	
	[[self openGLContext] makeCurrentContext];
	CGLLockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);

	NSLog(@"Update");
	// Temp
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// EndTemp

	CGLFlushDrawable((CGLContextObj)[[self openGLContext] CGLContextObj]);
   CGLUnlockContext((CGLContextObj)[[self openGLContext] CGLContextObj]); 

   if (false) { // Update loop returns false
      [NSApp terminate:self];
	}

	[appLock unlock];
	
	return kCVReturnSuccess;
}

// Resize
- (void)windowDidResize:(NSNotification*)notification {
	NSSize size = [ [ _window contentView ] frame ].size;
	[appLock lock];
	[[self openGLContext] makeCurrentContext];
	CGLLockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);
	NSLog(@"Window resize: %lf, %lf", size.width, size.height);
	// Temp
	windowRect.size.width = size.width;
	windowRect.size.height = size.height;
	glViewport(0, 0, windowRect.size.width, windowRect.size.height);
	// End temp
	CGLUnlockContext((CGLContextObj)[[self openGLContext] CGLContextObj]); 
	[appLock unlock];
}

- (void)resumeDisplayRenderer  {
    [appLock lock];
    CVDisplayLinkStop(displayLink);
    [appLock unlock]; 
}

- (void)haltDisplayRenderer  {
    [appLock lock];
    CVDisplayLinkStop(displayLink);
    [appLock unlock];
}

// Terminate window when the red X is pressed
-(void)windowWillClose:(NSNotification *)notification {
	if (running) {
		running = false;

		[appLock lock];
		NSLog(@"Cleanup");

		CVDisplayLinkStop(displayLink);
		CVDisplayLinkRelease(displayLink);

		[appLock unlock];
	}

	[NSApp terminate:self];
}

// Cleanup
- (void) dealloc {  
	[appLock release]; 
	[super dealloc];
}
@end

static CVReturn GlobalDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext) {
	CVReturn result = [(View*)displayLinkContext getFrameForTime:outputTime];
	return result;
}

void open_window(int scale){ 
	// Autorelease Pool: 
	// Objects declared in this scope will be automatically 
	// released at the end of it, when the pool is "drained". 
	NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init]; 
 
	// Create a shared app instance. 
	// This will initialize the global variable 
	// 'NSApp' with the application instance. 
	[NSApplication sharedApplication];
 
	// Create a window: 
 
	// Style flags 
	NSUInteger windowStyle = NSTitledWindowMask  | NSClosableWindowMask | NSResizableWindowMask | NSMiniaturizableWindowMask; 

	// Window bounds (x, y, width, height) 
	NSRect screenRect = [[NSScreen mainScreen] frame];
	NSRect viewRect = NSMakeRect(0, 0, 800, 600); 
	NSRect windowRect = NSMakeRect(NSMidX(screenRect) - NSMidX(viewRect),
								 NSMidY(screenRect) - NSMidY(viewRect),
								 viewRect.size.width, 
								 viewRect.size.height);

	NSWindow * window = [[NSWindow alloc] initWithContentRect:windowRect 
						styleMask:windowStyle 
						backing:NSBackingStoreBuffered 
						defer:NO]; 
	[window autorelease]; 
 
	// Window controller 
	NSWindowController * windowController = [[NSWindowController alloc] initWithWindow:window]; 
	[windowController autorelease]; 

	// Since Snow Leopard, programs without application bundles and Info.plist files don't get a menubar 
	// and can't be brought to the front unless the presentation option is changed
	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
   [NSApp activateIgnoringOtherApps:YES];
	
	// Next, we need to create the menu bar. You don't need to give the first item in the menubar a name 
	// (it will get the application's name automatically)
	id menubar = [[NSMenu new] autorelease];
	id appMenuItem = [[NSMenuItem new] autorelease];
	[menubar addItem:appMenuItem];
	[NSApp setMainMenu:menubar];

	// Then we add the quit item to the menu. Fortunately the action is simple since terminate: is 
	// already implemented in NSApplication and the NSApplication is always in the responder chain.
	id appMenu = [[NSMenu new] autorelease];
	id appName = [[NSProcessInfo processInfo] processName];
	id quitTitle = [@"Quit " stringByAppendingString:appName];
	id quitMenuItem = [[[NSMenuItem alloc] initWithTitle:quitTitle
		action:@selector(terminate:) keyEquivalent:@"q"] autorelease];
	[appMenu addItem:quitMenuItem];
	[appMenuItem setSubmenu:appMenu];

	// Create app delegate to handle system events
	View* view = [[[View alloc] initWithFrame:windowRect] autorelease];
	view->windowRect = windowRect;
	[window setAcceptsMouseMovedEvents:YES];
	[window setContentView:view];
	[window setDelegate:view];

	// Set app title
	[window setTitle:appName];

	// Add fullscreen button
	[window setCollectionBehavior: NSWindowCollectionBehaviorFullScreenPrimary];

	// Show window and run event loop 
	[window orderFrontRegardless]; 
	[NSApp run]; 
	
	[pool drain];
}