#include "Renderer.hpp"

#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <dxgi1_6.h>
#include <Windows.h>

#include <cassert>
#include <cstdint>

#include <CrossWindow/CrossWindow.h>

#define ASSERT(hr) assert(!FAILED(hr));

namespace DX12
{

	using Microsoft::WRL::ComPtr;

	Renderer::Renderer(xwin::Window& window)
	{
		enable_debug_layer();

		m_adapter = create_adapter();
		m_device = create_device(m_adapter);
		m_command_queue = create_command_queue(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		{
			m_swap_chain = create_swap_chain(
				m_command_queue,
				window,
				m_back_buffers.size());
			m_current_back_buffer_idx = m_swap_chain->GetCurrentBackBufferIndex();
		}
		m_rtv_descriptor_heap = create_descriptor_heap(
			m_device,
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			m_back_buffers.size());

		update_render_target_view(m_swap_chain, m_device, m_rtv_descriptor_heap);

		for (
			uint8_t back_buffer_idx = 0;
			back_buffer_idx < m_back_buffers.size();
			++back_buffer_idx)
		{
			m_command_allocators[back_buffer_idx] = create_command_allocator(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		}
		m_command_list = create_command_list(m_device, m_command_allocators[m_current_back_buffer_idx], D3D12_COMMAND_LIST_TYPE_DIRECT);

		m_fence = create_fence(m_device);
		m_fence_event = create_event_handle();

		// resize(window.getCurrentDisplaySize());
	}

	void Renderer::render()
	{
		auto command_allocator = m_command_allocators[m_current_back_buffer_idx];
		auto back_buffer = m_back_buffers[m_current_back_buffer_idx];

		command_allocator->Reset();
		m_command_list->Reset(command_allocator.Get(), nullptr);

		{
			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				back_buffer.Get(),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_command_list->ResourceBarrier(1, &barrier);

			FLOAT clear_color[] = { 0.4f, 0.6f, 0.9f, 1.0f };

			auto rtv_descriptor_size = m_device->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
				m_rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart(),
				m_current_back_buffer_idx,
				rtv_descriptor_size);

			m_command_list->ClearRenderTargetView(rtv, clear_color, 0, nullptr);
		}

		{
			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				back_buffer.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT);
			m_command_list->ResourceBarrier(1, &barrier);

			ASSERT(m_command_list->Close());

			ID3D12CommandList* const command_list[] = { m_command_list.Get() };
			m_command_queue->ExecuteCommandLists(_countof(command_list), command_list);

			m_frame_fence_values[m_current_back_buffer_idx] = signal(m_command_queue, m_fence, m_fence_value);

			ASSERT(m_swap_chain->Present(1, 0));

			m_current_back_buffer_idx = m_swap_chain->GetCurrentBackBufferIndex();

			block_until_fence_value(m_fence, m_frame_fence_values[m_current_back_buffer_idx], m_fence_event);
		}
	}

	void Renderer::resize(xwin::UVec2 size)
	{
		flush(m_command_queue, m_fence, m_fence_value, m_fence_event);

		for (
			uint8_t back_buffer_idx = 0;
			back_buffer_idx < m_back_buffers.size();
			++back_buffer_idx)
		{
			m_back_buffers[back_buffer_idx].Reset();
			m_frame_fence_values[back_buffer_idx] = m_frame_fence_values[m_current_back_buffer_idx];
		}

		DXGI_SWAP_CHAIN_DESC desc;
		ASSERT(m_swap_chain->GetDesc(&desc));
		ASSERT(m_swap_chain->ResizeBuffers(
			m_back_buffers.size(),
			size.x,
			size.y,
			desc.BufferDesc.Format,
			desc.Flags));

		m_current_back_buffer_idx = m_swap_chain->GetCurrentBackBufferIndex();

		update_render_target_view(m_swap_chain, m_device, m_rtv_descriptor_heap);
	}

	void Renderer::enable_debug_layer() const
	{
#if defined(_DEBUG)
		ComPtr<ID3D12Debug3> debug_interface;
		ASSERT(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)));
		debug_interface->EnableDebugLayer();
