#include "LockFreeFixedSizeHashmap.h"
#include <vector>
#include <set>
#include <random>
#include <thread>
#include <algorithm>

// 1. Set up the random number generator and distribution
static std::mt19937 gen(std::random_device{}()); // Mersenne Twister engine
static std::uniform_int_distribution<> dis(1, 1000); // Numbers between 1 and 1000

#define SYNC_START_THREADS()  {--start_counter; while (start_counter != 0) _mm_pause();}

std::vector<int> random_keys(int n = 100)
{
	std::set<int> random_numbers;
	while (random_numbers.size() < n)
		random_numbers.insert(dis(gen));

	return std::vector<int>(std::from_range, random_numbers);
}

template<size_t ... Idx>
auto spawn_n_of_h(auto lambda, std::index_sequence<Idx...> )
{
	return std::array<std::jthread, sizeof...(Idx)>{ (Idx, std::jthread{ lambda }) ... };
}

template<int N>
auto spawn_n_of(auto lambda)
{
	return spawn_n_of_h(std::move(lambda), std::make_index_sequence<N>());
}


void assert_true(bool res)
{
	if (!res)
	{
		_CrtDbgBreak();
		throw std::runtime_error(std::format("lock_free_hash_map_tests: Expected true"));
	}
}

void assert_false(bool res)
{
	if (res)
	{
		_CrtDbgBreak();
		throw std::runtime_error(std::format("lock_free_hash_map_tests: Expected false"));
	}
}

void assert_eq(int a, int b)
{
	if (a != b)
	{
		_CrtDbgBreak();
		throw std::runtime_error(std::format("lock_free_hash_map_tests: Expected numbers to be equal {} == {}", a, b));
	}
}

void assert_is_among(int a, std::initializer_list<int> b)
{
	if (std::ranges::find(b, a) == b.end())
	{
		_CrtDbgBreak();
		throw std::runtime_error(std::format("lock_free_hash_map_tests: Expected numbers to be equal {} == {}", a, b));
	}
}

//	-----------------------------------

void test_allocator()
{
	auto alloc_test = [](auto allocator)
		{
			for (int repeat = 0; repeat < 2; ++repeat)
			{
				for (size_t i = 0; i < allocator.NodesMax; ++i)
					allocator.alloc();

				while (true)
				{
					try { allocator.alloc(); }
					catch (const std::exception&) { break; }

					throw std::runtime_error(std::format("Overallocation expected to throw, {}", allocator.NodesMax));
				}

				for (size_t i = 0; i < allocator.NodesMax; ++i)
					allocator.free(i);
			}
		};

	alloc_test(details::FixedAllocator<3>());
	alloc_test(details::FixedAllocator<16>());
	alloc_test(details::FixedAllocator<30>());
	alloc_test(details::FixedAllocator<100>());
	alloc_test(details::FixedAllocator<256>());
	alloc_test(details::FixedAllocator<1111>());
}

//	test - basic functionality
//		thr1 - writes, and overwrites
//		thr2 - reads by key, should be able to read up all
void test_basic_writes()
{
	LockFreeFixedSizeHashMap <int, int, 100> hmap;
	auto keys = random_keys();

	constexpr size_t c_num_of_reading_threads = 5;
	std::atomic<int> start_counter = c_num_of_reading_threads + 1;

	std::jthread thr1{ [&, keys=keys] mutable {
		SYNC_START_THREADS();
		for (int repeat = 0; repeat < 5; ++repeat)
		{
			for (int key : keys)
				hmap.store(key, key * key);

			for (int key : keys)
				hmap.store(key, -1);
		}
	}};

	auto reader_threads = spawn_n_of<c_num_of_reading_threads>([&, keys = keys] mutable {
		SYNC_START_THREADS();
		for(int repeat = 0; repeat < 5; ++repeat)
		{
			std::ranges::shuffle(keys, gen);
			for (int key : keys)
			{
				std::optional<int> val;
				while (!val)
					val = hmap.read(key);

				assert_is_among(*val, { key * key, -1 });
			}
		}
	});
}

//	test - recreating key
//		thr1 - write/delete/write/delete same key
//		thr2 - successfully able to read the key most of the time
void test_add_del()
{
	LockFreeFixedSizeHashMap <int, int, 100> hmap;
	std::atomic<int> start_counter = 2;

	std::jthread thr1{ [&] mutable {
		SYNC_START_THREADS();
		for (int repeat = 0; repeat < 1000; ++repeat)
		{
			hmap.store(1, repeat+1);
			if(dis(gen) % 3 == 0)
				std::this_thread::yield();	//	should givabreak periodically
			hmap.remove(1);
		}
	}};

	std::jthread thr2{ [&] mutable {
		SYNC_START_THREADS();
		int last_collected = 0;
		for (int repeat = 0; repeat < 5000; ++repeat)
		{
			std::optional<int> value = hmap.read(1);
			if (value)
			{
				assert_true(last_collected <= *value);	//	i.e. in ascending order
				last_collected = *value;
			}
		}
	}};
}

