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
#include "unit_test.h"

#include <core/core.h>
#include <core/logging.h>
#include <core/atomic.h>

#include <platform/platform.h>

#include <runtime/runtime.h>
#include <runtime/filesystem.h>

#include <assert.h>

// ---------------------------------------------------------------------
// filesystem
// ---------------------------------------------------------------------
UNITTEST(filesystem)
{
	core::filesystem::IFileSystem* fs = core::filesystem::instance();
	TEST_ASSERT(fs != nullptr, filesystem_instance_exists);

	//	platform::PathString content_path;
	//	content_path = fs->root_directory();
	//	content_path.append(PATH_SEPARATOR_STRING).append("builds").append(PATH_SEPARATOR_STRING).append(PLATFORM_NAME);
	//	fs->content_directory(content_path);


	//	platform::PathString absolute_path;
	//	TEST_ASSERT(fs->get_absolute_path_for_content(absolute_path, "conf/shaders.conf") == false, get_absolute_path_for_content_missing);
}

// ---------------------------------------------------------------------
// logging
// ---------------------------------------------------------------------
UNITTEST(logging)
{
	TEST_ASSERT(core::logging::instance() != nullptr, log_instance_is_valid);

	LOGV("This is a test of the logging system!\n");

	LOGW("This is a warning\n");
	LOGE("This is an error!\n");

	LOGW("Warning, %i parameters missing!\n", 3);
}

#if 0
// Asset system re-design.
* should support streaming and reloading
* should just be a cache of data. This should not have strong dependencies on the renderer.
* allow assets to live in the runtime; not 'engine' project
* asset handle struct:
	This should guard against direct pointer access to assets.
	Allows validation of the handle
* asset return codes/statuses
	Report: success, failure, pending, loaded, evicted?
* assets should be loaded by relative file name (without file extension)
* internally manage refcount for acquire/release (or lock/unlock)
#endif

namespace gemini
{
	enum class AssetType
	{
		Model,
		Material,
		Texture,
		SoundEffect
	};

	class AssetHandle
	{
	private:
		size_t reference;
		AssetType type;

	public:

		bool valid() const;

		friend class AssetLibrary;
	};


	AssetHandle model_by_path(const char* path);
	AssetHandle material_by_path(const char* path);
	AssetHandle texture_by_path(const char* path);


} // namespace gemini


namespace gemini
{
	class ModelAsset
	{
	private:
		void* data;

	public:
		ModelAsset()
			: data(nullptr)
		{
		}
	};

	class AssetLibrary
	{
	private:
		Array<ModelAsset*> models;
		Array<AssetHandle> model_handles;

		// for experimentation
		size_t frame_number;


		void process_load_queue()
		{
		}

		void queue_load(const char* path)
		{
			fprintf(stdout, "queue model load: %s\n", path);
		}

		void process_evictions()
		{
		}


	public:

		template <AssetType type>
		AssetHandle get_handle(const char* path)
		{
			AssetHandle handle;
			handle.reference = 0;
			models.push_back(nullptr);

			// queue handle.reference with path
			queue_load(path);

			return handle;
		}

		bool validate(const AssetHandle& handle)
		{
			ModelAsset* asset = models[handle.reference];
			if (asset)
			{
				return true;
			}

			return false;
		}

		void update()
		{
			if (frame_number == 10)
			{
				// process queue
				process_load_queue();
			}
			else if (frame_number == 30)
			{
				// process evictions
				process_evictions();
			}


			++frame_number;
		}
	};

	static AssetLibrary lib;


	AssetHandle model_by_path(const char* path)
	{
		// this should be an async call
		AssetHandle handle;

		handle = lib.get_handle<AssetType::Model>(path);

		return handle;
	}
}

const size_t MAX_THREADS = 3;
namespace gemini
{
	// Single producer, multi-consumer job queue.
	// This creates a number of threads and executes jobs on them.
	class job_queue
	{
	public:
		struct worker_data
		{
			uint32_t worker_index;
			platform::Thread* thread;
			class job_queue* queue;
			int32_t is_active;
		};

		typedef void (*process_job)(const char* user_data);

		struct job
		{
			uint32_t valid;
			const char* data;
			process_job execute;
		};

	private:
		// this must be a power of two
		static const size_t MAX_QUEUE_ITEMS = 16;
		static_assert(((MAX_QUEUE_ITEMS - 1) & MAX_QUEUE_ITEMS) == 0, "MAX_QUEUE_ITEMS must be a power of two!");

		atomic<int32_t> total_jobs;
		atomic<int32_t> next_write_index;
		atomic<int32_t> next_read_index;
		atomic<int32_t> jobs_completed;