#endif
	}

	ComPtr<IDXGIAdapter4> Renderer::create_adapter() const
	{
		auto dxgi_factory = create_dxgi_factory();

		ComPtr<IDXGIAdapter4> dxgi_adapter4;
		ComPtr<IDXGIAdapter1> dxgi_adapter1;

		if (m_is_using_warp)
		{
			ASSERT(dxgi_factory->EnumWarpAdapter(IID_PPV_ARGS(&dxgi_adapter4)));
		}
		else
		{
			SIZE_T max_dedicated_video_memory = 0;
			for (
				UINT device_idx = 0;
				dxgi_factory->EnumAdapters1(
					device_idx,
					&dxgi_adapter1) != DXGI_ERROR_NOT_FOUND;
				++device_idx)
			{
				DXGI_ADAPTER_DESC1 adapter_desc;
				dxgi_adapter1->GetDesc1(&adapter_desc);

				if (
					adapter_desc.DedicatedVideoMemory > max_dedicated_video_memory &&
					// Check if its support  thefeature level and the interface
					SUCCEEDED(
						D3D12CreateDevice(
							dxgi_adapter4.Get(),
							D3D_FEATURE_LEVEL_12_1,
							__uuidof(ID3D12Device8),
							nullptr)) &&
					// Not a software adapter
					(adapter_desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) == 0
					)
				{
					max_dedicated_video_memory = adapter_desc.DedicatedVideoMemory;
					ASSERT(dxgi_adapter1.As(&dxgi_adapter4));
				}
			}
		}

		return dxgi_adapter4;
	}

	ComPtr<ID3D12Device8> Renderer::create_device(ComPtr<IDXGIAdapter4> adapter) const
	{
		ComPtr<ID3D12Device8> device;
		ASSERT(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device)));

#if defined(_DEBUG)
		{
			ComPtr<ID3D12InfoQueue> info_queue;
			ASSERT(device.As(&info_queue));
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

			// JUST COPY PASTE

			// Suppress whole categories of messages
			//D3D12_MESSAGE_CATEGORY Categories[] = {};

			// Suppress messages based on their severity level
			D3D12_MESSAGE_SEVERITY Severities[] =
			{
				D3D12_MESSAGE_SEVERITY_INFO
			};

			// Suppress individual messages by their ID
			D3D12_MESSAGE_ID DenyIds[] = {
				D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
				D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
				D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
			};

			D3D12_INFO_QUEUE_FILTER NewFilter = {};
			//NewFilter.DenyList.NumCategories = _countof(Categories);
			//NewFilter.DenyList.pCategoryList = Categories;
			NewFilter.DenyList.NumSeverities = _countof(Severities);
			NewFilter.DenyList.pSeverityList = Severities;
			NewFilter.DenyList.NumIDs = _countof(DenyIds);
			NewFilter.DenyList.pIDList = DenyIds;

			ASSERT(info_queue->PushStorageFilter(&NewFilter));
		}
