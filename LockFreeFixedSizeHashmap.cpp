#include "LockFreeFixedSizeHashmap.h"
#include <vector>
#include <random>
#include <thread>
#include <algorithm>

// 1. Set up the random number generator and distribution
static std::random_device rd;
static std::mt19937 gen(rd()); // Mersenne Twister engine
static std::uniform_int_distribution<> dis(1, 1000); // Numbers between 1 and 1000

std::vector<int> random_keys(int n = 100)
{
	std::vector<int> random_numbers;

	// 2. Generate a sequence of random numbers and store them in a container
	//    This isn't using views for generation directly, but for processing later.
	for (int i = 0; i < n; ++i) {
		random_numbers.push_back(dis(gen));
	}

	return random_numbers;
}

void assert_eq(int a, int b)
{
	if(a != b)
		throw std::runtime_error(std::format("lock_free_hash_map_tests: Expected numbers to be equal {} == {}", a, b));
}

//	-----------------------------------

//	test0 - basic functionality
//		thr1 - writes, and overwrites
//		thr2 - reads by key, should be able to read up all
void lfhm0()
{
	LockFreeFixedSizeHashMap <int, int, 100> hmap;
	auto keys = random_keys();

	std::jthread thr1{ [&, keys=keys] mutable {
		for (int key : keys)
			hmap.store(key, key * key);
	}};
	std::jthread thr2{ [&, keys = keys] mutable {
		std::ranges::shuffle(keys, gen);
		for (int key : keys)
		{
			std::optional<int> val;
			while (!val)
				val = hmap.read(key);
			assert_eq(key*key, *val);
		}
	} };

}

//	test1
//		thr1 - write/delete/write/delete same key
//		thr2 - successfully able to read the key most of the time
void lfhm1()
{

}

//	test2, a few buckets (bad hashing key function)
//		write one key
//		thr1 - writes and deletes lots of random stuff (but not the key)
//		thr2 - is able to clearly read the key
void lfhm2()
{

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
	lfhm0();
	lfhm1();
	lfhm2();
	lfhm3();
	lfhm4();
}