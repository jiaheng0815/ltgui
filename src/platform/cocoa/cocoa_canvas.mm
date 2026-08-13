#include "platform/cocoa/cocoa_canvas.h"

#ifdef LTGUI_PLATFORM_MACOS

#import <Cocoa/Cocoa.h>
#include <CoreText/CoreText.h>

namespace ltgui {

CocoaCanvas::CocoaCanvas(void *nsView) : nsView_(nsView) {}

CocoaCanvas::~CocoaCanvas() {
  if (backbuffer_) {
    CGContextRelease(static_cast<CGContextRef>(backbuffer_));
    backbuffer_ = nullptr;
  }
  if (currentFont_) {
    CFRelease(currentFont_);
    currentFont_ = nullptr;
  }
}

void CocoaCanvas::resize(int width, int height) {
  if (width <= 0 || height <= 0)
    return;

  if (backbuffer_) {
    CGContextRelease(static_cast<CGContextRef>(backbuffer_));
    backbuffer_ = nullptr;
  }

  canvasWidth_ = width;
  canvasHeight_ = height;

  // Create bitmap context as backbuffer — pass NULL so CG allocates its own
  // buffer
  int bytesPerRow = width * 4;
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  backbuffer_ = CGBitmapContextCreate(
      nullptr, width, height, 8, bytesPerRow, colorSpace,
      kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
  CGColorSpaceRelease(colorSpace);
}

void CocoaCanvas::beginPaint() {
  if (backbuffer_) {
    CGContextRef ctx = static_cast<CGContextRef>(backbuffer_);
    // Clear to white
    CGContextSetRGBFillColor(ctx, 1, 1, 1, 1);
    CGContextFillRect(ctx, CGRectMake(0, 0, canvasWidth_, canvasHeight_));
  }
}

void CocoaCanvas::endPaint() {
  if (!backbuffer_ || !nsView_)
    return;

  NSView *view = (__bridge NSView *)nsView_;
  CGContextRef viewCtx = [[NSGraphicsContext currentContext] CGContext];

  // Note: endPaint is called from drawRect:, so [NSGraphicsContext
  // currentContext] should be valid. If not, we can't draw.
  if (viewCtx) {
    CGImageRef img =
        CGBitmapContextCreateImage(static_cast<CGContextRef>(backbuffer_));
    if (img) {
      CGContextDrawImage(viewCtx, CGRectMake(0, 0, canvasWidth_, canvasHeight_),
                         img);
      CGImageRelease(img);
    }
  }
}

void CocoaCanvas::setColor(const Color &color) {
  currentR_ = color.r;
  currentG_ = color.g;
  currentB_ = color.b;
  currentA_ = color.a;
}

void CocoaCanvas::setFont(const Font &font) {
  currentFontDesc_ = font;

  if (currentFont_) {
    CFRelease(currentFont_);
    currentFont_ = nullptr;
  }

  NSString *family = [NSString stringWithUTF8String:font.family.c_str()];
  CGFloat size = font.size;

  // Weight
  NSString *weightStr = @"Regular";
  if (static_cast<int>(font.weight) >= static_cast<int>(FontWeight::Bold)) {
    weightStr = @"Bold";
  }

  // Use font descriptor for better matching
  NSFont *nsFont = [NSFont
      fontWithName:[NSString stringWithFormat:@"%@-%@", family, weightStr]
              size:size];
  if (!nsFont) {
    nsFont = [NSFont fontWithName:family size:size];
  }
  if (!nsFont) {
    nsFont = [NSFont systemFontOfSize:size];
  }

  currentFont_ = (__bridge_retained NSFont *)nsFont;
}

void CocoaCanvas::fillRect(const Rect &rect) {
  CGContextRef ctx = static_cast<CGContextRef>(backbuffer_);
  if (!ctx)
    return;

  CGContextSetRGBFillColor(ctx, currentR_ / 255.0f, currentG_ / 255.0f,
                           currentB_ / 255.0f, currentA_ / 255.0f);
  CGContextFillRect(ctx, CGRectMake(rect.x, rect.y, rect.width, rect.height));
}

void CocoaCanvas::strokeRect(const Rect &rect, int lineWidth) {
  CGContextRef ctx = static_cast<CGContextRef>(backbuffer_);
  if (!ctx)
    return;

  CGContextSetRGBStrokeColor(ctx, currentR_ / 255.0f, currentG_ / 255.0f,
                             currentB_ / 255.0f, currentA_ / 255.0f);
  CGContextSetLineWidth(ctx, lineWidth);
  CGContextStrokeRect(ctx, CGRectMake(rect.x, rect.y, rect.width, rect.height));
}

void CocoaCanvas::drawText(const std::string &text, const Rect &rect,
                           int flags) {
  CGContextRef ctx = static_cast<CGContextRef>(backbuffer_);
  if (!ctx || !currentFont_)
    return;

  NSFont *font = (__bridge NSFont *)currentFont_;
  NSString *nsStr = [NSString stringWithUTF8String:text.c_str()];
  if (!nsStr || [nsStr length] == 0)
    return;

  // Create attributed string
  NSDictionary *attrs = @{
    NSFontAttributeName : font,
    NSForegroundColorAttributeName :
        [NSColor colorWithCalibratedRed:currentR_ / 255.0f
                                  green:currentG_ / 255.0f
                                   blue:currentB_ / 255.0f
                                  alpha:currentA_ / 255.0f]
  };
  NSAttributedString *attrStr =
      [[NSAttributedString alloc] initWithString:nsStr attributes:attrs];

  // Create CTLine for single-line rendering
  CTLineRef line =
      CTLineCreateWithAttributedString((__bridge CFAttributedStringRef)attrStr);

  // Measure
  CGRect lineBounds =
      CTLineGetBoundsWithOptions(line, kCTLineBoundsUseGlyphPathBounds);
  CGFloat textW = lineBounds.size.width;
  CGFloat textH = lineBounds.size.height;

  // Position
  CGFloat tx = rect.x;
  CGFloat ty = rect.y;

  if (flags & AlignCenter) {
    tx += (rect.width - textW) / 2.0f;
  } else if (flags & AlignRight) {
    tx += rect.width - textW;
  }

  if (flags & AlignBottom) {
    ty += rect.height - textH;
  } else if (flags & AlignVCenter) {
    ty += (rect.height - textH) / 2.0f;
  }

  // Draw
  CGContextSaveGState(ctx);
  CGContextSetTextMatrix(ctx, CGAffineTransformIdentity);
  CGContextTranslateCTM(ctx, tx, ty + textH);
  CGContextScaleCTM(ctx, 1.0f, -1.0f);
  CTLineDraw(line, ctx);
  CGContextRestoreGState(ctx);

  CFRelease(line);
}

void CocoaCanvas::drawLine(const Point &p1, const Point &p2, int lineWidth) {
  CGContextRef ctx = static_cast<CGContextRef>(backbuffer_);
  if (!ctx)
    return;

  CGContextSetRGBStrokeColor(ctx, currentR_ / 255.0f, currentG_ / 255.0f,
                             currentB_ / 255.0f, currentA_ / 255.0f);
  CGContextSetLineWidth(ctx, lineWidth);
  CGContextMoveToPoint(ctx, p1.x, p1.y);
  CGContextAddLineToPoint(ctx, p2.x, p2.y);
  CGContextStrokePath(ctx);
}

void CocoaCanvas::fillEllipse(const Rect &rect) {
  CGContextRef ctx = static_cast<CGContextRef>(backbuffer_);
  if (!ctx)
    return;

  CGContextSetRGBFillColor(ctx, currentR_ / 255.0f, currentG_ / 255.0f,
                           currentB_ / 255.0f, currentA_ / 255.0f);
  CGContextFillEllipseInRect(
      ctx, CGRectMake(rect.x, rect.y, rect.width, rect.height));
}

void CocoaCanvas::strokeEllipse(const Rect &rect, int lineWidth) {
  CGContextRef ctx = static_cast<CGContextRef>(backbuffer_);
  if (!ctx)
    return;

  CGContextSetRGBStrokeColor(ctx, currentR_ / 255.0f, currentG_ / 255.0f,
                             currentB_ / 255.0f, currentA_ / 255.0f);
  CGContextSetLineWidth(ctx, lineWidth);
  CGContextStrokeEllipseInRect(
      ctx, CGRectMake(rect.x, rect.y, rect.width, rect.height));
}

static void addRoundedRectPath(CGContextRef ctx, const CGRect &rect,
                               CGFloat radius) {
  CGFloat r =
      std::min(radius, std::min(rect.size.width, rect.size.height) / 2.0f);
  CGFloat x = rect.origin.x, y = rect.origin.y;
  CGFloat w = rect.size.width, h = rect.size.height;
  CGFloat d = r * 2.0f;
  CGContextBeginPath(ctx);
  CGContextAddArc(ctx, x + w - r, y + r, r, -M_PI_2, 0, 0);
  CGContextAddArc(ctx, x + w - r, y + h - r, r, 0, M_PI_2, 0);
  CGContextAddArc(ctx, x + r, y + h - r, r, M_PI_2, M_PI, 0);
  CGContextAddArc(ctx, x + r, y + r, r, M_PI, 3.0f * M_PI_2, 0);
  CGContextClosePath(ctx);
}

void CocoaCanvas::fillRoundedRect(const Rect &rect, int radius) {
  CGContextRef ctx = static_cast<CGContextRef>(backbuffer_);
  if (!ctx)
    return;
  CGContextSetRGBFillColor(ctx, currentR_ / 255.0f, currentG_ / 255.0f,
                           currentB_ / 255.0f, currentA_ / 255.0f);
  addRoundedRectPath(ctx, CGRectMake(rect.x, rect.y, rect.width, rect.height),
                     radius);
  CGContextFillPath(ctx);
}

void CocoaCanvas::strokeRoundedRect(const Rect &rect, int radius,
                                    int lineWidth) {
  CGContextRef ctx = static_cast<CGContextRef>(backbuffer_);
  if (!ctx)
    return;
  CGContextSetRGBStrokeColor(ctx, currentR_ / 255.0f, currentG_ / 255.0f,
                             currentB_ / 255.0f, currentA_ / 255.0f);
  CGContextSetLineWidth(ctx, lineWidth);
  addRoundedRectPath(ctx, CGRectMake(rect.x, rect.y, rect.width, rect.height),
                     radius);
  CGContextStrokePath(ctx);
}

Size CocoaCanvas::measureText(const std::string &text) {
  NSFont *font = (__bridge NSFont *)currentFont_;
  if (!font) {
    font = [NSFont systemFontOfSize:12];
  }

  NSString *nsStr = [NSString stringWithUTF8String:text.c_str()];
  if (!nsStr)
    return {0, 0};

  NSDictionary *attrs = @{NSFontAttributeName : font};
  NSSize size = [nsStr sizeWithAttributes:attrs];
  return {static_cast<int>(ceil(size.width)),
          static_cast<int>(ceil(size.height))};
}

} // namespace ltgui

#endif // LTGUI_PLATFORM_MACOS
