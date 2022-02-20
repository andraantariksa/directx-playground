#include "renderer.hpp"

#include "exception.hpp"

#include <d3d11.h>
#include <dxgi.h>
#include <cassert>
#include <comdef.h>
#include <d3dcompiler.h>

namespace DX11
{

	using Microsoft::WRL::ComPtr;

	Renderer::Renderer(HWND h_wnd)
	{
		DXGI_SWAP_CHAIN_DESC swapchain_descriptor;
		swapchain_descriptor.BufferCount = 1;
		swapchain_descriptor.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapchain_descriptor.BufferDesc.Height = 720;
		swapchain_descriptor.BufferDesc.Width = 1280;
		swapchain_descriptor.BufferDesc.RefreshRate.Denominator = 0;
		swapchain_descriptor.BufferDesc.RefreshRate.Numerator = 0;
		swapchain_descriptor.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapchain_descriptor.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapchain_descriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchain_descriptor.Flags = 0;
		swapchain_descriptor.OutputWindow = h_wnd;
		swapchain_descriptor.SampleDesc.Count = 1;
		swapchain_descriptor.SampleDesc.Quality = 0;
		swapchain_descriptor.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapchain_descriptor.Windowed = true;

		D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			D3D11_CREATE_DEVICE_DEBUG,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&swapchain_descriptor,
			&m_swapchain,
			&m_device,
			nullptr,
			&m_device_context);

		ComPtr<ID3D11Resource> back_buffer;

		DX_THROW_INFO(m_swapchain->GetBuffer(0, __uuidof(ID3D11Resource), &back_buffer));
		DX_THROW_INFO(m_device->CreateRenderTargetView(back_buffer.Get(), nullptr, &m_render_target_view));
	}

	struct Vertex
	{
		float color[4];
		float pos[2];
	};

	void Renderer::render()
	{
		const float color[] = { 1.0f, 0.0f, 0.0f, 1.0f };
		m_device_context->ClearRenderTargetView(m_render_target_view.Get(), color);

		ComPtr<ID3D11Buffer> vertex_buffer;

		Vertex vertices[3] = {
			{
				{ 1.0f, 0.0f, 0.0f, 1.0f },
				{ 0.0f, 0.5f }
			},
			{
				{ 0.0f, 1.0f, 0.0f, 1.0f },
				{ 1.0f, -0.5f }
			},
			{
				{ 0.0f, 0.0f, 1.0f, 1.0f },
				{ -0.5f, -0.5f }
			},
		};

		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.ByteWidth = sizeof(vertices);
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = sizeof(Vertex);
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;

		D3D11_SUBRESOURCE_DATA subresource_data;
		subresource_data.SysMemPitch = 0;
		subresource_data.SysMemSlicePitch = 0;
		subresource_data.pSysMem = vertices;

		DX_THROW_INFO(m_device->CreateBuffer(&buffer_desc, &subresource_data, &vertex_buffer));

		const uint32_t strides = sizeof(Vertex);
		const uint32_t offset = 0;
		m_device_context->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &strides, &offset);
		m_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		ComPtr<ID3D11VertexShader> vertex_shader;
		ComPtr<ID3D11PixelShader> pixel_shader;
		ComPtr<ID3DBlob> blob;

		DX_THROW_INFO(D3DReadFileToBlob(L"C:\\Users\\andra\\Projects\\directx-playground\\src\\shaders\\main.pix.cso", &blob));
		DX_THROW_INFO(m_device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &pixel_shader));

		m_device_context->PSSetShader(pixel_shader.Get(), nullptr, 0);

		DX_THROW_INFO(D3DReadFileToBlob(L"C:\\Users\\andra\\Projects\\directx-playground\\src\\shaders\\main.vert.cso", &blob));
		DX_THROW_INFO(m_device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &vertex_shader));

		m_device_context->VSSetShader(vertex_shader.Get(), nullptr, 0);

		ComPtr<ID3D11InputLayout> input_layout;
		const D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
		{
			{
				"Color",
				0,
				DXGI_FORMAT_R32G32B32A32_FLOAT,
				0,
				0,
				D3D11_INPUT_PER_VERTEX_DATA,
				0
			},
			{
				"Position",
				0,
				DXGI_FORMAT_R32G32_FLOAT,
				0,
				16,
				D3D11_INPUT_PER_VERTEX_DATA,
				0
			}
		};

		DX_THROW_INFO(m_device->CreateInputLayout(input_element_desc, 2, blob->GetBufferPointer(), blob->GetBufferSize(), &input_layout));

		m_device_context->IASetInputLayout(input_layout.Get());

		m_device_context->OMSetRenderTargets(1, m_render_target_view.GetAddressOf(), nullptr);

		D3D11_VIEWPORT viewport;
		viewport.Height = 720.0f;
		viewport.Width = 1280.0f;
		viewport.MaxDepth = 1.0f;
		viewport.MinDepth = 0.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		m_device_context->RSSetViewports(1, &viewport);

		DX_THROW_INFO_ONLY(m_device_context->Draw(3, 0));

		if (HRESULT result = m_swapchain->Present(1, 0); FAILED(result))
		{
			if (result == DXGI_ERROR_DEVICE_REMOVED)
			{
				throw DX::DeviceRemovedException(__LINE__, __FILE__, result);
			}
			else
			{
				DX_THROW_INFO(result);
			}
		}
	}

}
