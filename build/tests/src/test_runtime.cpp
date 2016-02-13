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

#include <pthread.h>
#include <semaphore.h>

namespace gemini
{
	class job_queue
	{
	public:
		struct worker_data
		{
			uint32_t worker_index;
			platform::Thread handle;
			class job_queue* queue;
		};

		typedef void (*process_job)(const char* user_data);

		struct job
		{
			uint32_t valid;
			const char* data;
			process_job execute;
		};

	private:


		int32_t volatile total_jobs;
		int32_t volatile next_entry;
		int32_t volatile jobs_completed;
		job queue[128];
		worker_data workers[3];

		sem_t event;

	public:


		job_queue()
			: total_jobs(0)
			, next_entry(0)
			, jobs_completed(0)
		{
		}

		~job_queue()
		{
			destroy_workers();
		}

		void create_workers(size_t max_workers); // create max_workers
		void destroy_workers(); // destroys all workers
		void wake_workers();	// wake up worker threads
		void sleep_worker(); // puts this worker thread to sleep

		void push_back(process_job execute_function, const char* data);
		job pop();

		void wait_until_complete();
		void complete_job();
	}; // class job_queue
} // namespace gemini


sem_t test;

struct work_queue_entry
{
	const char* str;
};


work_queue_entry work_queue[256];

int32_t volatile current_work = 0;
int32_t volatile next_entry = 0;
int32_t volatile completed_work = 0;

// C11 atomics
// https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html

#if defined(PLATFORM_APPLE)
	#include <libkern/OSAtomic.h>

	// docs: "This function serves as both a read and write barrier."
	#define complete_past_writes_before_future_writes() OSMemoryBarrier()
	#define complete_past_reads_before_future_reads() OSMemoryBarrier()

#elif defined(PLATFORM_WINDOWS)
	// os (_WriteBarrier) + processor fence (_mm_sfence())
	#define complete_past_writes_before_future_writes() _WriteBarrier()
	#define complete_past_reads_before_future_reads() _ReadBarrier()
#if 0
	HANDLE semaphore_handle = CreateSemaphoreEx(NULL, // security attributes,
		0, 					// number of threads awake at startup
		MAX_THREADS, 		// maximum number of threads
		NULL,				// name
		0,					// flags: reserved and must be zero
		SEMAPHORE_ALL_ACCESS
	);

	DWORD return = WaitForSingleObjectEx(semaphore_handle, // event handle
		INFINITE,			// time-out interval in milliseconds
		FALSE				// alertable
	);

	// increment the semaphore value by 'release_count'
	ReleaseSemaphore(semaphore_handle,
		release_count,
		&opt_previous_count
	);
#endif
#else
	#define complete_past_writes_before_future_writes()
	#define complete_past_reads_before_future_reads()
#endif

// memory write ordering (guaranteed on some processors, but compiler won't).

void queue_string(const char* string)
{
	assert(current_work < 256);

	work_queue_entry* entry = work_queue + current_work;
	entry->str = string;

	// Needed because the compiler OR processor could re-order writes.
	complete_past_writes_before_future_writes();

	++current_work;

	// wake-up threads
	sem_post(&test);
}

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



void thread_proc(void*)
{
	fprintf(stdout, "created thread: %zu\n", (size_t)platform::thread_id());
	for (;;)
	{
		// check for available jobs
		if (next_entry < current_work)
		{
			size_t entry_index = OSAtomicIncrement32Barrier(&next_entry) - 1;

			complete_past_reads_before_future_reads();
			work_queue_entry* entry = work_queue + entry_index;

			// job has been accepted.

			fprintf(stdout, "thread: %zu, string: %s\n", (size_t)platform::thread_id(), entry->str);

			// job has completed.
			OSAtomicIncrement32(&completed_work);
		}
		else
		{
			// no available jobs. wait for signal.
			sem_wait(&test);
		}
	}
}


namespace gemini
{
	void job_processor(void* thread_data)
	{
		job_queue::worker_data* worker = static_cast<job_queue::worker_data*>(thread_data);


		for (;;)
		{
			const char temp[] = "test";
			job_queue::job entry;
			entry.valid = 0;
			entry = worker->queue->pop();
			if (entry.valid)
			{
//				fprintf(stdout, "execute job: %i\n", worker->worker_index);
//				fprintf(stdout, "item: %s, %p\n", entry.data, entry.execute);
//				assert(entry.valid);
				entry.execute(temp);
//				worker->queue->complete_job();
			}
			else
			{
				// no available jobs. wait for signal.
				worker->queue->sleep_worker();
			}
		}
		fprintf(stdout, "exiting job processor!\n");
	}

	void job_queue::create_workers(size_t max_workers)
	{
		for (size_t index = 0; index < 3; ++index)
		{
//			workers.push_back(worker_data());
			worker_data* data = &workers[index];
			data->worker_index = index;
			data->queue = this;
			platform::thread_create(data->handle, job_processor, data);
		}

		sem_init(&event, 0, max_workers);
	}

	void job_queue::destroy_workers()
	{
		for (worker_data& worker : workers)
		{
			platform::thread_detach(worker.handle);
			worker.queue = nullptr;
		}
//		workers.clear();

		sem_destroy(&event);
	}

	void job_queue::wake_workers()
	{
		sem_post(&event);
	}

	void job_queue::sleep_worker()
	{
		sem_wait(&event);
	}

	void job_queue::push_back(process_job execute_function, const char* data)
	{
		assert(next_entry < 127);

		// this is only intended to be called from a single thread.
		// otherwise, this should have a load/acquire barrier

		size_t index = total_jobs;
		job* entry = &queue[index];
		entry->execute = execute_function;
		entry->data = data;

		// Needed because the compiler OR processor could re-order writes.
		complete_past_writes_before_future_writes();
		++total_jobs;
		wake_workers();
	}

