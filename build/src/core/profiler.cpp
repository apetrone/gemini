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
#include <core/logging.h>

namespace gemini
{
#if defined(GEMINI_ENABLE_PROFILER)
	namespace profiler
	{
		typedef HashSet<const char*, ProfileBlock*> ScopeToProfileBlockHashSet;

		struct ProfilerState
		{
			Array<ProfileBlock*> scopes;
			size_t depth;
			uint64_t overhead;
			gemini::stack<ProfileBlock*> profile_stack;
			ScopeToProfileBlockHashSet blocks;
			ProfileBlock* current_scope;

			ProfilerState(gemini::Allocator& allocator)
				: scopes(allocator)
				, depth(0)
				, overhead(0)
				//, profile_stack(allocator)
				, blocks(allocator)
				, current_scope(nullptr)
			{
			}
		}; // ProfilerState


		gemini::Allocator* _allocator = nullptr;
		ProfilerState* _state = nullptr;

		void default_profile_output(const char* name, uint64_t cycles, uint32_t depth, uint32_t hitcount, float parent_weight)
		{
			size_t indents = 0;
			while (indents <= depth)
			{
				LOGV("-");
				++indents;
			}

			LOGV(" %s, cycles: %llu, hits: %i, pct: %2.3f cycles/hit: %2.2f\n", name, cycles, hitcount, parent_weight * 100.0, cycles / (float)hitcount);
		}

		ProfileBlock* find_or_create_block(const char* name)
		{
			if (_state->blocks.has_key(name))
				return _state->blocks.get(name);

			ProfileBlock* block = new ProfileBlock();
			block->index = static_cast<uint32_t>(_state->scopes.size());
			_state->scopes.push_back(block);
			_state->blocks[name] = block;
			return block;
		}

		void begin_scope(const char* name, const char* /*fancy_name*/)
		{
			ProfileBlock* block = find_or_create_block(name);
			block->parent_index = _state->current_scope ? _state->current_scope->index : block->index;
			block->cycles -= platform::time_ticks();
			block->name = name;
			block->depth = static_cast<uint32_t>(_state->depth++);
			block->hitcount++;
			_state->profile_stack.push(block);
			_state->current_scope = block;
		}

		void end_scope(const char* /*name*/, const char* /*fancy_name*/)
		{
			ProfileBlock* scope = _state->profile_stack.top();
			scope->cycles += platform::time_ticks() - _state->overhead;

			--_state->depth;

			// restore the last scope
			_state->profile_stack.pop();
			if (_state->profile_stack.empty())
			{
				_state->current_scope = nullptr;
			}
			else
			{
				_state->current_scope = _state->profile_stack.top();
			}
		}

		void report(profile_callback callback)
		{
			if (!callback)
			{
				callback = &default_profile_output;
			}

			for (ProfileBlock* scope : _state->scopes)
			{
				const ProfileBlock* parent = _state->scopes[scope->parent_index];
				const float parent_weight = scope->cycles / float(parent->cycles);
				callback(scope->name, scope->cycles, scope->depth, scope->hitcount, parent_weight);
			}
		}

		void reset()
		{
			_state->depth = 0;
			_state->profile_stack.clear(false);
			_state->current_scope = nullptr;
			_state->scopes.clear(false);
			_state->blocks.clear(/*false*/);
		}

		void startup(gemini::Allocator& allocator)
		{
			_allocator = &allocator;

			_state = MEMORY2_NEW(allocator, ProfilerState)(allocator);

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
			// and this will allow us to simply subtract overhead.
			_state->overhead = (sum / 11) * 2;
		}

		void shutdown()
		{
			for (ProfileBlock* block : _state->scopes)
			{
				delete block;
			}
			_state->scopes.clear();
			_state->profile_stack.clear();
			_state->blocks.clear();

			MEMORY2_DELETE(*_allocator, _state);
		}

		static_assert(sizeof(profiler::ProfileBlock) == 32, "ProfileBlock is not aligned on cache line.");
	} // namespace profiler
#endif
} // namespace gemini