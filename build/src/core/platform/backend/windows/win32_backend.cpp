// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------

#include "platform_internal.h"

#include <core/logging.h>

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <commdlg.h> // for OPENFILENAME, GetOpenFileName, GetSaveFileName
#include <shobjidl.h> // for the Common Item Dialog functions
#include <Shlwapi.h>

namespace win32
{
	void wide_to_ascii(platform::PathString& filename, WCHAR* string)
	{
		int mb_size_required = WideCharToMultiByte(CP_ACP, 0, string, wcslen(string), NULL, 0, NULL, NULL);

		// If this is too large, then we need to use a dynamic buffer instead.
		assert(filename.max_size() > mb_size_required);

		WideCharToMultiByte(CP_ACP, 0, string, wcslen(string), &filename[0], mb_size_required, NULL, NULL);
	} // wide_to_ascii
} // namespace win32

namespace platform
{
	static unsigned int _previous_error_mode;


	void joystick_startup();
	void joystick_shutdown();
	void joystick_update(float delta_milliseconds);

	Result backend_startup()
	{
		// prevent error dialogs from hanging the process.
		// these errors are forwarded to calling process.
		_previous_error_mode = SetErrorMode(SEM_FAILCRITICALERRORS);

		joystick_startup();

		return Result::success();
	} // backend_startup

	void backend_shutdown()
	{
		joystick_shutdown();

		// restore the error mode
		SetErrorMode(_previous_error_mode);
	} // backend_shutdown


	void backend_update(float delta_milliseconds)
	{
		// dispatch available window events
		window::dispatch_events();

		// update joysticks
		joystick_update(delta_milliseconds);
	} // backend_update

	// system
	size_t system_pagesize_bytes()
	{
		SYSTEM_INFO system_info;
		GetSystemInfo(&system_info);
		return system_info.dwPageSize;
	} // system_pagesize_bytes

	size_t system_processor_count()
	{
		// docs mention use of 'GetLogicalProcessorInformation',
		// but I don't understand the difference.
		SYSTEM_INFO system_info;
		GetSystemInfo(&system_info);
		return system_info.dwNumberOfProcessors;
	} // system_processor_count

	double system_uptime_seconds()
	{
		return GetTickCount64() * SecondsPerMillisecond;
	} // system_uptime_seconds

	core::StackString<64> system_version_string()
	{
		core::StackString<64> version = "Windows";
		return version;
	} // system_version_string

	Process* process_create(
		const char*,
		const Array<PathString>&,
		const char*
		)
	{
		return nullptr;
	} // process_create

	void process_destroy(Process*)
	{
	} // process_destroy

	bool process_is_running(Process*)
	{
		return false;
	} // process_is_running

	// Creates a filter string for use with OPENFILENAME structure.
	// The resulting char* MUST BE DEALLOCATED by the caller.
	char* create_filter_string(const Array<PlatformExtensionDescription>& extensions)
	{
		// determine the size of the buffer we need for the filter string.
		size_t filter_buffer_size = 0;
		for (size_t index = 0; index < extensions.size(); ++index)
		{
			// each entry is '<description> (*.<extension>)\0*.<extension>\0'
			const PlatformExtensionDescription& extensiondesc = extensions[index];

			filter_buffer_size += \
				core::str::len(extensiondesc.description) + \
				2 * core::str::len(extensiondesc.extension) + \
				8; // spaces, parens, terminators, etc.
		}

		// We need a buffer size of fiter_buffer_size bytes to pass to the
		// dialog function. This buffer requires TWO NULL TERMINATORS.
		char* filter_buffer = static_cast<char*>(MEMORY2_ALLOC(get_platform_allocator2(), filter_buffer_size + 2));
		memset(filter_buffer, 0, filter_buffer_size + 2);

		char* working_buffer = filter_buffer;

		// Now populate the filters string.
		for (size_t index = 0; index < extensions.size(); ++index)
		{
			// each entry is '<description> (*.<extension>)\0*.<extension>\0'
			const PlatformExtensionDescription& extensiondesc = extensions[index];

			char* description_line = core::str::format("%s (*.%s)",
				extensiondesc.description,
				extensiondesc.extension);
			core::str::cat(working_buffer, description_line);

			size_t buffer_offset = 0;

			buffer_offset = core::str::len(description_line);
			working_buffer += buffer_offset + 1;

			buffer_offset = 0;
			char* filter_line = core::str::format("*.%s", extensiondesc.extension);
			core::str::cat(working_buffer, filter_line);
			buffer_offset = core::str::len(filter_line);

			working_buffer += buffer_offset + 1;
		}

		// terminate the filter_buffer
		working_buffer[0] = '\0';

		return filter_buffer;
	}

	// These should be converted to use the OFNHookProc.
	// https://msdn.microsoft.com/en-us/library/ms646931(v=vs.85).aspx

	class OpenFileCallback : public IFileDialogEvents
	{
	public:
		open_dialog_event_handler event_handler;

		OpenFileCallback()
			: references(0)
		{
		}

		void set_handler(open_dialog_event_handler handler)
		{
			event_handler = handler;
		}

		IFACEMETHODIMP QueryInterface(REFIID refiid, void** ppv)
		{
			static const QITAB qit[] = {
				QITABENT(OpenFileCallback, IFileDialogEvents),
				{ 0 }
			};

			return QISearch(this, qit, refiid, ppv);
		}


		IFACEMETHODIMP_(ULONG) AddRef()
		{
			return InterlockedIncrement(&references);
		}

		IFACEMETHODIMP_(ULONG) Release()
		{
			long refs = InterlockedDecrement(&references);
			if (refs == 0)
			{
				delete this;
			}
			return refs;
		}

