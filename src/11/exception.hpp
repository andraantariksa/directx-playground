#ifndef DIRECTX_PLAYGROUND_SRC_EXCEPTION_HPP
#define DIRECTX_PLAYGROUND_SRC_EXCEPTION_HPP

#define DX_THROW_INFO(hrcall) if(HRESULT hr = (hrcall); FAILED(hr)) \
    throw DX::HrException(__LINE__, __FILE__, hr)
#define DX_THROW_INFO_ONLY(call) m_info_manager.Set(); (call); {auto v = m_info_manager.GetMessages(); if(!v.empty()) {throw DX::InfoException( __LINE__,__FILE__,v);}}

#include <stdexcept>
#include <Windows.h>
#include <string>
#include <cstdint>
#include <vector>

namespace DX
{
	class ChiliException : public std::exception
	{
	public:
		ChiliException(int line, const char* file) noexcept;
		const char* what() const noexcept override;
		virtual const char* GetType() const noexcept;
		int GetLine() const noexcept;
		const std::string& GetFile() const noexcept;
		std::string GetOriginString() const noexcept;
	private:
		int line;
		std::string file;
	protected:
		mutable std::string whatBuffer;
	};

	class Exception : public ChiliException
	{
		using ChiliException::ChiliException;
	};
	class HrException : public Exception
	{
	public:
		HrException(int line, const char* file, HRESULT hr, std::vector<std::string> infoMsgs = {}) noexcept;
		const char* what() const noexcept override;
		const char* GetType() const noexcept override;
		HRESULT GetErrorCode() const noexcept;
		std::string GetErrorString() const noexcept;
		std::string GetErrorDescription() const noexcept;
		std::string GetErrorInfo() const noexcept;
	private:
		HRESULT hr;
		std::string info;
	};
	class InfoException : public Exception
	{
	public:
		InfoException(int line, const char* file, std::vector<std::string> infoMsgs) noexcept;
		const char* what() const noexcept override;
		const char* GetType() const noexcept override;
		std::string GetErrorInfo() const noexcept;
	private:
		std::string info;
	};
	class DeviceRemovedException : public HrException
	{
		using HrException::HrException;
	public:
		const char* GetType() const noexcept override;
	private:
		std::string reason;
	};

}

#endif //DIRECTX_PLAYGROUND_SRC_EXCEPTION_HPP
