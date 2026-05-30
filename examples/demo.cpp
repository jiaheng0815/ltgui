#include "ltgui.h"
#include <iostream>

using namespace ltgui;

int main() {
    Window window;
    if (!window.create(640, 480, "ltgui Demo")) {
        std::cerr << "Failed to create window." << std::endl;
        return 1;
    }

    auto* root = new Widget();
    root->style().bgColor = Color::WindowBg;

    auto* mainLayout = new BoxLayout(BoxLayout::TopToBottom, 4, 8);

    // Title
    auto* title = new Label("ltgui Widget Demo");
    title->style().font = Font("Segoe UI", 20, FontWeight::Bold);
    title->style().fgColor = Color::DarkBlue;

    // Button row
    auto* buttonRow = new Widget();
    buttonRow->style().bgColor = Color::Transparent;
    auto* btnLayout = new BoxLayout(BoxLayout::LeftToRight, 8, 4);

    auto* btn1 = new Button("Normal");
    auto* btn2 = new Button("Disabled");
    btn2->setEnabled(false);
    int clicks = 0;
    auto* btn3 = new Button("Counter: 0");
    btn3->onClick([&]() {
        clicks++;
        btn3->setText("Counter: " + std::to_string(clicks));
    });

    buttonRow->addChild(btn1);
    buttonRow->addChild(btn2);
    buttonRow->addChild(btn3);
    buttonRow->setLayout(btnLayout);

    // Textbox row
    auto* textRow = new Widget();
    textRow->style().bgColor = Color::Transparent;
    auto* textLayout = new BoxLayout(BoxLayout::LeftToRight, 8, 4);
    auto* tbLabel = new Label("Text:");
    auto* textBox = new TextBox("Edit me!");
    textRow->addChild(tbLabel);
    textRow->addChild(textBox);
    textRow->setLayout(textLayout);
    textLayout->addStretch(0);
    textLayout->addStretch(1);

    // Checkbox + Radio row
    auto* checkRow = new Widget();
    checkRow->style().bgColor = Color::Transparent;
    auto* checkLayout = new BoxLayout(BoxLayout::LeftToRight, 12, 4);

    auto* cb1 = new CheckBox("Option A");
    auto* cb2 = new CheckBox("Option B");
    cb2->setChecked(true);
    auto* cb3 = new CheckBox("Option C");

    auto* rbGroup = new Widget();
    rbGroup->style().bgColor = Color::Transparent;
    auto* rbLayout = new BoxLayout(BoxLayout::LeftToRight, 4, 0);
    auto* rb1 = new RadioButton("Red");
    auto* rb2 = new RadioButton("Green");
    auto* rb3 = new RadioButton("Blue");
    rb1->setChecked(true);
    rbGroup->addChild(rb1);
    rbGroup->addChild(rb2);
    rbGroup->addChild(rb3);
    rbGroup->setLayout(rbLayout);

    checkRow->addChild(cb1);
    checkRow->addChild(cb2);
    checkRow->addChild(cb3);
    checkRow->addChild(rbGroup);
    checkRow->setLayout(checkLayout);

    // Slider row
    auto* sliderRow = new Widget();
    sliderRow->style().bgColor = Color::Transparent;
    auto* sliderLayout = new BoxLayout(BoxLayout::LeftToRight, 8, 4);
    auto* slLabel = new Label("Volume:");
    auto* slider = new Slider();
    slider->setRange(0, 100);
    slider->setValue(50);
    auto* slValue = new Label("50");
    slider->onValueChanged([&](int v) {
        slValue->setText(std::to_string(v));
    });
    sliderRow->addChild(slLabel);
    sliderRow->addChild(slider);
    sliderRow->addChild(slValue);
    sliderRow->setLayout(sliderLayout);
    sliderLayout->addStretch(0);
    sliderLayout->addStretch(1);
    sliderLayout->addStretch(0);

    // List + Combo row
    auto* listRow = new Widget();
    listRow->style().bgColor = Color::Transparent;
    auto* listRowLayout = new BoxLayout(BoxLayout::LeftToRight, 8, 4);

    auto* listBox = new ListBox();
    listBox->addItem("Apple");
    listBox->addItem("Banana");
    listBox->addItem("Cherry");
    listBox->addItem("Date");
    listBox->addItem("Elderberry");
    listBox->addItem("Fig");
    listBox->addItem("Grape");
    listBox->setSelected(0);

    auto* comboBox = new ComboBox();
    comboBox->addItem("Small");
    comboBox->addItem("Medium");
    comboBox->addItem("Large");
    comboBox->setCurrentIndex(1);

    listRow->addChild(listBox);
    listRow->addChild(comboBox);
    listRow->setLayout(listRowLayout);
    listRowLayout->addStretch(1);
    listRowLayout->addStretch(0);

    // Assemble
    root->addChild(title);
    root->addChild(buttonRow);
    root->addChild(textRow);
    root->addChild(checkRow);
    root->addChild(sliderRow);
    root->addChild(listRow);

    // Stretch factors: title(0), button(0), text(0), checks(0), slider(0), list(1)
    mainLayout->addStretch(0);
    mainLayout->addStretch(0);
    mainLayout->addStretch(0);
    mainLayout->addStretch(0);
    mainLayout->addStretch(0);
    mainLayout->addStretch(1);

    root->setLayout(mainLayout);
    window.setCentralWidget(root);
    window.show();

    return Application::instance().run();
}
