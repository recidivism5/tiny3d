#include <tiny3d.h>
#include <stdio.h>
#include <Cocoa/Cocoa.h>
#include <Metal/Metal.h>
#include <CoreVideo/CVDisplayLink.h>
#include "ShaderInterface.h"
#include <time.h>

uint64_t get_time(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t) ts.tv_sec * 1000000000 + (uint64_t) ts.tv_nsec;
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

///////////////////////////////////////////////////////////////////////
// Application / Window Delegate

@interface OSX_MainDelegate: NSObject<NSApplicationDelegate, NSWindowDelegate>
{
@public bool isRunning;
@public bool windowWasResized;
}
@end

@implementation OSX_MainDelegate
// NSApplicationDelegate methods
// NSWindowDelegate methods
- (NSSize)windowWillResize:(NSWindow*)window toSize:(NSSize)frameSize
{ 
    windowWasResized = true;
    return frameSize;
}
- (void)windowWillClose:(id)sender 
{ 
    isRunning = false; 
}
@end

// Application / Window Delegate
///////////////////////////////////////////////////////////////////////

static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    [(MyMetalView *)displayLinkContext setNeedsDisplay:YES];
    return kCVReturnSuccess;
}

void open_window(int scale)
{
    NSApplication* app = [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];

    // Create a Main Delegate to receive callbacks from App and Window
    OSX_MainDelegate* osxMainDelegate = [OSX_MainDelegate new];
    app.delegate = osxMainDelegate;

    [[NSFileManager defaultManager] changeCurrentDirectoryPath:[NSBundle mainBundle].bundlePath];

    // Create and open an NSWindow
    const float INITIAL_WIN_WIDTH = 1024;
    const float INITIAL_WIN_HEIGHT = 768;
    NSRect screenRect = [NSScreen mainScreen].frame;
    NSRect initialFrame = NSMakeRect((screenRect.size.width - INITIAL_WIN_WIDTH) * 0.5f,
                                    (screenRect.size.height - INITIAL_WIN_HEIGHT) * 0.5f,
                                    INITIAL_WIN_WIDTH, INITIAL_WIN_HEIGHT);

    NSWindowStyleMask windowStyleMask = (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable 
                                        | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable);
    NSWindow* mainWindow = [[NSWindow alloc] initWithContentRect:initialFrame
                                             styleMask:windowStyleMask
                                             backing:NSBackingStoreBuffered
                                             defer:NO];

    mainWindow.backgroundColor = NSColor.purpleColor;
    mainWindow.contentAspectRatio = NSMakeSize(4,3);
    mainWindow.minSize = NSMakeSize(400,300);
    mainWindow.title = @"Hello Metal Textured Quad";
    mainWindow.delegate = osxMainDelegate;
    mainWindow.contentView.wantsLayer = YES;
    [mainWindow makeKeyAndOrderFront: nil];

    id<MTLDevice> mtlDevice = MTLCreateSystemDefaultDevice();
    printf("System default GPU: %s\n", mtlDevice.name.UTF8String);

    // Create CAMetalLayer and add to mainWindow
    CAMetalLayer* caMetalLayer = [CAMetalLayer new];
    caMetalLayer.frame = mainWindow.contentView.frame;
    caMetalLayer.device = mtlDevice;
    caMetalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    [mainWindow.contentView.layer addSublayer:caMetalLayer];

	CVDisplayLinkRef displayLink;
	CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
	CVDisplayLinkSetOutputCallback(displayLink, &MyDisplayLinkCallback, mainWindow.contentView);
	CVDisplayLinkStart(displayLink);

    [NSApp finishLaunching];
    
    // Load shaders
    NSError* error = nil;
    id<MTLLibrary> mtlLibrary = [mtlDevice newLibraryWithFile: @"shaders.metallib" error:&error];
    if (!mtlLibrary) {
        fatal_error("Failed to load library. Error: %s\n", error.localizedDescription.UTF8String);
    }
    id<MTLFunction> vertFunc = [mtlLibrary newFunctionWithName:@"vert"];
    id<MTLFunction> fragFunc = [mtlLibrary newFunctionWithName:@"frag"];
    [mtlLibrary release];

    // Create Vertex Buffer
    float vertexData[] = { // x, y, u, v
        -0.5f,  0.5f, 0.f, 0.f,
        -0.5f, -0.5f, 0.f, 1.f,
         0.5f, -0.5f, 1.f, 1.f,
        -0.5f,  0.5f, 0.f, 0.f,
         0.5f, -0.5f, 1.f, 1.f,
         0.5f,  0.5f, 1.f, 0.f
    };

    id<MTLBuffer> quadVertexBuffer = [mtlDevice newBufferWithBytes:vertexData 
                                            length:sizeof(vertexData)
                                            options:MTLResourceOptionCPUCacheModeDefault];

    // Create Texture
    MTLTextureDescriptor* mtlTextureDescriptor =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                              width:SCREEN_WIDTH
                              height:SCREEN_HEIGHT
                              mipmapped:NO];
    id<MTLTexture> mtlTexture = [mtlDevice newTextureWithDescriptor:mtlTextureDescriptor];
    [mtlTextureDescriptor release];

    // Copy loaded image into MTLTextureObject
    [mtlTexture replaceRegion:MTLRegionMake2D(0,0,SCREEN_WIDTH,SCREEN_HEIGHT)
                mipmapLevel:0
                withBytes:screen
                bytesPerRow:SCREEN_WIDTH*sizeof(*screen)];


    // Create a Sampler State
    MTLSamplerDescriptor* mtlSamplerDesc = [MTLSamplerDescriptor new];
    mtlSamplerDesc.minFilter = MTLSamplerMinMagFilterNearest;
    mtlSamplerDesc.magFilter = MTLSamplerMinMagFilterNearest;
    id<MTLSamplerState> mtlSamplerState = [mtlDevice newSamplerStateWithDescriptor:mtlSamplerDesc];
    [mtlSamplerDesc release];

    // Create vertex descriptor
    MTLVertexDescriptor* vertDesc = [MTLVertexDescriptor new];
    vertDesc.attributes[VertexAttributeIndex_Position].format = MTLVertexFormatFloat2;
    vertDesc.attributes[VertexAttributeIndex_Position].offset = 0;
    vertDesc.attributes[VertexAttributeIndex_Position].bufferIndex = VertexBufferIndex_Attributes;
    vertDesc.attributes[VertexAttributeIndex_TexCoords].format = MTLVertexFormatFloat2;
    vertDesc.attributes[VertexAttributeIndex_TexCoords].offset = 2 * sizeof(float);
    vertDesc.attributes[VertexAttributeIndex_TexCoords].bufferIndex = VertexBufferIndex_Attributes;
    vertDesc.layouts[VertexBufferIndex_Attributes].stride = 4 * sizeof(float);
    vertDesc.layouts[VertexBufferIndex_Attributes].stepRate = 1;
    vertDesc.layouts[VertexBufferIndex_Attributes].stepFunction = MTLVertexStepFunctionPerVertex;

    // Create Render Pipeline State
    MTLRenderPipelineDescriptor* mtlRenderPipelineDesc = [MTLRenderPipelineDescriptor new];
    mtlRenderPipelineDesc.vertexFunction = vertFunc;
    mtlRenderPipelineDesc.fragmentFunction = fragFunc;
    mtlRenderPipelineDesc.vertexDescriptor = vertDesc;
    mtlRenderPipelineDesc.colorAttachments[0].pixelFormat = caMetalLayer.pixelFormat;
    id<MTLRenderPipelineState> mtlRenderPipelineState = [mtlDevice newRenderPipelineStateWithDescriptor:mtlRenderPipelineDesc error:&error];
    if (!mtlRenderPipelineState) {
        fatal_error("Failed to create pipeline state. Error: %s\n", error.localizedDescription.UTF8String);
    }

    [vertFunc release];
    [fragFunc release];
    [vertDesc release];
    [mtlRenderPipelineDesc release];

    id<MTLCommandQueue> mtlCommandQueue = [mtlDevice newCommandQueue];

    // Main Loop
    osxMainDelegate->isRunning = true;
    while(osxMainDelegate->isRunning) 
    {
        @autoreleasepool 
        {
			NSEvent *event;
            while(event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                        untilDate:nil
                                        inMode:NSDefaultRunLoopMode
                                        dequeue:YES])
            {
                switch(event.type)
                {
                    case NSEventTypeKeyDown:
                    {
                        if(event.keyCode == 0x35) //Escape key
                            osxMainDelegate->isRunning = false;
                        
                        break;
                    }
                    default: [NSApp sendEvent:event];
                }
            }
        } // autoreleasepool

        @autoreleasepool {
        if(osxMainDelegate->windowWasResized)
        {
            caMetalLayer.frame = mainWindow.contentView.frame;
            caMetalLayer.drawableSize = caMetalLayer.frame.size;
            osxMainDelegate->windowWasResized = false;
        }

        id<CAMetalDrawable> caMetalDrawable = [caMetalLayer nextDrawable];
        if(!caMetalDrawable) continue;

		t1 = get_time();
        update((double)(t1-tstart) / 1000000000.0, (double)(t1-t0) / 1000000000.0);
        t0 = t1;
		[mtlTexture replaceRegion:MTLRegionMake2D(0,0,SCREEN_WIDTH,SCREEN_HEIGHT)
                mipmapLevel:0
                withBytes:screen
                bytesPerRow:SCREEN_WIDTH*sizeof(*screen)];

        MTLRenderPassDescriptor* mtlRenderPassDescriptor = [MTLRenderPassDescriptor new];
        mtlRenderPassDescriptor.colorAttachments[0].texture = caMetalDrawable.texture;
        mtlRenderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        mtlRenderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        mtlRenderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.1, 0.2, 0.6, 1.0);

        id<MTLCommandBuffer> mtlCommandBuffer = [mtlCommandQueue commandBuffer];

        id<MTLRenderCommandEncoder> mtlRenderCommandEncoder = 
            [mtlCommandBuffer renderCommandEncoderWithDescriptor:mtlRenderPassDescriptor];
        [mtlRenderPassDescriptor release];

        [mtlRenderCommandEncoder setViewport:(MTLViewport){0, 0, 
                                                           caMetalLayer.drawableSize.width,
                                                           caMetalLayer.drawableSize.height,
                                                           0, 1}];
        [mtlRenderCommandEncoder setRenderPipelineState:mtlRenderPipelineState];
        [mtlRenderCommandEncoder setVertexBuffer:quadVertexBuffer offset:0 atIndex:VertexBufferIndex_Attributes];
        [mtlRenderCommandEncoder setFragmentTexture:mtlTexture atIndex:0];
        [mtlRenderCommandEncoder setFragmentSamplerState:mtlSamplerState atIndex:0];
        [mtlRenderCommandEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
        [mtlRenderCommandEncoder endEncoding];

        [mtlCommandBuffer presentDrawable:caMetalDrawable];
        [mtlCommandBuffer commit];

        } // autoreleasepool
    }
}