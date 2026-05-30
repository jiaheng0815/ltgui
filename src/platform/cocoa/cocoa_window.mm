#include "platform/cocoa/cocoa_window.h"

#ifdef LTGUI_PLATFORM_MACOS

#include "platform/cocoa/cocoa_canvas.h"
#import <Cocoa/Cocoa.h>

// Forward declare the ltgui event type in ObjC
namespace ltgui { struct Event; }

// --- CocoaWindowDelegate ---
@interface CocoaWindowDelegate : NSObject <NSWindowDelegate> {
@public
    ltgui::CocoaWindow* cppWindow_;
}
- (instancetype)initWithCppWindow:(ltgui::CocoaWindow*)w;
@end

@implementation CocoaWindowDelegate
- (instancetype)initWithCppWindow:(ltgui::CocoaWindow*)w {
    self = [super init];
    if (self) cppWindow_ = w;
    return self;
}
- (BOOL)windowShouldClose:(NSWindow*)sender {
    cppWindow_->onClose();
    return NO;
}
- (void)windowDidResize:(NSNotification*)notification {
    NSWindow* w = [notification object];
    NSRect frame = [w contentRectForFrameRect:[w frame]];
    cppWindow_->onResize(static_cast<int>(frame.size.width),
                          static_cast<int>(frame.size.height));
}
@end

// --- CocoaView ---
@interface CocoaView : NSView {
@public
    ltgui::CocoaWindow* cppWindow_;
}
- (instancetype)initWithCppWindow:(ltgui::CocoaWindow*)w;
@end

@implementation CocoaView
- (instancetype)initWithCppWindow:(ltgui::CocoaWindow*)w {
    self = [super init];
    if (self) cppWindow_ = w;
    return self;
}
- (BOOL)isFlipped { return YES; }
- (void)drawRect:(NSRect)dirtyRect {
    cppWindow_->onPaint();
}

- (void)mouseDown:(NSEvent*)event {
    ltgui::Event ev;
    ev.type = ltgui::EventType::MouseDown;
    ev.button = ltgui::MouseButton::Left;
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
    ev.pos = {static_cast<int>(p.x), static_cast<int>(p.y)};
    cppWindow_->onMouseEvent(ev);
}
- (void)mouseUp:(NSEvent*)event {
    ltgui::Event ev;
    ev.type = ltgui::EventType::MouseUp;
    ev.button = ltgui::MouseButton::Left;
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
    ev.pos = {static_cast<int>(p.x), static_cast<int>(p.y)};
    cppWindow_->onMouseEvent(ev);
}
- (void)rightMouseDown:(NSEvent*)event {
    ltgui::Event ev;
    ev.type = ltgui::EventType::MouseDown;
    ev.button = ltgui::MouseButton::Right;
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
    ev.pos = {static_cast<int>(p.x), static_cast<int>(p.y)};
    cppWindow_->onMouseEvent(ev);
}
- (void)rightMouseUp:(NSEvent*)event {
    ltgui::Event ev;
    ev.type = ltgui::EventType::MouseUp;
    ev.button = ltgui::MouseButton::Right;
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
    ev.pos = {static_cast<int>(p.x), static_cast<int>(p.y)};
    cppWindow_->onMouseEvent(ev);
}
- (void)mouseMoved:(NSEvent*)event {
    ltgui::Event ev;
    ev.type = ltgui::EventType::MouseMove;
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
    ev.pos = {static_cast<int>(p.x), static_cast<int>(p.y)};
    cppWindow_->onMouseEvent(ev);
}
- (void)mouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}
- (void)scrollWheel:(NSEvent*)event {
    ltgui::Event ev;
    ev.type = ltgui::EventType::MouseWheel;
    ev.wheelDelta = static_cast<int>([event deltaY]);
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
    ev.pos = {static_cast<int>(p.x), static_cast<int>(p.y)};
    cppWindow_->onMouseEvent(ev);
}
- (void)keyDown:(NSEvent*)event {
    ltgui::Event ev;
    ev.type = ltgui::EventType::KeyDown;
    NSString* chars = [event characters];
    if ([chars length] > 0) {
        ev.charCode = [chars characterAtIndex:0];
    }
    cppWindow_->onMouseEvent(ev);
}
- (void)keyUp:(NSEvent*)event {
    ltgui::Event ev;
    ev.type = ltgui::EventType::KeyUp;
    cppWindow_->onMouseEvent(ev);
}
@end

