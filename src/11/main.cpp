#include <CrossWindow/CrossWindow.h>
#include <cassert>

#include "renderer.hpp"

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

	auto h_wnd = window.getHwnd();
	DX11::Renderer renderer(h_wnd);

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
				is_running = false;
				break;
			default:
				break;
			}
			event_queue.pop();
		}

		renderer.render();
	}
}
