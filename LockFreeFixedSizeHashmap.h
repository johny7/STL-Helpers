#pragma once

#include <bit>
#include <array>
#include <atomic>
#include <cassert>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <immintrin.h>

/*
* Hash map, tailored to be used over shared memory. Properties are:
*  - Requires trivial Key/Value types (i.e. PODs, containing no allocations or any other pointers into memory)
*  - Fixed size
*  - Single writer, multiple readers
*  - Lock free
*  - All operations are amortized O(1), however in practice performance will start dropping once container is nearly full
*  - Throws on overfill
*  - Supports store (writer), remove (writer), read (reader/writer), visit all nodes (reader/writer)
*/

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
		static constexpr size_t NodesMax = NodesMax;

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
			assert((free_bitmask[bitmask_idx] & mask) != 0);

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

	void store(const K& key, auto&& value) requires(std::is_same_v<std::decay_t<decltype(value)>, V>)
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
				node.part_of_bucket = bucket_idx;
				//	mark version as even (readers good to go (but may need to reread))
				node.version.fetch_add(1, std::memory_order_acq_rel);

				return;
			}

			node_idx = node.next_node;
		}

		//	Alloc new node, by doing linked list (new node enters first),
		//	current root node is pushed down and becomes next node.
		//	This way, readers can navigate existing chain down safely - new node will just stay invisible
		node_idx = node_allocator.alloc();
		Node& node = nodes[node_idx];
		assert(node.part_of_bucket == EmptyBucketTag);
		assert(node.next_node == EmptyBucketTag);
		
		node.placement_new();

		node.version.fetch_add(1, std::memory_order_acq_rel);
		node.key = key;
		node.value = std::forward<V>(value);
		node.next_node = buckets[bucket_idx].load(std::memory_order_relaxed);
		node.part_of_bucket = bucket_idx;
		node.version.fetch_add(1, std::memory_order_acq_rel);

		//	At this point we got new node that correctly looks at our root node as next. So readers are oblivious to the addition and
		//	can navigate existing chain. No existing nodes are updated.
		//  However now we replace the root of the bucket to make it public.
		buckets[bucket_idx].store(node_idx, std::memory_order_release);
	}

	template<typename CompatibleK>
	std::optional<V> read(const CompatibleK& key)
	{
		std::optional<V> result;

		size_t bucket_idx = std::hash<CompatibleK>()(key) % BucketsNum;
		auto do_pause = pause_closure();

		//	Do full scan of the bucket.
		// 
		//	Internally we scan the existing chain, making sure we didn't derail on the way (i.e. checking deleted/reused nodes).
		//  It is safe to run over deleted nodes because physically these are not deallocated and stay as a part of `nodes` container, keeping their version value.
		// 
		//  If we found the node (key matches, even version, not derailed) - we grab the data and prep to exit.
		//  Before exit we extra check the node version didn't jump. If so, we must read stale data with possible key/value being overwritten - have to abandon and retry.
		// 
		//	However if we didn't find the node - we have to do extra check that our root node was intact.
		//  Deleter updates bucket's root node if anything inside the chain was modified, to notify readers of the change. Readers cannot detect chain alteration otherwise.
		//  We cannot recover and have to start scanning the chain from scratch.
		//
		while(true)
		{
		l_restart_from_root:
			//	remember characteristics of the root node, those have to stay the same once we done reading
			const size_t root_node_idx = buckets[bucket_idx].load(std::memory_order_acquire);
			if (root_node_idx == EmptyBucketTag)
			{
				//	Early exit - no root - nothing to worry about
				result = std::nullopt;
				return result;
			}

			size_t node_idx = root_node_idx;
			while (node_idx != EmptyBucketTag)
			{
				//	This part handles only node overwrites, so as far as we found the correct node - we just read and pray it was not overwritten.
				Node& node = nodes[node_idx];
				size_t before_version = node.version.load(std::memory_order_acquire);
				if (before_version % 2 == 1)
				{
					//	node is being altered, retrying
					do_pause();
					continue;
				}

				if (node.part_of_bucket != bucket_idx)
				{
					//	node was deleted and reused, we got derailed - has to start from the root
					do_pause();
					goto l_restart_from_root;
				}

				//	Another scenario, node was deleted and readded back into the same bucket. We won't see the change
				//  (version update could have been completed). Though we will be reading node that stays now earlier in the chain.
				//	This would lead to chain rescan - something that we want anyways. We will miss the most latest added nodes, 
				//  as those will stay ahead of the node we are rereading. Which is expected.

				if (node.key != key)
				{
					const size_t next_node_idx = node.next_node;

					//	consuming data from this node is done, we made a decision
					//	however we've been assuming so far it was intact
					//	check if it was true
					size_t after_version = node.version.load(std::memory_order_acquire);
					if (before_version == after_version)
					{
						node_idx = next_node_idx;
						continue;
					}
					
					//	if node was updated (say, overwritten) - we need to reread it again
					do_pause();
					continue;
				}

				//	we reach here if node was found, loading data
				result = node.value;

				//	now, same check were we reading over the same version of the node?
				size_t after_version = node.version.load(std::memory_order_acquire);
				if (before_version == after_version)
				{
					//	found node and managed to read value fully
					//	note, we don't mind if anything around us being erased - we're in the correct unaltered node - that's all that matters
					return result;
				}

				//	nope, version changed - rereading the node, thus we keep node_idx the same
				do_pause();
				continue;
			}

			//	now we fair and square - scanned all, bucket was intact: no such key
			result = std::nullopt;
			return result;
		}
	}

	template<typename CompatibleK>
	bool remove(const CompatibleK& key)
	{
		const size_t bucket_idx = std::hash<CompatibleK>()(key) % BucketsNum;
		const size_t root_node_idx = buckets[bucket_idx].load(std::memory_order_relaxed);
		size_t previous_node_idx = EmptyBucketTag;
		size_t node_idx = root_node_idx;
		while (node_idx != EmptyBucketTag)
		{
			Node& root_node = nodes[root_node_idx];
			Node& node = nodes[node_idx];
			if (node.key == key)
			{
				//	Relink parent node, now it points to the node after. For reader, the chain is in correct state, and current node looks already deleted.
				//	If reader didn't yet manage to read old `next_node` link, futher changes will be transparent.
				//	Note, that readed will have to reread the parent node if it read it during it's update and later detected version change.
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
					//	Here we know that being deleted node is the root node.
					assert(root_node_idx == node_idx);
					//	Update bucket
					buckets[bucket_idx].store(next_node_idx, std::memory_order_release);
				}

				//	Now, mark node as destroyed
				//	In case if client is reading this node, it might lose the ability to continue, because the next_node info was lost (or even reused).
				//	In such case, client should detect it derailed seeing `part_of_bucket` mismatch.
				//	In the edge case when `part_of_bucket` coincides after deletion and reusing - this is the scenario when reader looking at the node that was moved
				//	back to the beginning of the chain - safe to keep using it.
				node.version.fetch_add(1, std::memory_order_acq_rel);
				node.~Node();
				node.part_of_bucket = EmptyBucketTag;
				node.next_node = EmptyBucketTag;
				node.version.fetch_add(1, std::memory_order_acq_rel);
				node_allocator.free(node_idx);

				return true;
			}

			previous_node_idx = node_idx;
			node_idx = node.next_node;
		}

		return false;
	}

	template<typename F>	//	func(const std::pair<key, value>&)
	void visit(F func)
	{
		//	Visit goes across all nodes only once, this might miss some of the newly inserted nodes.
		//  Duplicates are possible if node was visited, deleted and then reinserted.
		for (size_t node_idx = 0; node_idx < MaxElems; ++node_idx)
		{
			//	usual pattern, looking after odd version, version change and part_of_bucket value
			auto do_pause = pause_closure();

			while(true)
			{
				Node& node = nodes[node_idx];
				size_t before_version = node.version.load(std::memory_order_acquire);
				if (before_version % 2 == 1)
				{
					do_pause();
					continue;
				}

				if (node.part_of_bucket == EmptyBucketTag)
				{
					//	empty node, skip..
					break;
				}

				std::pair<K, V> pair = std::make_pair(node.key, node.value);

				size_t after_version = node.version.load(std::memory_order_acquire);
				if (before_version != after_version)
					continue;

				func(pair);

				//	done reading this node
				break;
			}
		}
	}
	
private:
	struct Node
	{
		//	constantly increasing version
		//	odd - being changed
		//  even - ready to read
		std::atomic<size_t> version = 0;
		std::atomic<size_t> next_node = EmptyBucketTag;
		size_t part_of_bucket = EmptyBucketTag;	//	which bucket it belongs to, or -1 if deleted
		K key;
		V value;

		//	placement new of parts of the struct only, we cannot ask version constructor to run - it'd reset the version to zero
		//	instead, we will initialize only user data
		void placement_new()
		{
			new (&key) K;
			new (&value) V;
		}
	};

	auto pause_closure() {
		return [wait_duration = 10]() mutable {
			//	pause a bit
			for (int i = 0; i < wait_duration; ++i)
				_mm_pause();

			wait_duration += 10;
			};
	}

	alignas(64) std::array<std::atomic<size_t>, BucketsNum> buckets;	//	slot marked as EmptyBucketTag - empty
	alignas(64) std::array<Node, MaxElems> nodes;
	alignas(64) details::FixedAllocator<MaxElems> node_allocator;
};