#endif

		return device;
	}

	ComPtr<ID3D12CommandQueue> Renderer::create_command_queue(
		ComPtr<ID3D12Device8> device,
		D3D12_COMMAND_LIST_TYPE type) const
	{
		D3D12_COMMAND_QUEUE_DESC desc;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;
		// Why the type is "command list type" instead of "command queue type"???
		desc.Type = type;

		ComPtr<ID3D12CommandQueue> command_queue;
		ASSERT(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&command_queue)));

		return command_queue;
	}

	ComPtr<IDXGISwapChain4> Renderer::create_swap_chain(
		ComPtr<ID3D12CommandQueue> command_queue,
		xwin::Window& window,
		UINT buffer_count) const
	{
		auto window_size = window.getCurrentDisplaySize();

		auto dxgi_factory = create_dxgi_factory();
		ComPtr<IDXGISwapChain1> swap_chain1;
		ComPtr<IDXGISwapChain4> swap_chain4;

		DXGI_SWAP_CHAIN_DESC1 desc;
		desc.Width = window_size.x;
		desc.Height = window_size.y;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.Stereo = false;
		desc.SampleDesc = { 1, 0 };
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.Flags = 0;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.Scaling = DXGI_SCALING_STRETCH;
		desc.BufferCount = buffer_count;
		desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

		ASSERT(dxgi_factory->CreateSwapChainForHwnd(
			command_queue.Get(),
			window.getHwnd(),
			&desc,
			nullptr,
			nullptr,
			&swap_chain1));
		ASSERT(swap_chain1.As(&swap_chain4));

		return swap_chain4;
	}

	ComPtr<IDXGIFactory7> Renderer::create_dxgi_factory() const
	{
		ComPtr<IDXGIFactory7> dxgi_factory;
		auto dxgi_factory_flag = 0;
#if defined(_DEBUG)
		dxgi_factory_flag |= DXGI_CREATE_FACTORY_DEBUG;
#endif

		ASSERT(CreateDXGIFactory2(dxgi_factory_flag, IID_PPV_ARGS(&dxgi_factory)));

		return dxgi_factory;
	}

	ComPtr<ID3D12CommandAllocator> Renderer::create_command_allocator(
		ComPtr<ID3D12Device8> device,
		D3D12_COMMAND_LIST_TYPE type) const
	{
		ComPtr<ID3D12CommandAllocator> command_allocator;

		ASSERT(device->CreateCommandAllocator(type, IID_PPV_ARGS(&command_allocator)));

		return command_allocator;
	}

	ComPtr<ID3D12DescriptorHeap> Renderer::create_descriptor_heap(
		ComPtr<ID3D12Device8> device,
		D3D12_DESCRIPTOR_HEAP_TYPE type,
		UINT total_descriptors) const
	{
		ComPtr<ID3D12DescriptorHeap> descriptor_heap;

		D3D12_DESCRIPTOR_HEAP_DESC desc;
		desc.NumDescriptors = total_descriptors;
		desc.Type = type;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 0;

		ASSERT(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptor_heap)));

		return descriptor_heap;
	}

	ComPtr<ID3D12Fence1> Renderer::create_fence(ComPtr<ID3D12Device8> device) const
	{
		ComPtr<ID3D12Fence1> fence;
		ASSERT(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
		return fence;
	}

	ComPtr<ID3D12GraphicsCommandList> Renderer::create_command_list(
		ComPtr<ID3D12Device8> device,
		ComPtr<ID3D12CommandAllocator> command_allocator,
		D3D12_COMMAND_LIST_TYPE type) const
	{
		ComPtr<ID3D12GraphicsCommandList> command_list;
		ASSERT(device->CreateCommandList(
			0,
			type,
			command_allocator.Get(),
			nullptr,
			IID_PPV_ARGS(&command_list)));
		ASSERT(command_list->Close());
		return command_list;
	}

	uint64_t Renderer::signal(
		ComPtr<ID3D12CommandQueue> command_queue,
		ComPtr<ID3D12Fence> fence,
		uint64_t& fence_value)
	{
		uint64_t fence_value_for_signal = ++fence_value;
		ASSERT(command_queue->Signal(fence.Get(), fence_value_for_signal));
		return fence_value_for_signal;
	}

	void Renderer::flush(
		ComPtr<ID3D12CommandQueue> command_queue,
		ComPtr<ID3D12Fence> fence,
		uint64_t& fence_value,
		HANDLE fence_event)
	{
		uint64_t fence_value_for_signal = signal(command_queue, fence, fence_value);
		block_until_fence_value(fence, fence_value_for_signal, fence_event);
	}

	void Renderer::block_until_fence_value(
		ComPtr<ID3D12Fence> fence,
		uint64_t fence_value,
		HANDLE fence_event,
		std::chrono::milliseconds duration)
	{
		if (fence->GetCompletedValue() < fence_value)
		{
			ASSERT(fence->SetEventOnCompletion(fence_value, fence_event));
			WaitForSingleObject(fence_event, duration.count());
		}
	}

	void Renderer::update_render_target_view(
		ComPtr<IDXGISwapChain4> swap_chain,
		ComPtr<ID3D12Device8> device,
		ComPtr<ID3D12DescriptorHeap> descriptor_heap)
	{
		auto rtv_descriptor_size = device->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		auto rtv_cpu_descriptor_handle = descriptor_heap->GetCPUDescriptorHandleForHeapStart();
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(rtv_cpu_descriptor_handle);

		for (
			uint8_t back_buffer_idx = 0;
			back_buffer_idx < m_back_buffers.size();
			++back_buffer_idx)
		{
			ComPtr<ID3D12Resource2> back_buffer;
			ASSERT(swap_chain->GetBuffer(back_buffer_idx, IID_PPV_ARGS(&back_buffer)));

			device->CreateRenderTargetView(back_buffer.Get(), nullptr, rtv_handle);

			m_back_buffers[back_buffer_idx] = back_buffer;

			rtv_handle.Offset(rtv_descriptor_size);
		}
	}

	HANDLE Renderer::create_event_handle()
	{
		const HANDLE fence_event_handle = CreateEvent(nullptr, false, false, nullptr);
		assert(fence_event_handle && "Failed to create fence event handle");
		return fence_event_handle;
	}
}
