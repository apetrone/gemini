// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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
#pragma once

#include "config.h"

//
// Helpful Macros
//

// This can be used to essentially remove macros by setting a macro's definition to this instead
// Ex: #define MyMacro(x) NULL_MACRO
#define NULL_MACRO (void(0))

// unit <desired> per <have>
const double SecondsPerMillisecond = 1.0e-3;
const double SecondsPerMicrosecond = 1.0e-6;
const double SecondsPerNanosecond = 1.0e-9;

const double MillisecondsPerSecond = 1.0e3;
const double MillisecondsPerMicrosecond = 1.0e-3;

const double MicrosecondsPerMillisecond = 1.0e3;
const double MicrosecondsPerSecond = 1.0e6;
const double MicrosecondsPerNanosecond = 1.0e-3;

const double NanosecondsPerSecond = 1.0e9;
const double NanosecondsPerMicrosecond = 1.0e3;

// prefer the C99 types
#include <stdint.h>
#include <assert.h>

#include "mem.h"

