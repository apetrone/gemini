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
#pragma once

#include <sdk/entity.h>

#include <core/typedefs.h>
#include <core/mem.h>
#include <core/stackstring.h>

#include <vector>

// ---------------------------------------------------------------------
// Behavior Trees experimentation
// ---------------------------------------------------------------------
namespace behavior
{
	typedef std::vector<struct Behavior*> TreeNodeArray;

	typedef core::StackString<32> BehaviorName;

	// This is designed around the same principles presented by Alex J. Champandard
	// in the talk 'Understanding the Second Generation of Behavior Trees and Preparing
	// for Challenges Beyond'
	// This wiki entry from libgdx is also very helpful:
	// https://github.com/libgdx/gdx-ai/wiki/Behavior-Trees

	enum BehaviorStatus
	{
		Behavior_Invalid,
		Behavior_Succeeded,
		Behavior_Failed,
		Behavior_Running,
	};

	const char* name_from_status(BehaviorStatus status);

	struct BehaviorContext
	{
		size_t depth;

		BehaviorContext()
			: depth(0)
		{
		}

		virtual ~BehaviorContext() {}

		virtual void push(struct Behavior* behavior) = 0;
		virtual void pop(struct Behavior* behavior) = 0;
		virtual void visit(struct Behavior* behavior, BehaviorStatus status) = 0;
		virtual void populate(struct Behavior* behavior) = 0;
		virtual void log(const char* message) = 0;
	};

	struct Behavior
	{
		BehaviorStatus status;
		BehaviorName name;

		Behavior(const BehaviorName& behavior_name);
		virtual ~Behavior();

		virtual BehaviorStatus update(Entity* entity, BehaviorContext* context) = 0;

		virtual void activate();
		virtual void deactivate(Entity* entity, behavior::BehaviorContext* context);

		bool is_active() const;

		BehaviorStatus tick(Entity* entity, BehaviorContext* context);
		virtual void populate(BehaviorContext* context);
		virtual const char* get_classname() const { return "Behavior"; }
	};

	struct Condition : public Behavior
	{
		Behavior* next;

		Condition(const BehaviorName& behavior_name, Behavior& next_behavior)
			: Behavior(behavior_name)
			, next(&next_behavior)
		{
		}

		virtual void deactivate(Entity* entity, behavior::BehaviorContext* context)
		{
			if (next && next->is_active())
			{
				next->deactivate(entity, context);
			}

			Behavior::deactivate(entity, context);
		}

		virtual behavior::BehaviorStatus update(Entity* entity, BehaviorContext* context)
		{
			BehaviorStatus result = status;

			if (result == Behavior_Succeeded)
			{
				// we need to reset the condition if it succeeds
				// so that it is checked next tick.
				status = Behavior_Invalid;
				return next->tick(entity, context);
			}

			return result;
		}

		virtual void populate(BehaviorContext* context)
		{
			context->populate(this);
			context->push(this);
			next->populate(context);
			context->pop(this);
		}

		virtual const char* get_classname() const { return "Condition"; }
	};


	struct Composite : public Behavior
	{
		TreeNodeArray children;
		TreeNodeArray::iterator current_child;
		TreeNodeArray::iterator active_child;

		Composite(const BehaviorName& behavior_name)
			: Behavior(behavior_name)
			, current_child(children.end())
			, active_child(children.end())
		{
		}

		void add_child(Behavior* child)
		{
			children.push_back(child);
			active_child = children.end();
		}

		virtual void populate(BehaviorContext* context)
		{
			context->populate(this);
			context->push(this);
			for (auto& child : children)
			{
				child->populate(context);
			}
			context->pop(this);
		}

		virtual const char* get_classname() const { return "Composite"; }
	};

	// iterates over a fixed list of behaviors
	struct Sequence : public Composite
	{
		size_t child_index;

		Sequence(const BehaviorName& behavior_name)
			: Composite(behavior_name)
		{
		}

		virtual void activate()
		{
			current_child = children.begin();
			status = Behavior_Running;
			child_index = 0;
		}

		virtual BehaviorStatus update(Entity* entity, BehaviorContext* context)
		{
			assert(!children.empty());
			while (true)
			{
				Behavior* child = *current_child;
				context->log(core::str::format("[%s] ticking child_index: %i (%s)\n", name(), child_index, child->name()));
				BehaviorStatus result = child->tick(entity, context);
				if (result != Behavior_Succeeded)
				{
					if (active_child != current_child)
					{
						if (active_child != children.end())
						{
							Behavior* previous_active_child = *active_child;
							previous_active_child->deactivate(entity, context);
						}
						active_child = current_child;
					}

					context->log(core::str::format("[%s] result of child: '%s'\n", name(), behavior::name_from_status(result)));
					return result;
				}

				// next child
				++current_child;
				if (current_child == children.end())
				{
					active_child = children.end();
					context->log(core::str::format("[%s] end of child array; returning failed\n", name()));
					return Behavior_Succeeded;
				}
			}

			// It is expected to never reach this
			// because we need to iterate all children and then return.
			assert(0);
			return Behavior_Invalid;
		}

		virtual void deactivate(Entity* entity, BehaviorContext* context)
		{
			if (active_child != children.end())
			{
				Behavior* child = *active_child;
				child->deactivate(entity, context);
			}

			Behavior::deactivate(entity, context);
		}

		virtual const char* get_classname() const { return "Sequence"; }
	};

	// find the first behavior that succeeds
	struct Selector : public Composite
	{

		size_t child_index;

		Selector(const BehaviorName& behavior_name) :
		Composite(behavior_name)
		{
		}

		virtual void activate()
		{
			current_child = children.begin();
			status = Behavior_Running;
			child_index = 0;
		}

		virtual BehaviorStatus update(Entity* entity, BehaviorContext* context)
		{
			assert(!children.empty());
			while (true)
			{
				Behavior* child = *current_child;
				context->log(core::str::format("[%s] ticking child_index: %i (%s)\n", name(), child_index, child->name()));
				BehaviorStatus result = child->tick(entity, context);
				if (result == Behavior_Invalid)
				{
					assert(0);
				}
				if (result != Behavior_Failed)
				{
					// we reset the selector because it needs to re-evaluate
					// all children from the beginning.
					status = Behavior_Invalid;

					if (active_child != current_child)
					{
						if (active_child != children.end())
						{
							Behavior* previous_active_child = *active_child;
							previous_active_child->deactivate(entity, context);
						}
						active_child = current_child;
					}


					context->log(core::str::format("[%s] result of child: '%s'\n", name(), behavior::name_from_status(result)));
					return result;
				}

				// next child
				++current_child;
				if (current_child == children.end())
				{
					active_child = children.end();
					context->log(core::str::format("[%s] end of child array; returning failed\n", name()));
					return Behavior_Failed;
				}
			}

			// It is expected to never reach this
			// because we need to iterate all children and then return.
			assert(0);
			return Behavior_Invalid;
		}

		virtual void deactivate(Entity* entity, BehaviorContext* context)
		{
			if (active_child != children.end())
			{
				Behavior* child = *active_child;
				child->deactivate(entity, context);
			}

			Behavior::deactivate(entity, context);
		}

		virtual const char* get_classname() const { return "Selector"; }
	};
} // namespace behavior
