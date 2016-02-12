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
#pragma once

#include <core/typedefs.h>
#include <core/atomic.h>

namespace platform
{
	struct Semaphore;
} // namespace platform

namespace gemini
{
	// Single producer, multi-consumer job queue.
	// This creates a number of threads and executes jobs on them.
	class JobQueue
	{
	public:
		struct worker_data
		{
			uint32_t worker_index;
			platform::Thread* thread;
			class JobQueue* queue;
			int32_t is_active;
		};

		typedef void(*JobExecuteFunction)(const char* user_data);

		struct Job
		{
			uint32_t valid;
			const char* data;
			JobExecuteFunction execute;
		};

	private:
		// this must be a power of two
		static const uint32_t MAX_QUEUE_ITEMS = 16;
		static_assert(((MAX_QUEUE_ITEMS - 1) & MAX_QUEUE_ITEMS) == 0, "MAX_QUEUE_ITEMS must be a power of two!");

		atomic<uint32_t> total_jobs;
		atomic<uint32_t> next_write_index;
		atomic<uint32_t> next_read_index;
		atomic<uint32_t> jobs_completed;

		Job queue[MAX_QUEUE_ITEMS];
		Array<worker_data> workers;

		platform::Semaphore* semaphore;

	public:
		JobQueue() :
			total_jobs(0),
			next_write_index(0),
			next_read_index(0),
			jobs_completed(0),
			semaphore(nullptr)
		{
		}

		~JobQueue()
		{
		}

		// create max_workers
		LIBRARY_EXPORT void create_workers(uint32_t max_workers);

		// destroys all workers
		LIBRARY_EXPORT void destroy_workers();

		// wake up worker threads
		LIBRARY_EXPORT void wake_workers(bool all_workers = false);

		// puts this worker thread to sleep
		LIBRARY_EXPORT void sleep_worker();

		// push back a new job onto the queue
		LIBRARY_EXPORT void push_back(JobExecuteFunction execute_function, const char* data);

		// Pop an available job off the queue
		// Check Job's valid flag before operating on it.
		LIBRARY_EXPORT Job pop();

		// block waiting for all queued jobs to complete
		LIBRARY_EXPORT void wait_for_jobs_to_complete();

		// explicitly mark a job as complete. TODO: ugh, get rid of this.
		LIBRARY_EXPORT void complete_job(const JobQueue::Job& job);
	}; // class JobQueue
} // namespace gemini
