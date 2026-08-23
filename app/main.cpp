#include "ltgui.h"
#include <iostream>

using namespace ltgui;

int main()
{
	Window window;
	if (!window.create(400, 200, "Hello World"))
	{
		std::cerr << "Failed to create window." << std::endl;
		return 1;
	}

	auto root = std::make_unique<Widget>();
	auto label = root->makeChild<Label>("Hello, World!");
	label->style().font = Font("Segoe UI", 24, FontWeight::Bold);
	label->style().fgColor = currentTheme().accent;

	// 使用 BoxLayout 居中标签:label 前后各放一个 stretch
	auto layout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 0, 0);
	layout->addStretch(1);
	layout->addStretch(1);
	root->setLayout(std::move(layout));

	window.setCentralWidget(std::move(root));
	window.show();

	return Application::instance().run();
}