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

#include <runtime/jobqueue.h>

#include <platform/platform.h>
#include <core/logging.h>

#define JQDEBUG(...) NULL_MACRO
//#define JQDEBUG LOGV

namespace gemini
{
	void job_processor(platform::Thread* thread)
	{
		JQDEBUG("---------------> enter job_processor, thread: 0x%x\n", platform::thread_id());
		JobQueue::worker_data* worker = static_cast<JobQueue::worker_data*>(thread->user_data);
		JobQueue* queue = worker->queue;

		JobQueue::Job entry;
		entry.valid = 0;
		while (platform::thread_is_active(thread))
		{
			entry = queue->pop();
			if (entry.valid)
			{
				assert(entry.valid);
				assert(entry.execute);

				// execute the job
				entry.execute(entry.data);

				// mark it as complete
				queue->complete_job(entry);
			}
			else if (worker->is_active) // can this thread sleep?
			{
				// no available jobs. wait for signal.
				JQDEBUG("---------------> thread 0x%x going to sleep...\n", platform::thread_id());
				worker->queue->sleep_worker();
				JQDEBUG("---------------> thread 0x%x waking up...\n", platform::thread_id());
			}
		}

		JQDEBUG("---------------> exit job_processor, thread: 0x%x\n", platform::thread_id());
	}

	void JobQueue::create_workers(uint32_t max_workers)
	{
		// Reserve space for worker data: this is required because Array<T>
		// doesn't use re-alloc -- and once the data is passed
		// to the individual threads on startup, it can blow up.
		workers.resize(max_workers);

		for (uint32_t index = 0; index < max_workers; ++index)
		{
			worker_data* data = &workers[index];
			data->worker_index = index;
			data->queue = this;
			data->is_active = 1;

			data->thread = platform::thread_create(job_processor, data);
			assert(data->thread);
		}

		// If you hit this, create_workers was called more than once.
		assert(semaphore == nullptr);

		semaphore = platform::semaphore_create(0, max_workers);
		assert(semaphore);
	}

	void JobQueue::destroy_workers()
	{
		// set all threads as inactive
		// this should be done as a single transaction.
		for (uint32_t index = 0; index < workers.size(); ++index)
		{
			worker_data& worker = workers[index];
			worker.is_active = 0;
			JQDEBUG("set worker %i as inactive.\n", index);
		}

		// wake ALL workers (in case they were sleeping)
		// since this just increments a semaphore, it has to be called
		// once for each thread.
		wake_workers(true);

		for (worker_data& worker : workers)
		{
			// wait for the threads to join in a timely fashion
			platform::thread_join(worker.thread, 2500);

			// cleanup memory
			platform::thread_destroy(worker.thread);
			worker.queue = nullptr;
		}

		workers.clear();

		assert(semaphore);
		platform::semaphore_destroy(semaphore);
		semaphore = nullptr;
	}

	void JobQueue::wake_workers(bool all_workers)
	{
		if (semaphore)
		{
			uint32_t increment = 1;
			if (all_workers)
			{
				// Shouldn't have more than uint32_t workers.
				increment = static_cast<uint32_t>(workers.size());
			}
			platform::semaphore_signal(semaphore, increment);
		}
	}

	void JobQueue::sleep_worker()
	{
		if (semaphore)
		{
			platform::semaphore_wait(semaphore);
		}
	}

	void JobQueue::push_back(JobExecuteFunction execute_function, const char* data)
	{
		// This is only intended to be called from a single thread.
		uint32_t write_index = (next_write_index + 1) & (MAX_QUEUE_ITEMS - 1);

		// The queue is full. Attempting to write at this stage
		// would invalidate the queue's state.
		assert(write_index != next_read_index);

		assert(next_write_index < MAX_QUEUE_ITEMS);
		JQDEBUG("set '%s' to job %i\n", data, (int32_t)next_write_index);
		Job* entry = &queue[static_cast<size_t>(next_write_index)];
		entry->execute = execute_function;
		entry->data = data;

		// Needed because the compiler OR processor could re-order writes.
		PLATFORM_MEMORY_FENCE();
		next_write_index = write_index;
		++total_jobs;

		wake_workers();
	}

	JobQueue::Job JobQueue::pop()
	{
		JobQueue::Job entry;
		entry.data = nullptr;
		entry.execute = nullptr;
		entry.valid = 0;

		uint32_t read_index = next_read_index;
		uint32_t incremented_read_index = (read_index + 1) & (MAX_QUEUE_ITEMS - 1);
		if (read_index != next_write_index)
		{
			if (atom_compare_and_swap32(&next_read_index, incremented_read_index, read_index))
			{
				read_index = (read_index)& (MAX_QUEUE_ITEMS - 1);
				JQDEBUG("thread: 0x%x taking job: %i\n", platform::thread_id(), read_index);
				assert(read_index < MAX_QUEUE_ITEMS);
				JobQueue::Job& item = queue[read_index];
				entry.data = item.data;
				entry.execute = item.execute;
				entry.valid = 1;
				PLATFORM_MEMORY_FENCE();
			}
		}
		return entry;
	}

	void JobQueue::wait_for_jobs_to_complete()
	{
		while (total_jobs != jobs_completed);
	}

	void JobQueue::complete_job(const JobQueue::Job& job)
	{
		assert(job.valid);
		if (job.valid)
		{
			// job has completed.
			atom_increment32(&jobs_completed);
		}
	}
} // namespace gemini
