#include "ltgui.h"
#include <iostream>

using namespace ltgui;

int main() {
    Window window;
    if (!window.create(400, 300, "Hello, ltgui!")) {
        std::cerr << "Failed to create window." << std::endl;
        return 1;
    }

    // Create a root widget
    auto* root = new Widget();
    root->setStyle(Style::defaultStyle());
    root->style().bgColor = Color::WindowBg;

    // Vertical layout
    auto* layout = new BoxLayout(BoxLayout::TopToBottom, 8, 12);

    auto* label = new Label("Welcome to ltgui!");
    label->style().font = Font("Segoe UI", 18, FontWeight::Bold);
    label->style().fgColor = Color::DarkBlue;

    auto* button = new Button("Click Me!");
    int clickCount = 0;
    button->onClick([&]() {
        clickCount++;
        button->setText("Clicked: " + std::to_string(clickCount) + " times");
    });

    auto* quitBtn = new Button("Quit");
    quitBtn->onClick([&]() {
        window.close();
    });

    layout->addStretch(0); // label doesn't stretch
    layout->addStretch(0); // button doesn't stretch
    layout->addStretch(0); // quit doesn't stretch

    root->addChild(label);
    root->addChild(button);
    root->addChild(quitBtn);
    root->setLayout(layout);

    window.setCentralWidget(root);
    window.show();

    return Application::instance().run();
}
