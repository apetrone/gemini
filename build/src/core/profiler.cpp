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
#include "profiler.h"
#include "array.h"
#include "hashset.h"
#include "stack.h"

#include <platform/platform.h>

namespace gemini
{
#if defined(GEMINI_ENABLE_PROFILER)
	namespace profiler
	{
		Array<profile_block*> scopes;
		profile_block* current_scope = nullptr;

		typedef HashSet<const char*, profile_block*> profile_hash_t;
		profile_hash_t blocks;
		gemini::stack<profile_block*> profile_stack;
		size_t depth = 0;
		uint64_t overhead = 0;

		profile_block* find_or_create_block(const char* name)
		{
			if (blocks.has_key(name))
				return blocks.get(name);

			profile_block* block = new profile_block();
			block->index = static_cast<uint32_t>(scopes.size());
			scopes.push_back(block);
			blocks[name] = block;
			return block;
		}

		void begin_scope(const char* name, const char* /*fancy_name*/)
		{
			profile_block* block = find_or_create_block(name);
			block->parent_index = current_scope ? current_scope->index : block->index;
			block->cycles -= platform::time_ticks();
			block->name = name;
			block->depth = static_cast<uint32_t>(depth++);
			block->hitcount++;
			profile_stack.push(block);
			current_scope = block;
		}

		void end_scope(const char* /*name*/, const char* /*fancy_name*/)
		{
			profile_block* scope = profile_stack.top();
			scope->cycles += platform::time_ticks() - overhead;

			--depth;

			// restore the last scope
			profile_stack.pop();
			if (profile_stack.empty())
			{
				current_scope = nullptr;
			}
			else
			{
				current_scope = profile_stack.top();
			}
		}

		void report(profile_callback callback)
		{
			for (profile_block* scope : scopes)
			{
				const profile_block* parent = scopes[static_cast<int>(scope->parent_index)];
				const float parent_weight = scope->cycles / float(parent->cycles);
				callback(scope->name, scope->cycles, scope->depth, scope->hitcount, parent_weight);
			}
		}

		void reset()
		{
			depth = 0;
			profile_stack.clear(false);
			current_scope = nullptr;
			scopes.clear(false);
			blocks.clear(/*false*/);
		}

		void startup()
		{
			// calculate an average overhead for ticks
			uint64_t sum = 0;
			for (uint64_t count = 0; count < 11; ++count)
			{
				uint64_t a, b;
				a = platform::time_ticks();
				b = platform::time_ticks();
				sum += (b-a);
			}

			// multiply by two since we call ticks TWICE for a scope
			// and this will allow us to simply subtrack overhead.
			overhead = (sum / 11) * 2;
		}

		void shutdown()
		{
			for (profile_block* block : scopes)
			{
				delete block;
			}
			scopes.clear();
			profile_stack.clear();
			blocks.clear();
		}

		static_assert(sizeof(profiler::profile_block) == 32, "profile_block is not aligned on cache line.");
	} // namespace profiler
#endif
} // namespace gemini