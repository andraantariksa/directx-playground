#ifndef _RENDERER_HPP
#define _RENDERER_HPP

#include <directx/d3d12.h>
#include <wrl.h>
#include <Windows.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <array>


#ifdef max
#undef max
#endif

#include <cassert>
#include <chrono>

#include <CrossWindow/CrossWindow.h>

namespace DX12
{
	class Renderer
	{
	public:
		Renderer(xwin::Window& window);

		void enable_debug_layer() const;

		Microsoft::WRL::ComPtr<IDXGIAdapter4> create_adapter() const;
		Microsoft::WRL::ComPtr<ID3D12Device8> create_device(
			Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter) const;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> create_command_queue(
			Microsoft::WRL::ComPtr<ID3D12Device8> device,
			D3D12_COMMAND_LIST_TYPE type) const;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> create_swap_chain(
			Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue,
			xwin::Window& window,
			UINT buffer_count) const;
		Microsoft::WRL::ComPtr<IDXGIFactory7> create_dxgi_factory() const;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> create_command_allocator(
			Microsoft::WRL::ComPtr<ID3D12Device8> device,
			D3D12_COMMAND_LIST_TYPE type) const;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> create_descriptor_heap(
			Microsoft::WRL::ComPtr<ID3D12Device8> device,
			D3D12_DESCRIPTOR_HEAP_TYPE type,
			UINT total_descriptors) const;
		Microsoft::WRL::ComPtr<ID3D12Fence1> create_fence(
			Microsoft::WRL::ComPtr<ID3D12Device8> device) const;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> create_command_list(
			Microsoft::WRL::ComPtr<ID3D12Device8> device,
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator,
			D3D12_COMMAND_LIST_TYPE type) const;

		HANDLE create_event_handle();

		uint64_t signal(
			Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue,
			Microsoft::WRL::ComPtr<ID3D12Fence> fence,
			uint64_t& fence_value);
		void flush(
			Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue,
			Microsoft::WRL::ComPtr<ID3D12Fence> fence,
			uint64_t& fence_value,
			HANDLE fence_event);
		void block_until_fence_value(
			Microsoft::WRL::ComPtr<ID3D12Fence> fence,
			uint64_t fence_value,
			HANDLE fence_event,
			std::chrono::milliseconds duration = std::chrono::milliseconds::max());

		void update_render_target_view(
			Microsoft::WRL::ComPtr<IDXGISwapChain4> swap_chain,
			Microsoft::WRL::ComPtr<ID3D12Device8> device,
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap);

		void render();
		void resize(xwin::UVec2 size);
	private:
		Microsoft::WRL::ComPtr<ID3D12Device8> m_device;
		Microsoft::WRL::ComPtr<IDXGIAdapter4> m_adapter;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_command_queue;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swap_chain;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtv_descriptor_heap;

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_command_list;
		std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, 3> m_command_allocators;
		std::array<Microsoft::WRL::ComPtr<ID3D12Resource2>, 3> m_back_buffers;
		std::array<uint64_t, 3> m_frame_fence_values;

		Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
		uint64_t m_fence_value = 0;
		HANDLE m_fence_event;

		uint8_t m_current_back_buffer_idx = 0;

		bool m_is_using_warp = false;
	};
}

#endif
