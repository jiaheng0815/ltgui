#include "dragdrop.h"

namespace ltgui {

bool DragData::hasFormat(const std::string& mime) const {
    return data_.count(mime) > 0;
}

std::string DragData::text() const {
    auto it = data_.find("text/plain");
    if (it != data_.end() && !it->second.empty())
        return std::string(it->second.begin(), it->second.end());
    return {};
}

void DragData::setData(const std::string& mime, const std::vector<uint8_t>& d) {
    data_[mime] = d;
}

std::vector<uint8_t> DragData::data(const std::string& mime) const {
    auto it = data_.find(mime);
    if (it != data_.end()) return it->second;
    return {};
}

void DragData::setFiles(const std::vector<std::string>& paths) {
    std::string uriList;
    for (auto& p : paths) {
        if (!uriList.empty()) uriList += "\r\n";
        uriList += "file://" + p;
    }
    setData("text/uri-list", std::vector<uint8_t>(uriList.begin(), uriList.end()));
}

std::vector<std::string> DragData::files() const {
    auto it = data_.find("text/uri-list");
    if (it == data_.end()) return {};
    std::string uriList(it->second.begin(), it->second.end());
    std::vector<std::string> result;
    size_t pos = 0;
    while (pos < uriList.size()) {
        size_t end = uriList.find("\r\n", pos);
        if (end == std::string::npos) {
            // Last entry (no trailing \r\n) — also handle \n-only separators
            end = uriList.find('\n', pos);
            if (end == std::string::npos) end = uriList.size();
        }
        std::string uri = uriList.substr(pos, end - pos);
        // Handle file://, file://localhost/, and file:/// URI schemes
        if (uri.compare(0, 7, "file://") == 0) {
            uri = uri.substr(7);
            // Strip localhost prefix if present
            if (uri.compare(0, 10, "localhost/") == 0 || uri.compare(0, 10, "localhost") == 0) {
                uri = uri.substr(uri[9] == '/' ? 10 : 9);
            }
        }
        if (!uri.empty()) {
            result.push_back(uri);
        }
        pos = end;
        // Skip the separator
        if (pos < uriList.size() && uriList[pos] == '\r') pos++;
        if (pos < uriList.size() && uriList[pos] == '\n') pos++;
        if (end == uriList.size()) break;
    }
    return result;
}

std::vector<std::string> DragData::formats() const {
    std::vector<std::string> keys;
    for (auto& kv : data_) keys.push_back(kv.first);
    return keys;
}

// --- DropTarget ---

bool DropTarget::acceptsMime(const std::string& mime) const {
    if (acceptedTypes_.empty()) return true;
    for (auto& t : acceptedTypes_)
        if (t == mime) return true;
    return false;
}

void DropTarget::handleDrop(const DragData& data) {
    if (dropCb_) dropCb_(data);
}

bool DropTarget::handleDragOver(const DragData& data) {
    if (dragOverCb_) return dragOverCb_(data);
    return true;
}

} // namespace ltgui
