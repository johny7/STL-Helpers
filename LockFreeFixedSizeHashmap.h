#pragma once

#include <bit>
#include <array>
#include <atomic>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <immintrin.h>

/*
* Hash map, tailored to be used over shared memory. It has:
*  - Fixed size
*  - Lock free
*  - Single writer, multiple readers
*  - Requires trivial Key/Value types (i.e. PODs, containing no allocations or any other pointers into memory)
*  - All operations are amortized O(1), however in practice performance will start dropping once container is nearly full
*  - Throws on overfill
*  - Supports store (writer), remove (writer), read (reader/writer)
*/

//	adapt your logging
#define LOGGING(...) std::cout << std::format(__VA_ARGS__) << std::endl

namespace details {
	//	Memory-less fixed size allocator of size NodesMax
	//	each chunk is of ChunkSize. Returns available indexes (i.e. does not own or manage the memory itself).
	template<size_t NodesMax>
	class FixedAllocator
	{
		using BitmaskType = uint64_t;
		static constexpr size_t BitmaskBits = sizeof(BitmaskType) * 8;
		static constexpr size_t BitMaskLen = (NodesMax + BitmaskBits - 1) / BitmaskBits;

	public:
		FixedAllocator()
		{
			//	last byte of free_bitmask should have 1111...0000000 filled to mark unavailable space
			auto bits_overflow = BitMaskLen * BitmaskBits - NodesMax;
			BitmaskType val = 0;
			for (auto i = 0; i < bits_overflow; ++i)
				val = (val >> 1) | (BitmaskType(1) << (BitmaskBits - 1));
			free_bitmask[BitMaskLen - 1] = val;
		}

		size_t alloc()
		{
			//	search free in blocks of 64, scanning linearly
			auto old_last_allocated_free_bitmask_idx = last_allocated_free_bitmask_idx;
			while (free_bitmask[last_allocated_free_bitmask_idx] == std::numeric_limits<BitmaskType>::max())
			{
				++last_allocated_free_bitmask_idx;
				last_allocated_free_bitmask_idx %= BitMaskLen;

				if (last_allocated_free_bitmask_idx == old_last_allocated_free_bitmask_idx)
					throw std::runtime_error("Hash map overflow");
			}

			//	searching for the first zero bit
			unsigned zero_at_bit = std::countr_one(free_bitmask[last_allocated_free_bitmask_idx]);

			//	mark as allocated
			BitmaskType bitmask = BitmaskType(1) << zero_at_bit;
			free_bitmask[last_allocated_free_bitmask_idx] |= bitmask;

			size_t idx = last_allocated_free_bitmask_idx * BitmaskBits + zero_at_bit;

			return idx;
		}

		void free(size_t idx)
		{
			assert(idx < NodesMax);
			size_t bitmask_idx = idx / BitmaskBits;
			size_t bitmask_bit = idx % BitmaskBits;
			BitmaskType mask = BitmaskType(1) << bitmask_bit;

			free_bitmask[bitmask_idx] &= ~mask;
		}

	private:
		//	Bitmask of free chunks, for quick alloc/dealloc.
		//  Lowest bit stores availability of nodes[0], bit 63 encodes availability of -> nodes[63]
		//  further node's chunks are encoded in the next cells, example free_bitmask[1] & 0x0010 encodes availability of nodes[65]
		//	0 - free, 1 - taken
		uint64_t free_bitmask[BitMaskLen] = {};
		//	This means where we allocated something, zero - effectively this is where we will be looking for the next chunk again
		size_t last_allocated_free_bitmask_idx = 0;
	};

	//	round up to the next prime number
	consteval size_t next_prime(size_t num)
	{
		if (num <= 5)
			return 5;

		auto sqrt_top_boundary = num >> (std::bit_width(num) / 2 - 1);
		while (true)
		{
			bool not_prime = false;
			for (size_t i = 2; i <= sqrt_top_boundary; ++i)
			{
				if (num % i == 0)
				{
					not_prime = true;
					break;
				}
			}

			if (!not_prime)
				return num;

			++num;
		}
	}
}


template<typename K, typename V, size_t MaxElems>
	requires std::is_trivially_copyable_v<K> && std::is_trivially_copyable_v<V>
class LockFreeFixedSizeHashMap
{
	static constexpr size_t EmptyBucketTag = std::numeric_limits<size_t>::max();
	static constexpr size_t BucketsNum = details::next_prime(MaxElems * 2);

public:
	LockFreeFixedSizeHashMap()
	{
		std::fill(buckets.begin(), buckets.end(), EmptyBucketTag);
	}

