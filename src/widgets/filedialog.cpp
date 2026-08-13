#include "widgets/filedialog.h"
#include "widgets/listbox.h"
#include "widgets/textbox.h"
#include "widgets/combobox.h"
#include "widgets/button.h"
#include "layout.h"

#ifdef LTGUI_PLATFORM_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace ltgui {

FileDialog::FileDialog(Widget* parent) : Dialog(parent) {
    setTitle("Open File");
    panelW_ = 520;
    panelH_ = 380;
}

DialogResult FileDialog::exec() {
    buildCustomDialog();
    if (defaultPath_.empty()) defaultPath_ = ".";
    populateFileList(defaultPath_);
    return Dialog::exec();
}

std::string FileDialog::currentDir() const {
    return pathBar_ ? pathBar_->text() : defaultPath_;
}

std::string FileDialog::selectedPath() const {
    if (selection_.empty()) return {};
    return currentDir() + "/" + selection_[0];
}

void FileDialog::buildCustomDialog() {
    while (!children().empty())
        removeChild(children().back().get());
    panel_ = nullptr;

    panel_ = makeChild<Widget>();
    panel_->style().bgColor = Color::Transparent;
    panel_->style().borderWidth = 0;
    panel_->setLayout(std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 4, 8));

    pathBar_ = panel_->makeChild<TextBox>(defaultPath_);
    pathBar_->setMultiLine(false);

    fileList_ = panel_->makeChild<ListBox>();
    fileList_->onSelectionChanged.connect([this](int) { acceptSelection(); });

    if (!filters_.empty()) {
        filterCombo_ = panel_->makeChild<ComboBox>();
        for (auto& f : filters_) filterCombo_->addItem(f.name);
        filterCombo_->setCurrentIndex(0);
    }

    auto* btnRow = panel_->makeChild<Widget>();
    btnRow->style().bgColor = Color::Transparent;
    auto bl = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 0);
    bl->addStretch(1);
    btnRow->setLayout(std::move(bl));

    auto* ok = btnRow->makeChild<Button>("Open");
    ok->onClick([this]() { acceptSelection(); done(DialogResult::OK); });
    auto* cancel = btnRow->makeChild<Button>("Cancel");
    cancel->onClick([this]() { done(DialogResult::Cancel); });
}

void FileDialog::populateFileList(const std::string& dir) {
    if (!fileList_) return;
    fileList_->clear();

#ifdef LTGUI_PLATFORM_WINDOWS
    std::string searchPath = dir + "\\*";
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(searchPath.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        std::string name = fd.cFileName;
        if (name == "." || name == "..") continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) name = "[D] " + name;
        fileList_->addItem(name);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    (void)dir;
    fileList_->addItem("(platform scan not implemented)");
#endif
    if (pathBar_) pathBar_->setText(dir);
}

void FileDialog::acceptSelection() {
    if (!fileList_) return;
    int sel = fileList_->currentIndex();
    if (sel < 0) return;
    std::string item = fileList_->item(sel);
    // Remove [D] prefix if present
    bool isDir = (item.size() >= 4 && item[0] == '[' && item[1] == 'D' && item[2] == ']');
    std::string name = isDir ? item.substr(4) : item;
    selection_ = {name};

    if (isDir) {
        std::string newDir = currentDir() + "/" + name;
        populateFileList(newDir);
    }
}

} // namespace ltgui
