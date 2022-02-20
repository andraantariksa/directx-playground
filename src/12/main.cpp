#include <CrossWindow/CrossWindow.h>
#include <cassert>
#include <algorithm>

#include "Renderer.hpp"

void xmain(int argc, const char** argv)
{
	xwin::WindowDesc window_desc;
	window_desc.name = "DirectX Playground";
	window_desc.title = "DirectX Playground";
	window_desc.width = 1280;
	window_desc.height = 720;

	xwin::Window window;
	xwin::EventQueue event_queue;

	assert(window.create(window_desc, event_queue));

	DX12::Renderer renderer(window);

	bool is_running = true;
	while (is_running)
	{
		event_queue.update();

		while (!event_queue.empty())
		{
			const auto& event = event_queue.front();
			switch (event.type)
			{
			case xwin::EventType::Close:
			{
				is_running = false;
				break;
			}
			// case xwin::EventType::Resize:
			// {
			// 	auto size = window.getCurrentDisplaySize();
			// 	size.x = std::max(1U, size.x);
			// 	size.y = std::max(1U, size.y);
			// 	renderer.resize(size);
			// 	break;
			// }
			default:
				break;
			}
			event_queue.pop();
		}

		renderer.render();
	}
}
