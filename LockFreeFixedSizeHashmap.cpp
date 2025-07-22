#include "LockFreeFixedSizeHashmap.h"
#include <vector>
#include <set>
#include <random>
#include <thread>
#include <algorithm>

// 1. Set up the random number generator and distribution
static std::random_device rd;
static std::mt19937 gen(rd()); // Mersenne Twister engine
static std::uniform_int_distribution<> dis(1, 1000); // Numbers between 1 and 1000

#define SYNC_START_THREADS(COUNT)  {++start_counter; while (start_counter != COUNT) _mm_pause();}

std::vector<int> random_keys(int n = 100)
{
	std::set<int> random_numbers;
	while (random_numbers.size() < n)
		random_numbers.insert(dis(gen));

	return std::vector<int>(std::from_range, random_numbers);
}

void assert_true(bool res)
{
	if (!res)
	{
		_CrtDbgBreak();
		throw std::runtime_error(std::format("lock_free_hash_map_tests: Expected true"));
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
	std::atomic<int> start_counter{};

	std::jthread thr1{ [&, keys=keys] mutable {
		SYNC_START_THREADS(2);
		for (int repeat = 0; repeat < 5; ++repeat)
		{
			for (int key : keys)
				hmap.store(key, key * key);

			for (int key : keys)
				hmap.store(key, -1);
		}
	}};
	std::jthread thr2{ [&, keys = keys] mutable {
		SYNC_START_THREADS(2);
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
	}};
}

//	test - recreating key
//		thr1 - write/delete/write/delete same key
//		thr2 - successfully able to read the key most of the time
void test_add_del()
{
	LockFreeFixedSizeHashMap <int, int, 100> hmap;
	std::atomic<int> start_counter{};

	std::jthread thr1{ [&] mutable {
		SYNC_START_THREADS(2);
		for (int repeat = 0; repeat < 1000; ++repeat)
		{
			hmap.store(1, repeat+1);
			if(dis(gen) % 3 == 0)
				std::this_thread::yield();	//	should givabreak periodically
			hmap.remove(1);
		}
	}};
	std::jthread thr2{ [&] mutable {
		SYNC_START_THREADS(2);
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

void test_reinsert_same_bucket()
{
	//	specially selected keys so they differ but fall into the same bucket
	constexpr int key_reinsert = 8;
	constexpr int key_query = 9;

	LockFreeFixedSizeHashMap <int, int, 2> hmap;
	hmap.store(key_query, 42);
	std::atomic<int> start_counter{};

	std::jthread thr1{ [&] mutable {
		SYNC_START_THREADS(2);
		for (int repeat = 0; repeat < 1000; ++repeat)
		{
			hmap.store(key_reinsert, repeat);
			if (dis(gen) % 3 == 0)
				std::this_thread::yield();	//	should givabreak periodically
			hmap.remove(key_reinsert);
			std::this_thread::yield();
		}
	}};

	std::jthread thr2{ [&] mutable {
		SYNC_START_THREADS(2);
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

	LockFreeFixedSizeHashMap <int, int, 15> hmap;
	hmap.store(key_query, 42);
	std::atomic<int> start_counter{};

	std::jthread thr1{ [&] mutable {
		SYNC_START_THREADS(2);
		std::vector<int> inserted;
		inserted.reserve(10);
		for (int repeat = 0; repeat < 1000; ++repeat)
		{
			if (inserted.size() < 10)
			{
			}
			else if (inserted.size() < 15)
			{
				hmap.remove(inserted[inserted.size() - 10]);
			}
			else
			{
				hmap.remove(inserted[5]);
				inserted.erase(inserted.begin());
			}

			auto num = dis(gen);
			hmap.store(num, 77);
			inserted.push_back(num);
		}
	}};

	std::jthread thr2{ [&] mutable {
		SYNC_START_THREADS(2);
		for (int repeat = 0; repeat < 2000; ++repeat)
		{
			std::optional<int> value = hmap.read(key_query);
			assert_true(value.has_value());
			assert_eq(*value, 42);
		}
	}};
}


//	test3
//		thr1 - writes and deletes lot of random stuff
//		thr2 - reads non-existing key - always empty
void lfhm3()
{

}

//	test4
//		thr1 - writes set of keys, waits a bit and then shuffles and deletes all the keys
//		thr2 - keeps re-reading the same set of keys, amount should drop from N to 0
void lfhm4()
{

}

void lock_free_hash_map_tests()
{
	test_other_key_writer_does_not_affect_reader();
	return;

	test_allocator();
	test_basic_writes();
	test_add_del();
	test_reinsert_same_bucket();
	test_other_key_writer_does_not_affect_reader();
	lfhm3();
	lfhm4();
}