namespace ltgui {

CocoaWindow::CocoaWindow() {
    // Ensure NSApplication exists
    if (![NSApplication sharedApplication]) {
        [NSApplication sharedApplication];
    }
}

CocoaWindow::~CocoaWindow() {
    destroy();
    delete canvas_;
    canvas_ = nullptr;
}

bool CocoaWindow::create(int width, int height, const std::string& title) {
    if (nsWindow_) return true;

    NSRect frame = NSMakeRect(0, 0, width, height);

    NSUInteger style = NSWindowStyleMaskTitled |
                       NSWindowStyleMaskClosable |
                       NSWindowStyleMaskMiniaturizable |
                       NSWindowStyleMaskResizable;

    nsWindow_ = [[NSWindow alloc] initWithContentRect:frame
                                            styleMask:style
                                              backing:NSBackingStoreBuffered
                                                defer:NO];

    NSString* nsTitle = [NSString stringWithUTF8String:title.c_str()];
    [nsWindow_ setTitle:nsTitle];

    // Create delegate
    delegate_ = [[CocoaWindowDelegate alloc] initWithCppWindow:this];
    [nsWindow_ setDelegate:delegate_];

    // Create custom view
    view_ = [[CocoaView alloc] initWithCppWindow:this];
    [nsWindow_ setContentView:view_];
    [nsWindow_ makeFirstResponder:view_];
    [nsWindow_ setAcceptsMouseMovedEvents:YES];

    size_.width = width;
    size_.height = height;

    canvas_ = new CocoaCanvas((__bridge void*)view_);
    canvas_->resize(width, height);

    return true;
}

void CocoaWindow::destroy() {
    if (nsWindow_) {
        [nsWindow_ close];
        nsWindow_ = nullptr;
    }
    if (delegate_) {
        delegate_ = nullptr;
    }
    if (view_) {
        view_ = nullptr;
    }
}

void CocoaWindow::show() {
    if (nsWindow_) {
        [nsWindow_ makeKeyAndOrderFront:nil];
    }
}

void CocoaWindow::hide() {
    if (nsWindow_) {
        [nsWindow_ orderOut:nil];
    }
}

void CocoaWindow::close() {
    if (nsWindow_) {
        [nsWindow_ performClose:nil];
    }
}

void CocoaWindow::setTitle(const std::string& title) {
    if (nsWindow_) {
        NSString* t = [NSString stringWithUTF8String:title.c_str()];
        [nsWindow_ setTitle:t];
    }
}

void CocoaWindow::setSize(int width, int height) {
    if (nsWindow_) {
        NSRect frame = [nsWindow_ frame];
        frame.size = NSMakeSize(width, height);
        [nsWindow_ setFrame:frame display:YES];
    }
}

Size CocoaWindow::getSize() const {
    return size_;
}

void CocoaWindow::invalidate(const Rect& /*rect*/) {
    if (view_) {
        [view_ setNeedsDisplay:YES];
    }
}

void* CocoaWindow::nativeHandle() const {
    return (__bridge void*)nsWindow_;
}

NativeCanvas* CocoaWindow::getCanvas() {
    return canvas_;
}

void CocoaWindow::onPaint() {
    if (!eventCallback_) return;
    Event ev;
    ev.type = EventType::Paint;
    ev.width = size_.width;
    ev.height = size_.height;
    eventCallback_(ev);
}

void CocoaWindow::onResize(int width, int height) {
    size_.width = width;
    size_.height = height;
    if (canvas_) canvas_->resize(width, height);

    if (!eventCallback_) return;
    Event ev;
    ev.type = EventType::Resize;
    ev.width = width;
    ev.height = height;
    eventCallback_(ev);
}

void CocoaWindow::onClose() {
    if (!eventCallback_) return;
    Event ev;
    ev.type = EventType::Close;
    eventCallback_(ev);
}

void CocoaWindow::onMouseEvent(Event& ev) {
    if (eventCallback_) {
        eventCallback_(ev);
    }
}

} // namespace ltgui

#endif // LTGUI_PLATFORM_MACOS
