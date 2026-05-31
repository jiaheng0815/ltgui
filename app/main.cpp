#include "ltgui.h"
#include <iostream>
#include <cstdlib>
#include <ctime>

using namespace ltgui;

static int g_clickCount = 0;

int main() {
    Window window;
    if (!window.create(800, 600, "ltgui Showcase")) {
        std::cerr << "Failed to create window." << std::endl;
        return 1;
    }

    auto* root = new Widget();
    root->style().bgColor = currentTheme().bgPrimary;

    auto* mainLayout = new BoxLayout(BoxLayout::TopToBottom, 0, 0);
    mainLayout->addStretch(0);
    mainLayout->addStretch(1);
    root->setLayout(mainLayout);

    // --- Header ---
    auto* header = new Widget();
    header->style().bgColor = currentTheme().bgSecondary;

    auto* headerLayout = new BoxLayout(BoxLayout::LeftToRight, 8, 12);
    headerLayout->addStretch(0);
    headerLayout->addStretch(1);
    headerLayout->addStretch(0);

    auto* title = new Label("ltgui Framework");
    title->style().font = Font("Segoe UI", 18, FontWeight::Bold);
    title->style().fgColor = currentTheme().accent;

    // Theme toggle button
    bool darkTheme = false;
    auto* themeBtn = new Button("Dark Theme");
    themeBtn->onClick([&]() {
        darkTheme = !darkTheme;
        setTheme(darkTheme ? Theme::Dark() : Theme::Light());
        themeBtn->setText(darkTheme ? "Light Theme" : "Dark Theme");
    });

    header->addChild(title);
    header->addChild(new Widget()); // spacer
    header->addChild(themeBtn);
    header->setLayout(headerLayout);

    // --- Tab Widget ---
    auto* tabs = new TabWidget();

    // ===== Tab 1: Widgets =====
    int tab1 = tabs->addTab("Widgets");
    auto* w1 = tabs->tabContent(tab1);
    auto* w1Layout = new BoxLayout(BoxLayout::TopToBottom, 8, 12);

    // Buttons section
    auto* btnSection = new Widget();
    btnSection->style().bgColor = Color::Transparent;
    auto* btnSectionLayout = new BoxLayout(BoxLayout::TopToBottom, 6, 0);

    auto* btnTitle = new Label("Buttons");
    btnTitle->style().font = Font("Segoe UI", 14, FontWeight::SemiBold);

    auto* btnRow = new Widget();
    btnRow->style().bgColor = Color::Transparent;
    auto* btnRowLayout = new BoxLayout(BoxLayout::LeftToRight, 8, 0);

    auto* normalBtn = new Button("Normal Button");
    auto* primaryBtn = new Button("Primary Action");
    primaryBtn->style().bgColor = currentTheme().accent;
    primaryBtn->style().fgColor = Color::White;

    auto* counterBtn = new Button("Clicked: 0 times");
    counterBtn->onClick([&]() {
        g_clickCount++;
        counterBtn->setText("Clicked: " + std::to_string(g_clickCount) + " times");
    });

    btnRow->addChild(normalBtn);
    btnRow->addChild(primaryBtn);
    btnRow->addChild(counterBtn);
    btnRow->setLayout(btnRowLayout);

    btnSection->addChild(btnTitle);
    btnSection->addChild(btnRow);
    btnSection->setLayout(btnSectionLayout);

    // Input section
    auto* inputSection = new Widget();
    inputSection->style().bgColor = Color::Transparent;
    auto* inputSectionLayout = new BoxLayout(BoxLayout::TopToBottom, 6, 0);

    auto* inputTitle = new Label("Input Controls");
    inputTitle->style().font = Font("Segoe UI", 14, FontWeight::SemiBold);

    auto* inputRow = new Widget();
    inputRow->style().bgColor = Color::Transparent;
    auto* inputRowLayout = new BoxLayout(BoxLayout::LeftToRight, 8, 0);

    auto* nameLabel = new Label("Name:");
    auto* nameBox = new TextBox("Type your name...");

    auto* cb1 = new CheckBox("Accept terms");
    auto* cb2 = new CheckBox("Subscribe");

    inputRow->addChild(nameLabel);
    inputRow->addChild(nameBox);
    inputRow->addChild(cb1);
    inputRow->addChild(cb2);
    inputRow->setLayout(inputRowLayout);
    inputRowLayout->addStretch(0);
    inputRowLayout->addStretch(1);
    inputRowLayout->addStretch(0);
    inputRowLayout->addStretch(0);

    inputSection->addChild(inputTitle);
    inputSection->addChild(inputRow);
    inputSection->setLayout(inputSectionLayout);

    // Radio + select section
    auto* selectSection = new Widget();
    selectSection->style().bgColor = Color::Transparent;
    auto* selectSectionLayout = new BoxLayout(BoxLayout::LeftToRight, 16, 0);

    auto* rbGroup = new Widget();
    rbGroup->style().bgColor = Color::Transparent;
    auto* rbLayout = new BoxLayout(BoxLayout::LeftToRight, 8, 0);
    auto* rb1 = new RadioButton("Red");
    auto* rb2 = new RadioButton("Green");
    auto* rb3 = new RadioButton("Blue");
    rb1->setChecked(true);
    rbGroup->addChild(rb1);
    rbGroup->addChild(rb2);
    rbGroup->addChild(rb3);
    rbGroup->setLayout(rbLayout);

    auto* combo = new ComboBox();
    combo->addItem("Small");
    combo->addItem("Medium");
    combo->addItem("Large");
    combo->addItem("X-Large");
    combo->setCurrentIndex(1);

    auto* comboLabel = new Label("Size: Medium");
    combo->onSelectionChanged([&](int index) {
        comboLabel->setText("Size: " + combo->currentText());
    });

    selectSection->addChild(rbGroup);
    selectSection->addChild(combo);
    selectSection->addChild(comboLabel);
    selectSection->setLayout(selectSectionLayout);

    // Slider + Progress
    auto* progSection = new Widget();
    progSection->style().bgColor = Color::Transparent;
    auto* progSectionLayout = new BoxLayout(BoxLayout::LeftToRight, 12, 0);

    auto* slider = new Slider();
    slider->setRange(0, 100);
    slider->setValue(60);
    auto* slValue = new Label("60%");
    auto* progress = new ProgressBar();
    progress->setValue(60);
    slider->onValueChanged([&](int v) {
        slValue->setText(std::to_string(v) + "%");
        progress->setValue(v);
    });

    progSection->addChild(new Label("Progress:"));
    progSection->addChild(slider);
    progSection->addChild(slValue);
    progSection->addChild(progress);
    progSection->setLayout(progSectionLayout);
    progSectionLayout->addStretch(0);
    progSectionLayout->addStretch(1);
    progSectionLayout->addStretch(0);
    progSectionLayout->addStretch(1);

    // Assemble tab 1
    w1->addChild(btnSection);
    w1->addChild(inputSection);
    w1->addChild(selectSection);
    w1->addChild(progSection);
    w1->setLayout(w1Layout);
    w1Layout->addStretch(0);
    w1Layout->addStretch(0);
    w1Layout->addStretch(0);
    w1Layout->addStretch(1);

    // ===== Tab 2: List & Tree =====
    int tab2 = tabs->addTab("List & Tree");
    auto* w2 = tabs->tabContent(tab2);
    auto* w2Layout = new BoxLayout(BoxLayout::LeftToRight, 8, 12);

    // ListBox
    auto* listCol = new Widget();
    listCol->style().bgColor = Color::Transparent;
    auto* listColLayout = new BoxLayout(BoxLayout::TopToBottom, 4, 0);

    auto* listTitle = new Label("ListBox");
    listTitle->style().font = Font("Segoe UI", 14, FontWeight::SemiBold);

    auto* listBox = new ListBox();
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
    listBox->setSelected(2);

    listCol->addChild(listTitle);
    listCol->addChild(listBox);
    listCol->setLayout(listColLayout);
    listColLayout->addStretch(0);
    listColLayout->addStretch(1);

    // TreeView
    auto* treeCol = new Widget();
    treeCol->style().bgColor = Color::Transparent;
    auto* treeColLayout = new BoxLayout(BoxLayout::TopToBottom, 4, 0);

    auto* treeTitle = new Label("TreeView");
    treeTitle->style().font = Font("Segoe UI", 14, FontWeight::SemiBold);

    auto* treeView = new TreeView();
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

    treeCol->addChild(treeTitle);
    treeCol->addChild(treeView);
    treeCol->setLayout(treeColLayout);
    treeColLayout->addStretch(0);
    treeColLayout->addStretch(1);

    w2->addChild(listCol);
    w2->addChild(treeCol);
    w2->setLayout(w2Layout);
    w2Layout->addStretch(1);
    w2Layout->addStretch(1);

    // ===== Tab 3: About =====
    int tab3 = tabs->addTab("About");
    auto* w3 = tabs->tabContent(tab3);
    auto* w3Layout = new BoxLayout(BoxLayout::TopToBottom, 12, 20);

    auto* aboutTitle = new Label("About ltgui");
    aboutTitle->style().font = Font("Segoe UI", 22, FontWeight::Bold);
    aboutTitle->style().fgColor = currentTheme().accent;

    auto* aboutText = new Label("A from-scratch cross-platform retained-mode C++ GUI framework.");
    aboutText->style().font = Font("Segoe UI", 13);

    auto* aboutFeatures = new Label(
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

    w3->addChild(aboutTitle);
    w3->addChild(aboutText);
    w3->addChild(aboutFeatures);
    w3->setLayout(w3Layout);
    w3Layout->addStretch(0);
    w3Layout->addStretch(0);
    w3Layout->addStretch(1);

    // Assemble
    root->addChild(header);
    root->addChild(tabs);

    window.setCentralWidget(root);
    window.show();

    return Application::instance().run();
}
