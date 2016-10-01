// -------------------------------------------------------------
// Copyright (C) 2016- Adam Petrone
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

#include <ui/ui.h>
#include <ui/utils.h>
#include <ui/events.h>

namespace gui
{
	const char* event_type_to_string(EventType type)
	{
		switch (type)
		{
			case Invalid: return "Invalid";
			case Event_CursorEnter: return "Event_CursorEnter";
			case Event_CursorExit: return "Event_CursorExit";
			case Event_FocusGain: return "Event_FocusGain";
			case Event_FocusLost: return "Event_FocusLost";
			case Event_CursorMove: return "Event_CursorMove";
			case Event_CursorDrag: return "Event_CursorDrag";
			case Event_CursorDropMove: return "Event_CursorDropMove";
			case Event_CursorDragEnter: return "Event_CursorDragEnter";
			case Event_CursorDragExit: return "Event_CursorDragExit";
			case Event_CursorDrop: return "Event_CursorDrop";
			case Event_CursorScroll: return "Event_CursorScroll";
			case Event_CursorButtonPressed: return "Event_CursorButtonPressed";
			case Event_CursorButtonReleased: return "Event_CursorButtonReleased";
			case Event_KeyButtonPressed: return "Event_KeyButtonPressed";
			case Event_KeyButtonReleased: return "Event_KeyButtonReleased";
			case Event_Click: return "Event_Click";
			default: return "Invalid";
		}

		return "Invalid";
	} // event_type_to_string

} // namespace gui