		job queue[MAX_QUEUE_ITEMS];
		Array<worker_data> workers;

		platform::Semaphore* semaphore;

	public:
		job_queue() :
			total_jobs(0),
			next_write_index(0),
			next_read_index(0),
			jobs_completed(0),
			semaphore(nullptr)
		{
		}

		~job_queue()
		{
		}

		// create max_workers
		void create_workers(size_t max_workers);

		// destroys all workers
		void destroy_workers();

		// wake up worker threads
		void wake_workers(bool all_workers = false);

		// puts this worker thread to sleep
		void sleep_worker();

		// push back a new job onto the queue
		void push_back(process_job execute_function, const char* data);

		// Pop an available job off the queue
		// Check Job's valid flag before operating on it.
		job pop();

		// block waiting for all queued jobs to complete
		void wait_for_jobs_to_complete();

		// explicitly mark a job as complete. TODO: ugh, get rid of this.
		void complete_job(const job_queue::job& job);
	}; // class job_queue
} // namespace gemini

// http://nullprogram.com/blog/2014/09/02/
// https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html
// http://en.cppreference.com/w/c/atomic
// http://en.cppreference.com/w/cpp/atomic/memory_order

#if 0
* declaration: volatile
* load/store
* memory fences (mostly compiler barriers)
	lightweight: orders loads from memory. orders stores to memory
		- atomic_thread_fence(acquire/release/acq_rel)
	full fence: lightweight + commits stores before next load
		- atomic_thread_fence(memory_order_seq_cst)
* read/modify/write
	- AtomicIncrement, AtomicCompareExchange

* use atomic operations for high contention objects

#endif

namespace gemini
{
	void job_processor(platform::Thread* thread)
	{
		LOGV("---------------> enter job_processor, thread: 0x%x\n", platform::thread_id());
		job_queue::worker_data* worker = static_cast<job_queue::worker_data*>(thread->user_data);
		job_queue* queue = worker->queue;

		job_queue::job entry;
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
//				LOGV("---------------> thread 0x%x going to sleep...\n", platform::thread_id());
				worker->queue->sleep_worker();
//				LOGV("---------------> thread 0x%x waking up...\n", platform::thread_id());
			}
		}

		LOGV("---------------> exit job_processor, thread: 0x%x\n", platform::thread_id());
	}

	void job_queue::create_workers(size_t max_workers)
	{
		// Reserve space for worker data: this is required because Array<T>
		// doesn't use re-alloc -- and once the data is passed
		// to the individual threads on startup, it can blow up.
		workers.resize(max_workers);

		for (size_t index = 0; index < max_workers; ++index)
		{
			worker_data* data = &workers[index];
			data->worker_index = static_cast<uint32_t>(index);
			data->queue = this;
			data->is_active = 1;

			data->thread = platform::thread_create(job_processor, data);
			assert(data->thread);
		}

		semaphore = platform::semaphore_create(0, static_cast<int32_t>(max_workers));
		assert(semaphore);
	}

	void job_queue::destroy_workers()
	{
		// set all threads as inactive
		// this should be done as a single transaction.
		for (size_t index = 0; index < workers.size(); ++index)
		{
			worker_data& worker = workers[index];
			worker.is_active = 0;
			LOGV("set worker %i as inactive.\n", index);
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
		platform::semaphore_destroy(semaphore);
		semaphore = nullptr;
	}

	void job_queue::wake_workers(bool all_workers)
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

	void job_queue::sleep_worker()
	{
		if (semaphore)
		{
			platform::semaphore_wait(semaphore);
		}
	}

	void job_queue::push_back(process_job execute_function, const char* data)
	{
		// This is only intended to be called from a single thread.
		int32_t write_index = (next_write_index + 1) & (MAX_QUEUE_ITEMS - 1);

		// The queue is full. Attempting to write at this stage
		// would invalidate the queue's state.
		assert(write_index != next_read_index);

		assert(next_write_index < MAX_QUEUE_ITEMS);
		LOGV("set '%s' to job %i\n", data, (int32_t)next_write_index);
		job* entry = &queue[static_cast<size_t>(next_write_index)];
		entry->execute = execute_function;
		entry->data = data;

		// Needed because the compiler OR processor could re-order writes.
		PLATFORM_MEMORY_FENCE();
		next_write_index = write_index;
		++total_jobs;

		wake_workers();
	}

	job_queue::job job_queue::pop()
	{
		job_queue::job entry;
		entry.data = nullptr;
		entry.execute = nullptr;
		entry.valid = 0;

		int32_t read_index = next_read_index;
		int32_t incremented_read_index = (read_index + 1) & (MAX_QUEUE_ITEMS - 1);
		if (read_index != next_write_index)
		{
			if (atom_compare_and_swap32(&next_read_index, incremented_read_index, read_index))
			{
				read_index = (read_index) & (MAX_QUEUE_ITEMS - 1);
				LOGV("thread: 0x%x taking job: %i\n", platform::thread_id(), read_index);
				assert(read_index < MAX_QUEUE_ITEMS);
				job_queue::job& item = queue[read_index];
				entry.data = item.data;
				entry.execute = item.execute;
				entry.valid = 1;
				PLATFORM_MEMORY_FENCE();
			}
		}
		return entry;
	}

	void job_queue::wait_for_jobs_to_complete()
	{
		while(total_jobs != jobs_completed);
	}

	void job_queue::complete_job(const job_queue::job& job)
	{
		assert(job.valid);
		if (job.valid)
		{
			// job has completed.
			atom_increment32(&jobs_completed);
		}
	}
} // namespace gemini