	void store(const K& key, V&& value)
	{
		size_t bucket_idx = std::hash<K>()(key) % BucketsNum;

		//	navigate existing bucket's nodes for the key match
		size_t node_idx = buckets[bucket_idx].load(std::memory_order_relaxed);
		while (node_idx != EmptyBucketTag)
		{
			Node& node = nodes[node_idx];
			if (node.key == key)
			{
				//	version is odd (readers stay away)
				node.version.fetch_add(1, std::memory_order_acq_rel);
				//	update value
				node.value = std::forward<V>(value);
				node.has_value = true;
				//	version is even (readers good to go (but may need to reread))
				node.version.fetch_add(1, std::memory_order_acq_rel);

				return;
			}

			node_idx = node.next_node;
		}

		//	alloc new node, by doing linked list (new node enters first)
		node_idx = node_allocator.alloc();
		Node& node = nodes[node_idx];
		new (&node) Node();

		node.version.fetch_add(1, std::memory_order_acq_rel);
		node.key = key;
		node.value = std::forward<V>(value);
		node.next_node = buckets[bucket_idx].load(std::memory_order_relaxed);
		node.has_value = true;
		node.version.fetch_add(1, std::memory_order_acq_rel);
		buckets[bucket_idx].store(node_idx, std::memory_order_release);
	}

	template<typename CompatibleK>
	std::optional<V> read(const CompatibleK& key)
	{
		std::optional<V> result;

		size_t bucket_idx = std::hash<CompatibleK>()(key) % BucketsNum;

		//	Do full scan of the bucket.
		//	If, during traversing, version does not match even once, we have to reread the whole bucket, otherwise we might lose ourselves
		//	in the broken chain due to deleted nodes.
		for (int tries = 0; tries < 50; ++tries)
		{
			size_t node_idx = buckets[bucket_idx].load(std::memory_order_acquire);
			while (node_idx != EmptyBucketTag)
			{
				Node& node = nodes[node_idx];
				size_t before_version = node.version.load(std::memory_order_acquire);
				if(before_version % 2 == 1)
					goto l_next_try;	//	node is being altered, retrying

				if (node.key != key)
				{
					node_idx = node.next_node;

					size_t after_version = node.version.load(std::memory_order_acquire);
					if (before_version != after_version)
						goto l_next_try;	//	can't trust information in this node anymore, retrying

					continue;
				}

				//	found the node

				if (!node.has_value)
					return result;	//	funky situation, we found node but it was already deleted

				result = node.value;
				size_t after_version = node.version.load(std::memory_order_acquire);
				if (before_version != after_version)
				{
					result = {};

					goto l_next_try;	//	can't trust information in this node anymore, retrying
				}

				//	found node and managed to read value fully
				return result;
			}

			if(node_idx != buckets[bucket_idx].load(std::memory_order_acquire))
				goto l_next_try;	//	root bucket node has changed, best to reread new chain

			//	no such key
			return result;

		l_next_try:
			//	pause a bit
			for(int i = -10; i < tries*10; ++i)
				_mm_pause();
		}

		LOGGING("Exhausted all tries reading the key");
		return result;
	}

	template<typename CompatibleK>
	bool remove(const CompatibleK& key)
	{
		size_t bucket_idx = std::hash<CompatibleK>()(key) % BucketsNum;
		size_t previous_node_idx = EmptyBucketTag;
		size_t node_idx = buckets[bucket_idx].load(std::memory_order_relaxed);
		while (node_idx != EmptyBucketTag)
		{
			Node& node = nodes[node_idx];
			if (node.key == key)
			{
				//	first, relink parent node
				size_t next_node_idx = node.next_node;
				if (previous_node_idx != EmptyBucketTag)
				{
					Node& previous_node = nodes[previous_node_idx];
					previous_node.version.fetch_add(1, std::memory_order_acq_rel);
					previous_node.next_node = next_node_idx;
					previous_node.version.fetch_add(1, std::memory_order_acq_rel);
				}
				else
				{
					buckets[bucket_idx].store(next_node_idx, std::memory_order_release);
				}

				//	now this node is lose
				node.version.fetch_add(1, std::memory_order_acq_rel);
				node.has_value = false;
				node.version.fetch_add(1, std::memory_order_acq_rel);
				node_allocator.free(node_idx);

				return true;
			}

			previous_node_idx = node_idx;
			node_idx = node.next_node;
		}

		return false;
	}
	
private:
	struct Node
	{
		//	constantly increasing version
		//	odd - being changed
		//  even - ready to read
		std::atomic<size_t> version = 0;
		std::atomic<size_t> next_node = EmptyBucketTag;
		bool has_value = false;
		K key;
		V value;
	};

	//static constexpr size_t ChunkSizePow2 = std::bit_ceil(ChunkSize);

	alignas(64) std::array<std::atomic<size_t>, BucketsNum> buckets;	//	slot marked as EmptyBucketTag - empty
	alignas(64) std::array<Node, MaxElems> nodes;
	alignas(64) details::FixedAllocator<MaxElems> node_allocator;
};


