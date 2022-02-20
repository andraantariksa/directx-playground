#ifndef DIRECTX_PLAYGROUND_SRC_RENDERER_HPP
#define DIRECTX_PLAYGROUND_SRC_RENDERER_HPP

#include <d3d11.h>
#include <dxgi.h>
#include <wrl.h>

#include "dxgi_info_manager.hpp"

namespace DX11
{

	class Renderer
	{
	public:
		explicit Renderer(HWND h_wnd);
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		~Renderer() = default;

		void render();
	private:
		Microsoft::WRL::ComPtr<ID3D11Device> m_device;
		Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapchain;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_device_context;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_render_target_view;
		DxgiInfoManager m_info_manager;
	};

}

#endif //DIRECTX_PLAYGROUND_SRC_RENDERER_HPP
