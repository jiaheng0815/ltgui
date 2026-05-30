#include "ltgui.h"
using namespace ltgui;

int main() {
    Window window;
    window.create(400, 200, "My App");

    auto* root = new Widget();
    auto* layout = new BoxLayout(BoxLayout::TopToBottom, 8, 12);

    auto* label = new Label("Hello from app/main.cpp!");
    label->style().font = Font("Segoe UI", 16, FontWeight::Bold);

    auto* button = new Button("");
    button->onClick([&]() { window.close(); });

    root->addChild(label);
    root->addChild(button);
    root->setLayout(layout);

    window.setCentralWidget(root);
    window.show();
    return Application::instance().run();
}
