#pragma once
#include "widgets/dialog.h"
#include <string>
#include <vector>

namespace ltgui {

class ListBox;
class TextBox;
class ComboBox;

enum class FileDialogMode { OpenFile, OpenMultiple, SaveFile, SelectFolder };

struct FileFilter { std::string name; std::string pattern; };

class FileDialog : public Dialog {
public:
    explicit FileDialog(Widget* parent = nullptr);

    void setMode(FileDialogMode mode) { mode_ = mode; }
    void setDefaultPath(const std::string& path) { defaultPath_ = path; }
    void addFilter(const FileFilter& f) { filters_.push_back(f); }
    void clearFilters() { filters_.clear(); }

    std::string selectedPath() const;
    std::vector<std::string> selectedPaths() const { return selection_; }

    DialogResult exec() override;

    LTGUI_DECLARE_WIDGET_TYPE(FileDialog)

private:
    FileDialogMode mode_ = FileDialogMode::OpenFile;
    std::string defaultPath_;
    std::vector<FileFilter> filters_;
    std::vector<std::string> selection_;
    ListBox* fileList_ = nullptr;
    TextBox* pathBar_ = nullptr;
    ComboBox* filterCombo_ = nullptr;

    void buildCustomDialog();
    void populateFileList(const std::string& dir);
    void acceptSelection();
    std::string currentDir() const;
};

} // namespace ltgui