		// IFileDialogEvents
		IFACEMETHODIMP OnFileOk(IFileDialog* file_dialog)
		{
			uint32_t result = 0;
			if (event_handler.is_valid())
			{
				PlatformDialogEvent event;
				event.type = OpenDialogEventType::OkClicked;

				IShellItem* shell_item;
				if (SUCCEEDED(file_dialog->GetResult(&shell_item)))
				{
					WCHAR* buffer;
					if (!SUCCEEDED(shell_item->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, (LPWSTR*)&buffer)))
					{
						return E_INVALIDARG;
					}

					win32::wide_to_ascii(event.filename, buffer);
					CoTaskMemFree(buffer);
				}
				result = event_handler(event);
			}

			// result to HRESULT.
			if (result != 0)
			{
				return E_INVALIDARG;
			}
			else
			{
				return S_OK;
			}
		}
		IFACEMETHODIMP OnFolderChanging(IFileDialog* pfd, IShellItem* psiFolder) { return E_NOTIMPL; }
		IFACEMETHODIMP OnFolderChange(IFileDialog* pfd) { return E_NOTIMPL; }
		IFACEMETHODIMP OnSelectionChange(IFileDialog* pfd) { return S_OK; }
		IFACEMETHODIMP OnShareViolation(IFileDialog* pfd, IShellItem* psi,
			FDE_SHAREVIOLATION_RESPONSE* pResponse) { return E_NOTIMPL; }
		IFACEMETHODIMP OnTypeChange(IFileDialog* pfd) { return E_NOTIMPL; }
		IFACEMETHODIMP OnOverwrite(IFileDialog* pfd, IShellItem* psi,
			FDE_OVERWRITE_RESPONSE* pResponse) { return E_NOTIMPL; }


	private:
		long references;
	}; // OpenFileCallback


	HRESULT OpenFileCallback_CreateInstance(REFIID riid, void** ppv)
	{
		*ppv = nullptr;

		HRESULT result = E_OUTOFMEMORY;

		OpenFileCallback* callback = new OpenFileCallback();
		if (callback)
		{
			result = callback->QueryInterface(riid, ppv);
			//callback->Release();
		}

		return result;
	}

	Result show_open_dialog(const char* title, uint32_t open_flags, Array<PathString>& paths, open_dialog_event_handler event_handler)
	{
		bool result = false;
		WCHAR* buffer = nullptr;

		IFileDialog* filedialog;
		if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&filedialog))))
		{
			DWORD dwOptions;
			if (SUCCEEDED(filedialog->GetOptions(&dwOptions)))
			{
				if (open_flags & OpenDialogFlags::CanChooseDirectories)
				{
					filedialog->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST);
				}
				else if (open_flags & OpenDialogFlags::CanChooseFiles)
				{
					filedialog->SetOptions(dwOptions | FOS_FILEMUSTEXIST);
				}
				else
				{
					// I don't know what to display here.
					assert(0);
				}
			}

			IFileDialogEvents* dialog_events;
			OpenFileCallback_CreateInstance(IID_PPV_ARGS(&dialog_events));
			DWORD cookie;
			HRESULT advise_result = filedialog->Advise(dialog_events, &cookie);
			if (FAILED(advise_result))
			{
				assert(0);
			}

			OpenFileCallback* callback = reinterpret_cast<OpenFileCallback*>(dialog_events);
			callback->set_handler(event_handler);

			filedialog->SetTitle(L"Choose...");

			if (SUCCEEDED(filedialog->Show(NULL)))
			{
				IShellItem *psi;
				if (SUCCEEDED(filedialog->GetResult(&psi)))
				{
					if (!SUCCEEDED(psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, (LPWSTR*)&buffer)))
					{
						return Result::failure("GetDisplayName failed");
					}

					platform::PathString filename;
					win32::wide_to_ascii(filename, buffer);
					CoTaskMemFree(buffer);
					result = true;
					paths.push_back(filename);
					psi->Release();
				}
			}
			filedialog->Unadvise(cookie);
			dialog_events->Release();
			filedialog->Release();
		}

		if (result)
		{
			return Result::success();
		}

		return Result::failure("User canceled dialog");
	} // show_open_dialog

	Result show_save_dialog(const char* title,
							uint32_t save_flags,
							const Array<PlatformExtensionDescription>& extensions,
							const PathString& default_extension,
							PathString& filename)
	{
		char filename_buffer[MAX_PATH_SIZE] = { 0 };

		char* filter_buffer = create_filter_string(extensions);

		OPENFILENAMEA info;
		ZeroMemory(&info, sizeof(OPENFILENAMEA));

		info.lStructSize = sizeof(OPENFILENAMEA);
		info.hwndOwner = GetFocus();

		info.lpstrTitle = title;
		info.lpstrFilter = filter_buffer;
		info.lpstrFile = filename_buffer;
		info.nMaxFile = MAX_PATH_SIZE;

		info.Flags = OFN_OVERWRITEPROMPT;

		if (save_flags & OpenDialogFlags::ShowHiddenFiles)
		{
			info.Flags |= OFN_FORCESHOWHIDDEN;
		}

		if (save_flags & OpenDialogFlags::AllowMultiselect)
		{
			info.Flags |= OFN_ALLOWMULTISELECT;
		}

		info.lpstrDefExt = default_extension();

		if (GetSaveFileNameA(&info))
		{
			MEMORY2_DEALLOC(get_platform_allocator2(), filter_buffer);
			filename = filename_buffer;
			return Result::success();
		}

		MEMORY2_DEALLOC(get_platform_allocator2(), filter_buffer);

		return Result::failure("User canceled dialog.");
	} // show_save_dialog


} // namespace platform
