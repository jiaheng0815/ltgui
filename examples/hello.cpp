#include "ltgui.h"
#include <iostream>

using namespace ltgui;

int main() {
    Window window;
    if (!window.create(400, 300, "Hello, ltgui!")) {
        std::cerr << "Failed to create window." << std::endl;
        return 1;
    }

    auto root = std::make_unique<Widget>();
    root->setStyle(Style::defaultStyle());
    root->style().bgColor = Color::WindowBg;

    auto layout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 8, 12);

    auto* label = root->makeChild<Label>("Welcome to ltgui!");
    label->style().font = Font("Segoe UI", 18, FontWeight::Bold);
    label->style().fgColor = Color::DarkBlue;

    auto* button = root->makeChild<Button>("Click Me!");
    int clickCount = 0;
    button->onClick([&]() {
        clickCount++;
        button->setText("Clicked: " + std::to_string(clickCount) + " times");
    });

    auto* quitBtn = root->makeChild<Button>("Quit");
    quitBtn->onClick([&]() {
        window.close();
    });

    layout->addStretch(0);
    layout->addStretch(0);
    layout->addStretch(0);

    root->setLayout(std::move(layout));

    window.setCentralWidget(std::move(root));
    window.show();

    return Application::instance().run();
}
