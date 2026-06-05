#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace ltgui {

enum class ClipboardFormat { Text, RichText, Image, Files };

class ClipboardData {
public:
    void setText(const std::string& text) { text_ = text; }
    std::string text() const { return text_; }

    void setHtml(const std::string& html) { html_ = html; }
    std::string html() const { return html_; }

    void setImage(const uint8_t* rgba, int w, int h);
    bool hasImage() const { return !imageRGBA_.empty(); }
    const uint8_t* imageData() const { return imageRGBA_.data(); }
    int imageWidth() const { return imageW_; }
    int imageHeight() const { return imageH_; }

    void setFiles(const std::vector<std::string>& files) { files_ = files; }
    std::vector<std::string> files() const { return files_; }

    std::vector<ClipboardFormat> availableFormats() const;
    bool hasFormat(ClipboardFormat f) const;

private:
    std::string text_;
    std::string html_;
    std::vector<uint8_t> imageRGBA_;
    int imageW_ = 0, imageH_ = 0;
    std::vector<std::string> files_;
};

class Clipboard {
public:
    static std::string getText();
    static void setText(const std::string& text);

    static ClipboardData getData();
    static void setData(const ClipboardData& data);
    static std::vector<ClipboardFormat> availableFormats();
};

} // namespace ltgui