void print_string(const char* data)
{
	LOGV("thread: 0x%x, string: %s\n", (size_t)platform::thread_id(), data);
}


// ---------------------------------------------------------------------
// configloader
// ---------------------------------------------------------------------

int main(int, char** argv)
{
	gemini::core_startup();
	gemini::runtime_startup("arcfusion.net/gemini/test_runtime");

	size_t max_iterations = atoi(argv[1]);
	if (max_iterations > 6)
		max_iterations = 6;

	LOGV("using iterations: %i (capped at 5)\n", max_iterations);

	using namespace gemini;

	job_queue jq;
	jq.create_workers(MAX_THREADS);

	const char* iterations[] = {
		"ALPHA: 0",
		"ALPHA: 1",
		"ALPHA: 2",
		"ALPHA: 3",
		"ALPHA: 4",
		"ALPHA: 5",
		"ALPHA: 6",
		"ALPHA: 7",
		"ALPHA: 8",
		"ALPHA: 9",
		"BETA: 0",
		"BETA: 1",
		"BETA: 2",
		"BETA: 3",
		"BETA: 4",
		"BETA: 5",
		"BETA: 6",
		"BETA: 7",
		"BETA: 8",
		"BETA: 9",
		"DELTA: 0",
		"DELTA: 1",
		"DELTA: 2",
		"DELTA: 3",
		"DELTA: 4",
		"DELTA: 5",
		"DELTA: 6",
		"DELTA: 7",
		"DELTA: 8",
		"DELTA: 9",
		"EPSILON: 0",
		"EPSILON: 1",
		"EPSILON: 2",
		"EPSILON: 3",
		"EPSILON: 4",
		"EPSILON: 5",
		"EPSILON: 6",
		"EPSILON: 7",
		"EPSILON: 8",
		"EPSILON: 9",
		"FOXTROT: 0",
		"FOXTROT: 1",
		"FOXTROT: 2",
		"FOXTROT: 3",
		"FOXTROT: 4",
		"FOXTROT: 5",
		"FOXTROT: 6",
		"FOXTROT: 7",
		"FOXTROT: 8",
		"FOXTROT: 9",
		"GAMMA: 0",
		"GAMMA: 1",
		"GAMMA: 2",
		"GAMMA: 3",
		"GAMMA: 4",
		"GAMMA: 5",
		"GAMMA: 6",
		"GAMMA: 7",
		"GAMMA: 8",
		"GAMMA: 9"
	};

	for (size_t index = 0; index < max_iterations; ++index)
	{
		jq.push_back(print_string, iterations[(index*10)+0]);
		jq.push_back(print_string, iterations[(index*10)+1]);
		jq.push_back(print_string, iterations[(index*10)+2]);
		jq.push_back(print_string, iterations[(index*10)+3]);
		jq.push_back(print_string, iterations[(index*10)+4]);
		jq.push_back(print_string, iterations[(index*10)+5]);
		jq.push_back(print_string, iterations[(index*10)+6]);
		jq.push_back(print_string, iterations[(index*10)+7]);
		jq.push_back(print_string, iterations[(index*10)+8]);
		jq.push_back(print_string, iterations[(index*10)+9]);

		jq.wait_for_jobs_to_complete();

		platform::thread_sleep(250);
	}

	LOGV("destroying workers...\n");
	jq.destroy_workers();

//	unittest::UnitTest::execute();
	gemini::runtime_shutdown();
	gemini::core_shutdown();
	return 0;
}