//	test - quickly updating the same bucket - reader should be able to read unaffected node
void test_reinsert_same_bucket()
{
	//	specially selected keys so they differ but fall into the same bucket
	constexpr int key_reinsert = 8;
	constexpr int key_query = 9;

	LockFreeFixedSizeHashMap <int, int, 2> hmap;
	hmap.store(key_query, 42);
	std::atomic<int> start_counter = 2;

	std::jthread thr1{ [&] mutable {
		SYNC_START_THREADS();
		for (int repeat = 0; repeat < 1000; ++repeat)
		{
			hmap.store(key_reinsert, repeat);
			_mm_pause();
			hmap.remove(key_reinsert);
			_mm_pause();
		}
	}};

	std::jthread thr2{ [&] mutable {
		SYNC_START_THREADS();
		for (int repeat = 0; repeat < 2000; ++repeat)
		{
			std::optional<int> value = hmap.read(key_query);
			assert_true(value.has_value());
			assert_eq(*value, 42);
		}
	}};
}

//	test - writes and deletes all sort of keys but must not affect only one key
//		thr1 - writes and deletes lots of random stuff (but not the key)
//		thr2 - is able to clearly read the key
void test_other_key_writer_does_not_affect_reader()
{
	int key_query = -1;	//	guaranteed to be off the random generator range
	constexpr size_t c_num_of_reading_threads = 5;
	std::atomic<int> start_counter = c_num_of_reading_threads + 1;

	LockFreeFixedSizeHashMap <int, int, 15> hmap;
	hmap.store(key_query, 42);

	std::jthread thr1{ [&] mutable {
		SYNC_START_THREADS();
		std::vector<int> inserted;
		for (int repeat = 0; repeat < 1000; ++repeat)
		{
			if (inserted.size() == 10)
			{
				hmap.remove(inserted[0]);
				inserted.erase(inserted.begin());
			}

			auto num = dis(gen);
			hmap.store(num, 77);
			inserted.push_back(num);
		}
	}};

	auto reader_threads = spawn_n_of<c_num_of_reading_threads>([&] mutable {
		SYNC_START_THREADS();
		for (int repeat = 0; repeat < 2000; ++repeat)
		{
			std::optional<int> value = hmap.read(key_query);
			assert_true(value.has_value());
			assert_eq(*value, 42);
		}
	});
}


//	test - writes and deletes all sort of keys but older keys must still be readable
//		thr1 - writes and deletes lots of random stuff, keeping the list of inserted
//		thr2 - checks the second half of the keys if still readable
void test_other_key_writer_does_not_affect_reader2()
{
	//	large enough, so writer thread does not delete all the keys in the queue, while reader tries to check one key
	constexpr size_t c_elements_num = 1000;
	constexpr size_t c_num_of_reading_threads = 5;
	std::atomic<int> start_counter = c_num_of_reading_threads + 1;

	LockFreeFixedSizeHashMap <int, int, c_elements_num> hmap;
	std::vector<int> inserted;

	std::jthread thr1{ [&] mutable {
		//	prefill
		inserted = random_keys(c_elements_num);
		for(auto num : inserted)
			hmap.store(num, num * num);

		SYNC_START_THREADS();
		for (int repeat = 0; repeat < 10000; ++repeat)
		{
			bool removed = hmap.remove(inserted[0]);
			assert_true(removed);
			inserted.erase(inserted.begin());

			int num;
			while(true)
			{
				//	prevent duplicates, so remove always succeed
				num = dis(gen);
				if (!std::ranges::contains(inserted, num))
					break;
			}

			hmap.store(num, num * num);
			inserted.push_back(num);
		}
	} };

	auto reader_threads = spawn_n_of<c_num_of_reading_threads>([&] mutable {
		SYNC_START_THREADS();
		for (int repeat = 0; repeat < 20000; ++repeat)
		{
			std::uniform_int_distribution<> dis(c_elements_num / 2, c_elements_num - 2);
			int key = inserted[dis(gen)];

			std::optional<int> value = hmap.read(key);
			assert_true(value.has_value());
			assert_eq(*value, key* key);
		}
	} );
}

//	test - reading non-existing key should always complete
//		thr1 - writes and deletes lot of random stuff
//		thr2 - reads non-existing key - always empty
void test_non_existing_key()
{
	//	large enough, so writer thread does not delete all the keys in the queue, while reader tries to check one key
	constexpr size_t c_elements_num = 1000;
	constexpr size_t c_num_of_reading_threads = 5;
	std::atomic<int> start_counter = c_num_of_reading_threads + 1;

	LockFreeFixedSizeHashMap <int, int, c_elements_num> hmap;

	std::jthread thr1{ [&] mutable {
		SYNC_START_THREADS();
		std::vector<int> inserted;
		for (int repeat = 0; repeat < 10000; ++repeat)
		{
			if(inserted.size() == c_elements_num)
			{
				hmap.remove(inserted[0]);
				inserted.erase(inserted.begin());
			}

			int num = dis(gen);	//	duplicates are fine
			hmap.store(num, num * num);
			inserted.push_back(num);
		}
	} };

	auto reader_threads = spawn_n_of<c_num_of_reading_threads>( [&] mutable {
		SYNC_START_THREADS();
		for (int repeat = 0; repeat < 20000; ++repeat)
		{
			//	negative is guaranteed missing key
			int key = -dis(gen);

			std::optional<int> value = hmap.read(key);
			assert_false(value.has_value());
		}
	} );
}


//	test4
//		thr1 - writes set of keys, waits a bit and then shuffles and deletes all the keys
//		thr2 - keeps iterating and counts number of keys encountered, should drop from N-ish to 0
void lfhm4()
{

}

void lock_free_hash_map_tests()
{
	test_allocator();
	test_basic_writes();
	test_add_del();
	test_reinsert_same_bucket();
	test_other_key_writer_does_not_affect_reader();
	test_other_key_writer_does_not_affect_reader2();
	test_non_existing_key();
	lfhm4();
}