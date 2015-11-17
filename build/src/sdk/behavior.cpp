// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.

//      * Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//       SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#include "behavior.h"


namespace behavior
{
	const char* name_from_status(BehaviorStatus status)
	{
		switch (status)
		{
			case Behavior_Invalid: return "invalid"; break;
			case Behavior_Succeeded: return "succeeded"; break;
			case Behavior_Failed: return "failed"; break;
			case Behavior_Running: return "running"; break;
		}

		return "unknown";
	}


	Behavior::Behavior(const BehaviorName& behavior_name) :
	name(behavior_name),
	status(Behavior_Invalid)
	{
	}

	void Behavior::activate()
	{
	}

	void Behavior::deactivate(Entity* entity, behavior::BehaviorContext* context)
	{
		context->log(core::str::format("[%s] deactivate\n", name()));
		status = Behavior_Invalid;
	}

	bool Behavior::is_active() const
	{
		return (status != Behavior_Invalid) && (status != Behavior_Failed);
	}

	BehaviorStatus Behavior::tick(Entity* entity, BehaviorContext* context)
	{
		if (status == Behavior_Invalid)
		{
			context->log(core::str::format("[%s] status is '%s', activating\n", name(), behavior::name_from_status(status)));
			activate();
		}


		BehaviorStatus return_status = update(entity, context);
		context->log(core::str::format("[%s] updated, result is: '%s'\n", name(), behavior::name_from_status(return_status)));

		context->visit(this, return_status);
		if (return_status != Behavior_Running)
		{
			context->log(core::str::format("[%s] status is '%s', deactivating\n", name(), behavior::name_from_status(status)));
			deactivate(entity, context);
		}

		return return_status;
	}

	void Behavior::populate(BehaviorContext* context)
	{
		context->push(this);
		context->populate(this);
		context->pop(this);
	}

} // namespace behavior