	job_queue::job job_queue::pop()
	{
		job_queue::job entry;
		entry.data = nullptr;
		entry.execute = nullptr;
		entry.valid = 0;



		int32_t next_index = next_entry;
		if (next_index < total_jobs)
		{
			if (OSAtomicCompareAndSwap32Barrier(next_entry, next_index + 1, &next_entry))
			{
//				assert(next_index < 127);
				job_queue::job& item = queue[next_index];
				entry.data = item.data;
				entry.execute = item.execute;
				entry.valid = 1;
				complete_past_reads_before_future_reads();
			}
		}
		return entry;
	}

	void job_queue::wait_until_complete()
	{
		while(total_jobs != jobs_completed);
	}

	void job_queue::complete_job()
	{
		OSAtomicIncrement32(&jobs_completed);
	}

} // namespace gemini


void print_string(const char* data)
{
	fprintf(stdout, "thread: %zu, string: %s\n", (size_t)platform::thread_id(), data);
	platform::thread_sleep(250);
}


// ---------------------------------------------------------------------
// configloader
// ---------------------------------------------------------------------

int main(int, char**)
{
	gemini::core_startup();
	gemini::runtime_startup("arcfusion.net/gemini/test_runtime");

	using namespace gemini;

#if 0
	platform::thread_sleep(1000);

	sem_init(&test, 0, 10);

	const size_t MAX_THREADS = 7;
	platform::Thread threads[MAX_THREADS];

	for (size_t index = 0; index < MAX_THREADS; ++index)
	{
		platform::thread_create(threads[index], thread_proc, 0);
	}

	platform::thread_sleep(1000);

	for (size_t index = 0; index < 10; ++index)
	{
		queue_string("ALPHA: 0");
		queue_string("ALPHA: 1");
		queue_string("ALPHA: 2");
		queue_string("ALPHA: 3");
		queue_string("ALPHA: 4");
		queue_string("ALPHA: 5");
		queue_string("ALPHA: 6");
		queue_string("ALPHA: 7");
		queue_string("ALPHA: 8");
		queue_string("ALPHA: 9");

//		platform::thread_sleep(1000);

//	AssetHandle handle = model_by_path("models/test");
//	fprintf(stdout, "is handle valid?: %s\n", lib.validate(handle) ? "Yes" : "No");

		queue_string("BETA: 0");
		queue_string("BETA: 1");
		queue_string("BETA: 2");
		queue_string("BETA: 3");
		queue_string("BETA: 4");
		queue_string("BETA: 5");
		queue_string("BETA: 6");
		queue_string("BETA: 7");
		queue_string("BETA: 8");
		queue_string("BETA: 9");

	}

	// should wait until the queue has finished.
	while(current_work != completed_work);

	for (size_t index = 0; index < MAX_THREADS; ++index)
	{
		platform::thread_detach(threads[index]);
	}



	sem_destroy(&test);
#else

	job_queue jq;
	jq.create_workers(3);

	for (size_t index = 0; index < 4; ++index)
	{

		jq.push_back(print_string, "ALPHA: 0");
		jq.push_back(print_string, "ALPHA: 1");
		jq.push_back(print_string, "ALPHA: 2");
		jq.push_back(print_string, "ALPHA: 3");
		jq.push_back(print_string, "ALPHA: 4");
		jq.push_back(print_string, "ALPHA: 5");
		jq.push_back(print_string, "ALPHA: 6");
		jq.push_back(print_string, "ALPHA: 7");
		jq.push_back(print_string, "ALPHA: 8");
		jq.push_back(print_string, "ALPHA: 9");


	//	platform::thread_sleep(1000);


		jq.push_back(print_string, "BETA: 0");
		jq.push_back(print_string, "BETA: 1");
		jq.push_back(print_string, "BETA: 2");
		jq.push_back(print_string, "BETA: 3");
		jq.push_back(print_string, "BETA: 4");
		jq.push_back(print_string, "BETA: 5");
		jq.push_back(print_string, "BETA: 6");
		jq.push_back(print_string, "BETA: 7");
		jq.push_back(print_string, "BETA: 8");
		jq.push_back(print_string, "BETA: 9");

		jq.push_back(print_string, "DELTA: 0");
		jq.push_back(print_string, "DELTA: 1");
		jq.push_back(print_string, "DELTA: 2");
		jq.push_back(print_string, "DELTA: 3");
		jq.push_back(print_string, "DELTA: 4");
		jq.push_back(print_string, "DELTA: 5");
		jq.push_back(print_string, "DELTA: 6");
		jq.push_back(print_string, "DELTA: 7");
		jq.push_back(print_string, "DELTA: 8");
		jq.push_back(print_string, "DELTA: 9");

		jq.push_back(print_string, "GAMMA: 0");
		jq.push_back(print_string, "GAMMA: 1");
		jq.push_back(print_string, "GAMMA: 2");
		jq.push_back(print_string, "GAMMA: 3");
		jq.push_back(print_string, "GAMMA: 4");
		jq.push_back(print_string, "GAMMA: 5");
		jq.push_back(print_string, "GAMMA: 6");
		jq.push_back(print_string, "GAMMA: 7");
		jq.push_back(print_string, "GAMMA: 8");
		jq.push_back(print_string, "GAMMA: 9");
	}



	// wait until all work completes
	jq.wait_until_complete();

	fprintf(stdout, "destroying workers...\n");
	jq.destroy_workers();
#endif
//	unittest::UnitTest::execute();
	gemini::runtime_shutdown();
	gemini::core_shutdown();
	return 0;
}
