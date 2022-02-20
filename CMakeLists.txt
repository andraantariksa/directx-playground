cmake_minimum_required(VERSION 3.20)
project(directx_playground)

set(CMAKE_CXX_STANDARD 17)

include(cmake/HLSL.cmake)
add_subdirectory(external)

add_hlsl(
	OUTPUT src/shaders/main.vert.spv
	SOURCE src/shaders/main.vert.hlsl
)

set(DIRECTX11_PLAYGROUND_SOURCES

	src/11/main.cpp
	src/11/renderer.cpp
	src/11/renderer.hpp
	src/11/err/dxerr.cpp
	src/11/exception.hpp
	src/11/exception.cpp
	src/11/dxgi_info_manager.cpp
	src/11/dxgi_info_manager.hpp
	)

xwin_add_executable(directx11_playground
	"${DIRECTX11_PLAYGROUND_SOURCES}"
	)

target_link_libraries(directx11_playground CrossWindow d3d11 dxguid D3DCompiler)

set(DIRECTX12_PLAYGROUND_SOURCES
	
	src/12/main.cpp
	src/12/Renderer.hpp
	src/12/Renderer.cpp
	)

xwin_add_executable(directx12_playground
	"${DIRECTX12_PLAYGROUND_SOURCES}"
	)

target_link_libraries(directx12_playground
	CrossWindow
	d3d12
	dxguid
	dxgi
	D3DCompiler
	Microsoft::DirectX-Headers
	)
