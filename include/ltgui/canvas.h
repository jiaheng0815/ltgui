#pragma once
#include "platform/native_canvas.h"

namespace ltgui {

// Canvas is a thin wrapper over NativeCanvas.
// Currently forwards directly; may add coordinate transforms and clipping later.
using Canvas = NativeCanvas;

} // namespace ltgui
