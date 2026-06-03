#include "ltgui.h"
#include <iostream>

using namespace ltgui;

int main() {
    Window window;
    if (!window.create(640, 480, "ltgui Demo")) {
        std::cerr << "Failed to create window." << std::endl;
        return 1;
    }

    auto root = std::make_unique<Widget>();
    root->style().bgColor = Color::WindowBg;

    auto mainLayout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 4, 8);

    // Title
    auto* title = root->makeChild<Label>("ltgui Widget Demo");
    title->style().font = Font("Segoe UI", 20, FontWeight::Bold);
    title->style().fgColor = Color::DarkBlue;

    // Button row
    auto* buttonRow = root->makeChild<Widget>();
    buttonRow->style().bgColor = Color::Transparent;
    auto btnLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 4);

    auto* btn1 = buttonRow->makeChild<Button>("Normal");
    auto* btn2 = buttonRow->makeChild<Button>("Disabled");
    btn2->setEnabled(false);
    int clicks = 0;
    auto* btn3 = buttonRow->makeChild<Button>("Counter: 0");
    btn3->onClick([&]() {
        clicks++;
        btn3->setText("Counter: " + std::to_string(clicks));
    });

    buttonRow->setLayout(std::move(btnLayout));

    // Textbox row
    auto* textRow = root->makeChild<Widget>();
    textRow->style().bgColor = Color::Transparent;
    auto textLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 4);
    auto* tbLabel = textRow->makeChild<Label>("Text:");
    auto* textBox = textRow->makeChild<TextBox>("Edit me!");
    textLayout->addStretch(0);
    textLayout->addStretch(1);
    textRow->setLayout(std::move(textLayout));

    // Checkbox + Radio row
    auto* checkRow = root->makeChild<Widget>();
    checkRow->style().bgColor = Color::Transparent;
    auto checkLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 12, 4);

    auto* cb1 = checkRow->makeChild<CheckBox>("Option A");
    auto* cb2 = checkRow->makeChild<CheckBox>("Option B");
    cb2->setChecked(true);
    auto* cb3 = checkRow->makeChild<CheckBox>("Option C");

    auto* rbGroup = checkRow->makeChild<Widget>();
    rbGroup->style().bgColor = Color::Transparent;
    auto rbLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 4, 0);
    auto* rb1 = rbGroup->makeChild<RadioButton>("Red");
    auto* rb2 = rbGroup->makeChild<RadioButton>("Green");
    auto* rb3 = rbGroup->makeChild<RadioButton>("Blue");
    rb1->setChecked(true);
    rbGroup->setLayout(std::move(rbLayout));

    checkRow->setLayout(std::move(checkLayout));

    // Slider row
    auto* sliderRow = root->makeChild<Widget>();
    sliderRow->style().bgColor = Color::Transparent;
    auto sliderLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 4);
    auto* slLabel = sliderRow->makeChild<Label>("Volume:");
    auto* slider = sliderRow->makeChild<Slider>();
    slider->setRange(0, 100);
    slider->setValue(50);
    auto* slValue = sliderRow->makeChild<Label>("50");
    slider->onValueChanged([&](int v) {
        slValue->setText(std::to_string(v));
    });
    sliderLayout->addStretch(0);
    sliderLayout->addStretch(1);
    sliderLayout->addStretch(0);
    sliderRow->setLayout(std::move(sliderLayout));

    // List + Combo row
    auto* listRow = root->makeChild<Widget>();
    listRow->style().bgColor = Color::Transparent;
    auto listRowLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 4);

    auto* listBox = listRow->makeChild<ListBox>();
    listBox->addItem("Apple");
    listBox->addItem("Banana");
    listBox->addItem("Cherry");
    listBox->addItem("Date");
    listBox->addItem("Elderberry");
    listBox->addItem("Fig");
    listBox->addItem("Grape");
    listBox->setSelected(0);

    auto* comboBox = listRow->makeChild<ComboBox>();
    comboBox->addItem("Small");
    comboBox->addItem("Medium");
    comboBox->addItem("Large");
    comboBox->setCurrentIndex(1);

    listRowLayout->addStretch(1);
    listRowLayout->addStretch(0);
    listRow->setLayout(std::move(listRowLayout));

    // Stretch factors
    mainLayout->addStretch(0);
    mainLayout->addStretch(0);
    mainLayout->addStretch(0);
    mainLayout->addStretch(0);
    mainLayout->addStretch(0);
    mainLayout->addStretch(1);

    root->setLayout(std::move(mainLayout));
    window.setCentralWidget(std::move(root));
    window.show();

    return Application::instance().run();
}
