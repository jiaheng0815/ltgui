#include "ltgui.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>

using namespace ltgui;

static int g_clickCount = 0;

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--debug") == 0) {
            Logger::instance().setGlobalDebug(true);
        }
    }

    Window window;
    if (!window.create(800, 600, "ltgui Showcase")) {
        std::cerr << "Failed to create window." << std::endl;
        return 1;
    }

    auto root = std::make_unique<Widget>();
    root->style().bgColor = currentTheme().bgPrimary;

    auto mainLayout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 0, 0);
    mainLayout->addStretch(0);
    mainLayout->addStretch(1);
    root->setLayout(std::move(mainLayout));

    // --- Header ---
    auto* header = root->makeChild<Widget>();
    header->style().bgColor = currentTheme().bgSecondary;

    auto headerLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 12);
    headerLayout->addStretch(0);
    headerLayout->addStretch(1);
    headerLayout->addStretch(0);

    auto* title = header->makeChild<Label>("ltgui Framework");
    title->style().font = Font("Segoe UI", 18, FontWeight::Bold);
    title->style().fgColor = currentTheme().accent;

    // Theme toggle button
    bool darkTheme = false;
    auto* themeBtn = header->makeChild<Button>("Dark Theme");
    themeBtn->onClicked.connect([&]() {
        darkTheme = !darkTheme;
        setTheme(darkTheme ? Theme::Dark() : Theme::Light());
        themeBtn->setText(darkTheme ? "Light Theme" : "Dark Theme");
    });

    header->addChild(std::make_unique<Widget>()); // spacer
    header->setLayout(std::move(headerLayout));

    // --- Tab Widget ---
    auto* tabs = root->makeChild<TabWidget>();

    // ===== Tab 1: Widgets =====
    int tab1 = tabs->addTab("Widgets");
    auto* w1 = tabs->tabContent(tab1);
    auto w1Layout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 8, 12);

    // Buttons section
    auto* btnSection = w1->makeChild<Widget>();
    btnSection->style().bgColor = Color::Transparent;
    auto btnSectionLayout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 6, 0);

    auto* btnTitle = btnSection->makeChild<Label>("Buttons");
    btnTitle->style().font = Font("Segoe UI", 14, FontWeight::SemiBold);

    auto* btnRow = btnSection->makeChild<Widget>();
    btnRow->style().bgColor = Color::Transparent;
    auto btnRowLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 0);

    auto* normalBtn = btnRow->makeChild<Button>("Normal Button");
    auto* primaryBtn = btnRow->makeChild<Button>("Primary Action");
    primaryBtn->style().bgColor = currentTheme().accent;
    primaryBtn->style().fgColor = Color::White;

    auto* counterBtn = btnRow->makeChild<Button>("Clicked: 0 times");
    counterBtn->onClicked.connect([&]() {
        g_clickCount++;
        counterBtn->setText("Clicked: " + std::to_string(g_clickCount) + " times");
    });

    btnRow->setLayout(std::move(btnRowLayout));

    btnSection->setLayout(std::move(btnSectionLayout));

    // Input section
    auto* inputSection = w1->makeChild<Widget>();
    inputSection->style().bgColor = Color::Transparent;
    auto inputSectionLayout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 6, 0);

    auto* inputTitle = inputSection->makeChild<Label>("Input Controls");
    inputTitle->style().font = Font("Segoe UI", 14, FontWeight::SemiBold);

    auto* inputRow = inputSection->makeChild<Widget>();
    inputRow->style().bgColor = Color::Transparent;
    auto inputRowLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 0);

    auto* nameLabel = inputRow->makeChild<Label>("Name:");
    auto* nameBox = inputRow->makeChild<TextBox>("Type your name...");

    auto* cb1 = inputRow->makeChild<CheckBox>("Accept terms");
    auto* cb2 = inputRow->makeChild<CheckBox>("Subscribe");

    inputRowLayout->addStretch(0);
    inputRowLayout->addStretch(1);
    inputRowLayout->addStretch(0);
    inputRowLayout->addStretch(0);
    inputRow->setLayout(std::move(inputRowLayout));

    inputSection->setLayout(std::move(inputSectionLayout));

    // Radio + select section
    auto* selectSection = w1->makeChild<Widget>();
    selectSection->style().bgColor = Color::Transparent;
    auto selectSectionLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 16, 0);

    auto* rbGroup = selectSection->makeChild<Widget>();
    rbGroup->style().bgColor = Color::Transparent;
    auto rbLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 0);
    auto* rb1 = rbGroup->makeChild<RadioButton>("Red");
    auto* rb2 = rbGroup->makeChild<RadioButton>("Green");
    auto* rb3 = rbGroup->makeChild<RadioButton>("Blue");
    rb1->setChecked(true);
    rbGroup->setLayout(std::move(rbLayout));

    auto* combo = selectSection->makeChild<ComboBox>();
    combo->addItem("Small");
    combo->addItem("Medium");
    combo->addItem("Large");
    combo->addItem("X-Large");
    combo->setCurrentIndex(1);

    auto* comboLabel = selectSection->makeChild<Label>("Size: Medium");
    combo->onSelectionChanged.connect([&](int index) {
        (void)index;
        comboLabel->setText("Size: " + combo->currentText());
    });

    selectSection->setLayout(std::move(selectSectionLayout));

    // Slider + Progress
    auto* progSection = w1->makeChild<Widget>();
    progSection->style().bgColor = Color::Transparent;
    auto progSectionLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 12, 0);

    auto* progLabel = progSection->makeChild<Label>("Progress:");
    auto* slider = progSection->makeChild<Slider>();
    slider->setRange(0, 100);
    slider->setValue(60);
    auto* slValue = progSection->makeChild<Label>(" 60%");
    auto* progress = progSection->makeChild<ProgressBar>();
    progress->setValue(60);
    slider->onValueChanged.connect([&](int v) {
        // Fixed-width: "  0%", " 50%", "100%" so layout doesn't resize slider
        if (v >= 100) slValue->setText(std::to_string(v) + "%");
        else if (v >= 10) slValue->setText(" " + std::to_string(v) + "%");
        else slValue->setText("  " + std::to_string(v) + "%");
        progress->setValue(v);
    });

    // Stretch factors: label(0), slider(1), value(0), progress(1)
    progSectionLayout->addStretch(0);
    progSectionLayout->addStretch(1);
    progSectionLayout->addStretch(0);
    progSectionLayout->addStretch(1);
    progSection->setLayout(std::move(progSectionLayout));

    // Assemble tab 1
    w1Layout->addStretch(0);
    w1Layout->addStretch(0);
    w1Layout->addStretch(0);
    w1Layout->addStretch(1);
    w1->setLayout(std::move(w1Layout));

    // ===== Tab 2: List & Tree =====
    int tab2 = tabs->addTab("List & Tree");
    auto* w2 = tabs->tabContent(tab2);
    auto w2Layout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 12);

    // ListBox
    auto* listCol = w2->makeChild<Widget>();
    listCol->style().bgColor = Color::Transparent;
    auto listColLayout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 4, 0);

    auto* listTitle = listCol->makeChild<Label>("ListBox");
    listTitle->style().font = Font("Segoe UI", 14, FontWeight::SemiBold);

    auto* listBox = listCol->makeChild<ListBox>();
    listBox->addItem("Apple");
    listBox->addItem("Banana");
    listBox->addItem("Cherry");
    listBox->addItem("Date");
    listBox->addItem("Elderberry");
    listBox->addItem("Fig");
    listBox->addItem("Grapefruit");
    listBox->addItem("Honeydew");
    listBox->addItem("Kiwi");
    listBox->addItem("Lemon");
    listBox->setCurrentIndex(2);

    listColLayout->addStretch(0);
    listColLayout->addStretch(1);
    listCol->setLayout(std::move(listColLayout));

    // TreeView
    auto* treeCol = w2->makeChild<Widget>();
    treeCol->style().bgColor = Color::Transparent;
    auto treeColLayout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 4, 0);

    auto* treeTitle = treeCol->makeChild<Label>("TreeView");
    treeTitle->style().font = Font("Segoe UI", 14, FontWeight::SemiBold);

    auto* treeView = treeCol->makeChild<TreeView>();
    auto* treeRoot = treeView->rootItem();
    treeRoot->setExpanded(true);

    auto* fruit = treeRoot->addChild("Fruits");
    fruit->addChild("Apple");
    fruit->addChild("Banana");
    fruit->addChild("Cherry");
    fruit->setExpanded(true);

    auto* veg = treeRoot->addChild("Vegetables");
    veg->addChild("Carrot");
    veg->addChild("Broccoli");
    auto* leafy = veg->addChild("Leafy Greens");
    leafy->addChild("Spinach");
    leafy->addChild("Kale");
    leafy->addChild("Lettuce");
    veg->addChild("Tomato");

    auto* grain = treeRoot->addChild("Grains");
    grain->addChild("Rice");
    grain->addChild("Wheat");
    grain->addChild("Oats");

    treeColLayout->addStretch(0);
    treeColLayout->addStretch(1);
    treeCol->setLayout(std::move(treeColLayout));

    w2Layout->addStretch(1);
    w2Layout->addStretch(1);
    w2->setLayout(std::move(w2Layout));

    // ===== Tab 3: About =====
    int tab3 = tabs->addTab("About");
    auto* w3 = tabs->tabContent(tab3);
    auto w3Layout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 12, 20);

    auto* aboutTitle = w3->makeChild<Label>("About ltgui");
    aboutTitle->style().font = Font("Segoe UI", 22, FontWeight::Bold);
    aboutTitle->style().fgColor = currentTheme().accent;

    auto* aboutText = w3->makeChild<Label>("A from-scratch cross-platform retained-mode C++ GUI framework.");
    aboutText->style().font = Font("Segoe UI", 13);

    auto* aboutFeatures = w3->makeChild<Label>(
        "Features:\n"
        "  - Zero external dependencies (beyond platform APIs)\n"
        "  - Cross-platform: Windows (GDI+), Linux (X11/Xft), macOS (Cocoa)\n"
        "  - Retained-mode widget tree with BoxLayout & GridLayout\n"
        "  - Full widget set: Button, Label, TextBox, CheckBox, RadioButton,\n"
        "    Slider, ListBox, ComboBox, ScrollArea, TabWidget, TreeView, ProgressBar\n"
        "  - Animation system with easing curves\n"
        "  - Light & dark themes\n"
        "  - Smooth hover, press, and scroll animations");
    aboutFeatures->style().font = Font("Segoe UI", 12);
    aboutFeatures->style().fgColor = currentTheme().textSecondary;

    w3Layout->addStretch(0);
    w3Layout->addStretch(0);
    w3Layout->addStretch(1);
    w3->setLayout(std::move(w3Layout));

    window.setCentralWidget(std::move(root));
    window.show();

    return Application::instance().run();
}
