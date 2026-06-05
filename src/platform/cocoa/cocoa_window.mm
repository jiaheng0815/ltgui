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
    if (cppWindow_) cppWindow_->onClose();
    return NO;
}
- (void)windowDidResize:(NSNotification*)notification {
    if (!cppWindow_) return;
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
    if (self) {
        cppWindow_ = w;
        // Enable mouse-moved events for hover tracking.
        // Without a tracking area, mouseMoved: is never called
        // even if the window accepts mouse moved events.
        NSTrackingAreaOptions opts = NSTrackingMouseMoved
                                   | NSTrackingActiveInActiveApp
                                   | NSTrackingInVisibleRect;
        NSTrackingArea* area = [[NSTrackingArea alloc] initWithRect:NSZeroRect
                                                            options:opts
                                                              owner:self
                                                           userInfo:nil];
        [self addTrackingArea:area];
        [area release];
    }
    return self;
}
- (BOOL)isFlipped { return YES; }
- (void)drawRect:(NSRect)dirtyRect {
    if (cppWindow_) cppWindow_->onPaint();
}

- (void)mouseDown:(NSEvent*)event {
    if (!cppWindow_) return;
    ltgui::Event ev;
    ev.type = ltgui::EventType::MouseDown;
    ev.button = ltgui::MouseButton::Left;
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
    ev.pos = {static_cast<int>(p.x), static_cast<int>(p.y)};
    cppWindow_->onMouseEvent(ev);
}
- (void)mouseUp:(NSEvent*)event {
    if (!cppWindow_) return;
    ltgui::Event ev;
    ev.type = ltgui::EventType::MouseUp;
    ev.button = ltgui::MouseButton::Left;
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
    ev.pos = {static_cast<int>(p.x), static_cast<int>(p.y)};
    cppWindow_->onMouseEvent(ev);
}
- (void)rightMouseDown:(NSEvent*)event {
    if (!cppWindow_) return;
    ltgui::Event ev;
    ev.type = ltgui::EventType::MouseDown;
    ev.button = ltgui::MouseButton::Right;
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
    ev.pos = {static_cast<int>(p.x), static_cast<int>(p.y)};
    cppWindow_->onMouseEvent(ev);
}
- (void)rightMouseUp:(NSEvent*)event {
    if (!cppWindow_) return;
    ltgui::Event ev;
    ev.type = ltgui::EventType::MouseUp;
    ev.button = ltgui::MouseButton::Right;
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
    ev.pos = {static_cast<int>(p.x), static_cast<int>(p.y)};
    cppWindow_->onMouseEvent(ev);
}
- (void)mouseMoved:(NSEvent*)event {
    if (!cppWindow_) return;
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
    if (!cppWindow_) return;
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
    // Map Cocoa keyCode to ltgui Key enum
    ev.key = cppWindow_->mapCocoaKey(static_cast<int>([event keyCode]));
    // Map Cocoa modifier flags
    NSEventModifierFlags flags = [event modifierFlags];
    if (flags & NSEventModifierFlagShift)     ev.modifiers |= static_cast<int>(ltgui::KeyModifier::Shift);
    if (flags & NSEventModifierFlagControl)   ev.modifiers |= static_cast<int>(ltgui::KeyModifier::Control);
    if (flags & NSEventModifierFlagOption)    ev.modifiers |= static_cast<int>(ltgui::KeyModifier::Alt);
    if (flags & NSEventModifierFlagCommand)   ev.modifiers |= static_cast<int>(ltgui::KeyModifier::Super);
    cppWindow_->onKeyEvent(ev);
}
- (void)keyUp:(NSEvent*)event {
    ltgui::Event ev;
    ev.type = ltgui::EventType::KeyUp;
    // [event characters] is typically empty for KeyUp — leave charCode unset.
    ev.key = cppWindow_->mapCocoaKey(static_cast<int>([event keyCode]));
    NSEventModifierFlags flags = [event modifierFlags];
    if (flags & NSEventModifierFlagShift)     ev.modifiers |= static_cast<int>(ltgui::KeyModifier::Shift);
    if (flags & NSEventModifierFlagControl)   ev.modifiers |= static_cast<int>(ltgui::KeyModifier::Control);
    if (flags & NSEventModifierFlagOption)    ev.modifiers |= static_cast<int>(ltgui::KeyModifier::Alt);
    if (flags & NSEventModifierFlagCommand)   ev.modifiers |= static_cast<int>(ltgui::KeyModifier::Super);
    cppWindow_->onKeyEvent(ev);
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
    // Clear the C++ pointer in ObjC objects BEFORE destroying them
    // to prevent use-after-free from pending callbacks
    if (delegate_) {
        delegate_->cppWindow_ = nullptr;
    }
    if (view_) {
        view_->cppWindow_ = nullptr;
    }
    if (nsWindow_) {
        [nsWindow_ close];
        nsWindow_ = nullptr;
    }
    delegate_ = nullptr;
    view_ = nullptr;
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

void CocoaWindow::invalidate(const Rect& rect) {
    if (view_) {
        if (rect.width > 0 && rect.height > 0) {
            NSRect r = NSMakeRect(rect.x, rect.y, rect.width, rect.height);
            [view_ setNeedsDisplayInRect:r];
        } else {
            [view_ setNeedsDisplay:YES];
        }
    }
}

void* CocoaWindow::nativeHandle() const {
    return (__bridge void*)nsWindow_;
}

float CocoaWindow::dpiScale() const {
    @autoreleasepool {
        CGFloat sf = [[NSScreen mainScreen] backingScaleFactor];
        return sf > 0.0f ? (float)sf : 1.0f;
    }
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

void CocoaWindow::onKeyEvent(Event& ev) {
    if (eventCallback_) {
        eventCallback_(ev);
    }
}

bool CocoaWindow::setClipboardText(const std::string& text) {
    @autoreleasepool {
        NSPasteboard* pb = [NSPasteboard generalPasteboard];
        [pb clearContents];
        NSString* nsStr = [NSString stringWithUTF8String:text.c_str()];
        return [pb setString:nsStr forType:NSPasteboardTypeString];
    }
}

std::string CocoaWindow::getClipboardText() {
    @autoreleasepool {
        NSPasteboard* pb = [NSPasteboard generalPasteboard];
        NSString* nsStr = [pb stringForType:NSPasteboardTypeString];
        if (nsStr) {
            return [nsStr UTF8String];
        }
        return {};
    }
}

Key CocoaWindow::mapCocoaKey(int keyCode) const {
    // Map macOS virtual key codes to ltgui Key enum
    // Reference: <HIToolbox/Events.h> kVK_* constants
    switch (keyCode) {
    case 0:   return Key::A;
    case 1:   return Key::S;
    case 2:   return Key::D;
    case 3:   return Key::F;
    case 4:   return Key::H;
    case 5:   return Key::G;
    case 6:   return Key::Z;
    case 7:   return Key::X;
    case 8:   return Key::C;
    case 9:   return Key::V;
    case 11:  return Key::B;
    case 12:  return Key::Q;
    case 13:  return Key::W;
    case 14:  return Key::E;
    case 15:  return Key::R;
    case 16:  return Key::Y;
    case 17:  return Key::T;
    case 18:  return Key::Num1;
    case 19:  return Key::Num2;
    case 20:  return Key::Num3;
    case 21:  return Key::Num4;
    case 22:  return Key::Num6;
    case 23:  return Key::Num5;
    case 26:  return Key::Num9;
    case 28:  return Key::Num8;
    case 25:  return Key::Num7;
    case 29:  return Key::Num0;
    case 31:  return Key::O;
    case 32:  return Key::U;
    case 34:  return Key::I;
    case 35:  return Key::P;
    case 36:  return Key::Enter;
    case 37:  return Key::L;
    case 38:  return Key::J;
    case 40:  return Key::K;
    case 45:  return Key::N;
    case 46:  return Key::M;
    case 48:  return Key::Tab;
    case 49:  return Key::Space;
    case 51:  return Key::Backspace;
    case 53:  return Key::Escape;
    case 114: return Key::Insert;
    case 115: return Key::Home;
    case 116: return Key::PageUp;
    case 117: return Key::Delete;
    case 119: return Key::End;
    case 121: return Key::PageDown;
    case 122: return Key::F1;
    case 123: return Key::Left;
    case 124: return Key::Right;
    case 125: return Key::Down;
    case 126: return Key::Up;
    case 120: return Key::F2;
    case 99:  return Key::F3;
    case 118: return Key::F4;
    case 96:  return Key::F5;
    case 97:  return Key::F6;
    case 98:  return Key::F7;
    case 100: return Key::F8;
    case 101: return Key::F9;
    case 109: return Key::F10;
    case 103: return Key::F11;
    case 111: return Key::F12;
    default:  return Key::Unknown;
    }
}

} // namespace ltgui

#endif // LTGUI_PLATFORM_MACOS
