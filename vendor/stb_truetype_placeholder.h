// Placeholder: to enable GPU font rendering, download the real stb_truetype.h
// from https://github.com/nothings/stb/blob/master/stb_truetype.h
// and replace this file.
//
// Without stb_truetype:
//   - drawText() fills a solid rectangle as placeholder
//   - measureText() returns estimated size based on character count
//
// GPU rendering (fillRect, roundedRect, ellipse, drawImage, etc.)
// works fully without stb_truetype.
//
// Download command:
//   curl -sL https://raw.githubusercontent.com/nothings/stb/master/stb_truetype.h -o vendor/stb_truetype.h

#pragma once
// stb_truetype.h not installed — GPU text rendering will use placeholders
#define LTGUI_NO_STB_TRUETYPE
