#pragma once

#include <algorithm>
#include <limits>
#include <type_traits>
#include <random>
#include <assert.h>
#include "IsInstanceOf.h"

#undef min
#undef max

//
//	Presented algorithms will allow to use functional approach at max:
//
//	Foreach adapter for maps/multimaps, you can iterate over
//	select1st(container)	//.. keys
//	select2nd(container)	//.. values
//	select_member(container)//.. or any member of value
//
//
//	Clamp value to [min, max] range
//	T clamp(T value, U min, V max)
//
//	Clamp value to [0-1]
//	T saturate(T value)
//
//	Linear interpolating between [from-to], scale in [0-1]
//	T lerp(T from, U to, V scale)
//
//	Stretches value from [from-to] range into [0-1],
//	useful when coupled together with 'lerp' as 'scale' parameter
//	T scale(T value, U from, V to)
//
//	Min for variadic list of args
//	T min(T val1, U val2, ...)
//	T min_f(T val1, U val2, ..., Comparator f)
//
//	Max for list of args
//	T max(T val1, U val2, ...)
//	T max_f(T val1, U val2, ..., Comparator f)
//
//	Random element choice using probability table
//	T random_choose(CONT values, CONT probabilities)
//	Same as above but will just return index instead
//	unsigned random_choose(CONT probabilities)
//
//	Round to nearest integer
//	int round(double val)
//
//	Quick power into natural number power
//	T pow(T val, int power)
//	T square(T val)
//
//	Quick check if value is among range of values - useful for enums for example
//	bool among(T val, { values, .. } )
//
//	Removes element from vector-like container, returns number of deleted items. Preserving order.
//	size_t remove(container, value)
//
//	Removes element from container using functor, returns number of deleted items. Preserving order.
//	size_t remove_if(container, F)
//	
//	Removes elements using list of indexes. Preserving order.
//	void remove_indexes( T& vector, const IDXType& to_remove )
//
//	Removes element by index in a container, returns 'true' if was deleted
//	bool erase(container, size_t index)
//
//
//	Returns index of the object in an container, or size() if was not found
//	size_t object_id(container, value)
//
//	Retrieves element from map/unordered_map, `assert` if no such element
//	value& map_get(container, key)
//
//	Retrieves element from map/unordered_map, returns default value if no such element
//	value& map_get_def(container, key, default)
//
//	Retrieves element by index, `assert` if no such element
//	value& get_by_idx(container, idx)
//
//
//	Is there such element in container
//	bool is_exist(container, value)
//
//	Is there an element in container satisfying predicate
//	bool if_exist(container, predicate)
//
//
//	Do 'functor' for each element of container
//	functor for_each(container, functor)
//
//
//	Find element in container, returns end() iterator if wasn't found
//	iterator find(container, key)
//
//	Find element in container, `assert` if not found
//	iterator find_a(container, key)
//
//	Find element in container that satisfies 'predicate', returns end() iterator if wasn't found
//	iterator find_if(container, predicate)
//
//	Find element in container that satisfies 'predicate', `assert` if not found
//	iterator find_if_a(container, predicate)
//
//	bool binary_search(container, value)
//	iterator lower_bound(container, value)
//	iterator upper_bound(container, value)
//	iterator min_element(container, predicate)
//	iterator max_element(container, predicate)
//
//	Does non-sorted container have duplicates? Applicable to non-ordered containers only (like std::vector)
//	has_duplicates(container)
//
//	Remove non-sorted container duplicates. It sorts the container first.
//	remove_duplicates(container)
//
//	Insert element into container, 'true' if was inserted (for usage in templates)
//	bool insert(container, value)
//
//
//	Copy data from one container to another, returns 'true' if anything was copied.
//	Combining with select1st, select2nd allows to copy key or value from key/value containers.
//	Example:  copy(select2nd(some_map), some_array))
//	It inserts values into destination array, so it does not overwrite if such key already existed.
//	bool copy(src_container, dst_container)
//	bool copy(src_container, dst_container, transform_functor)
//	bool copy(src_container, dst_container, src_member_ptr)
//
//	Copy data from one container to another, returns 'true' if anything was copied.
//	'predicate' - a conditional functor that all elements of src_container are checked upon
//	bool copy_if(src_container, dst_container, predicate)
//
//	Move all elements from one container to another, returns 'true' if anything was moved
//	bool move(src_container, dst_container)
//	bool move_if(src_container, dst_container, predicate)
//
//	Sort
//	void sort(container, functor = operator<)
//	void partial_sort(container, num, functor = operator<)
//
//	Randomly shuffle
//	void shuffle(container)
//	
//	Could elements that are equal to 'element'
//	size_t count(container, element)
//
//	Count elements for which 'predicate' returns true
//	size_t count_if(container, predicate)
//
//	Sum or multiply all values in a container
//	T sum(container)
//	T multiply(container)
//
//	Accumulate value using functor
//	T accumulate(container, functor)
//
//	Advances iterator using std::advance, then returns it
//	IT advance(iterator, shift)
//
//	Reverse all elements in the container
//	void reverse(container)
//
//	Generates positive key value in map, that is not exists
//	K  new_map_id(const std::map<K, T>& m)
//	
//
namespace std
{
	//	Predeclaration does not work in Linux, you will have to include all this stuff
	template<typename T, typename A> class vector;
	template<typename T, typename A> class list;
	template<typename K, typename P, typename A> class set;
	template<typename K, typename P, typename A> class multiset;
	template<typename K, typename V, typename P, typename A> class map;
	template<typename K, typename V, typename P, typename A> class multimap;
	template<class _Kty, class _Ty, class _Hasher, class _Keyeq, class _Alloc> class unordered_map;
	template<class _Kty, class _Hasher, class _Keyeq, class _Alloc> class unordered_set;
};


namespace alg
{
	template<typename T, typename U, typename V>
	T clamp(T value, U min, V max)
	{
		return std::min( std::max(value, static_cast<T>(min)), static_cast<T>(max) );
	}

	template<typename T>
	T saturate(T value)
	{
		return std::min( std::max(value, static_cast<T>(0)), static_cast<T>(1) );
	}

	template<typename T, typename U, typename V>
	T lerp(T from, U to, V scale)
	{
		static_assert( std::is_floating_point_v<V>, "Scale should be able to represent [0-1]" );

		return T( from * (1 - scale) + to * scale );
	}

	template<typename T, typename U, typename V>
	T scale(T value, U from, V to)
	{
		assert(to != from);
		return T( (value - from) / (to - from) );
	}

	template<typename T>
	T square(T value)
	{
		return value*value;
	}

	template<typename A1, typename ... ARGS>
	A1 min(A1 arg1, ARGS ... args)
	{
		static_assert( sizeof... (ARGS) >= 1, "alg::min must have 2 args min");
		auto list = { static_cast<A1>(args)... };
		
		return std::min(arg1, *std::min_element(list.begin(), list.end()) );
	}

	template<typename A1, typename ... ARGS>
	A1 max(A1 arg1, ARGS ... args)
	{
		static_assert( sizeof... (ARGS) >= 1, "alg::max must have 2 args min");
		auto list = { static_cast<A1>(args)... };
		
		return std::max(arg1, *std::max_element(list.begin(), list.end()) );
	}

	template<typename ... ARGS, typename COMPARATOR>
	auto min_f(COMPARATOR f, ARGS ... args) -> typename std::tuple_element<0, std::tuple<ARGS...> >::type
	{
		static_assert( sizeof... (ARGS) >= 2, "alg::min_f must have 2 args min");
		typedef typename std::tuple_element<0, std::tuple<ARGS...> >::type A1;

		auto list = { static_cast<A1>(args)... };
		return *std::min_element(list.begin(), list.end(), f);
	}

	template<typename ... ARGS, typename COMPARATOR>
	auto max_f(COMPARATOR f, ARGS ... args) -> typename std::tuple_element<0, std::tuple<ARGS...> >::type
	{
		static_assert( sizeof... (ARGS) >= 2, "alg::max_f must have 2 args min");
		typedef typename std::tuple_element<0, std::tuple<ARGS...> >::type A1;

		auto list = { static_cast<A1>(args)... };
		return *std::max_element(list.begin(), list.end(), f);
	}

	inline int round(double val)
	{
		return (int)floor(val + 0.5);
	}

	template<typename T>
	inline T round(T val, int precisionAfterPoint)
	{
		float mult = ::pow(10, precisionAfterPoint);

		return (T)(floor(val*mult + 0.5) / mult);
	}

	template<typename T>
	bool is_among(T val, std::initializer_list<T> list)
	{
		return std::find(list.begin(), list.end(), val) != list.end();
	}

	template<typename T>
	bool has_duplicates(T& container)
	{
		using EL = std::remove_reference_t<decltype(*container.begin())>;

		std::set<EL> s;
		for (auto& el : container)
		{
			if(is_exist(s, el))
				return true;

			s.insert(el);
		}

		return false;
	}

	//  some compile time pleasure to calculate pow in compile time
	template<unsigned ORDER, typename T>
	consteval T pow(T val)
	{
		T result = val;
		for(unsigned i = 1; i < ORDER; ++i)
			result *= val;
		return result;
	}

	template<typename CONT>
	size_t remove(CONT& cont, const typename CONT::value_type& value)
	{
		typename CONT::iterator trash = std::remove(cont.begin(), cont.end(), value);
		const size_t value_number = cont.end() - trash;

		if(value_number)
			cont.erase( trash, cont.end() );

		return value_number;
	}

	template<typename T, typename A>
	size_t remove( std::list<T, A>& cont, const typename std::list<T, A>::value_type& value )
	{
		size_t ret_val = 0;
		for(auto obj_it = cont.begin(); obj_it != cont.end();  )
		{
			if( *obj_it == value )
			{
				obj_it = cont.erase(obj_it);
				ret_val++;
			}
			else
				++obj_it;
		}

		return ret_val;
	}

	template<typename T>
	size_t remove(std::basic_string<T>&  str, const T* value)
	{
		size_t  amount = 0;

		while(1)
		{
			size_t pos = str.find(value);

			if( pos == std::basic_string<T>::npos )
				break;

			str.erase( pos, std::char_traits<T>::strlen(value) );
			++amount;
		}

		return amount;
	}

	template<typename CONT, typename F>
	size_t remove_if(CONT& cont, F func)
	{
		typename CONT::iterator trash = std::remove_if(cont.begin(), cont.end(), func);
		const size_t value_number = unsigned(cont.end() - trash);

		if(value_number)
			cont.erase( trash, cont.end() );

		return value_number;
	}

	template< typename K, typename V, typename P, typename A, typename PredicateT >
	void remove_if( std::map<K, V, P, A>& items, const PredicateT& predicate ) 
	{
		for( auto it = items.begin(); it != items.end(); )
			if( predicate(*it) ) it = items.erase(it);
				else ++it;
	}

	template< typename K, typename V, typename P, typename A, typename PredicateT >
	void remove_if( std::multimap<K, V, P, A>& items, const PredicateT& predicate ) 
	{
		for( auto it = items.begin(); it != items.end(); )
			if( predicate(*it) ) it = items.erase(it);
				else ++it;
	}

	template<typename T, typename IDXType>
	void remove_indexes( T& vector, const IDXType& to_remove )
	{
		auto vector_base = vector.begin();
		typename T::size_type down_by = 0;

		for( auto iter = to_remove.cbegin();
			iter < to_remove.cend();
			iter++, down_by++ )
		{
			typename T::size_type next = (iter + 1 == to_remove.cend()
				? vector.size()
				: *(iter + 1));

			std::move( vector_base + *iter + 1,
				vector_base + next,
				vector_base + *iter - down_by );
		}

		vector.erase( vector.begin() + (vector.size() - to_remove.size()), vector.end() );
	}


	template<typename CONT>
	bool erase(CONT& cont, size_t index)
	{
		if(index >= cont.size())
			return false;

		typename CONT::iterator  p = cont.begin();
		std::advance(p, index);
		cont.erase(p);

		return true;
	}


	//	returns size() if there is no element
	template<typename T, typename A>
	size_t object_id(const std::vector<T, A>& cont, const T& value)
	{
		return std::find(cont.begin(), cont.end(), value) - cont.begin();
	}

	template<typename CONT>
	const typename CONT::value_type& get_by_idx(const CONT& cont, size_t idx)
	{
		auto it = cont.begin();
		std::advance(it, idx);

		return *it;
	}


	template<typename K, typename V, typename P, typename A>
	const V& map_get(const std::map<K, V, P, A>& cont, const K& key)
	{
		assert( is_exist(cont, key) );
		return cont.find(key)->second;
	}
	template<typename K, typename V, typename P, typename A>
	V& map_get(std::map<K, V, P, A>& cont, const K& key)
	{
		assert( is_exist(cont, key) );
		return cont.find(key)->second;
	}
	template<typename K, typename V, typename Hasher, typename Keyeq, typename Alloc>
	const V& map_get(const std::unordered_map<K, V, Hasher, Keyeq, Alloc>& cont, const K& key)
	{
		assert(is_exist(cont, key));
		return cont.find(key)->second;
	}
	template<typename K, typename V, typename Hasher, typename Keyeq, typename Alloc>
	V& map_get(std::unordered_map<K, V, Hasher, Keyeq, Alloc>& cont, const K& key)
	{
		assert(is_exist(cont, key));
		return cont.find(key)->second;
	}


	template<typename K, typename V, typename P, typename A>
	const V&  map_get_def(const std::map<K, V, P, A>& cont, const K& key, const V& default_value = V())
	{
		auto  p = cont.find(key);
		if(p == cont.end())
			return default_value;
		
		return p->second;
	}
	//	& specialization for 'nullptr'
	template<typename K, typename V, typename P, typename A>
	const V&  map_get_def(const std::map<K, V, P, A>& cont, const K& key, void* ptr)
	{
		return map_get_def(cont, key, (V)ptr);
	}
	template<typename K, typename V, typename Hasher, typename Keyeq, typename Alloc>
	const V&  map_get_def(const std::unordered_map<K, V, Hasher, Keyeq, Alloc>& cont, const K& key, const V& default_value = V())
	{
		auto  p = cont.find(key);
		if(p == cont.end())
			return default_value;

		return p->second;
	}
	//	& specialization for 'nullptr'
	template<typename K, typename V, typename Hasher, typename Keyeq, typename Alloc>
	const V&  map_get_def(const std::unordered_map<K, V, Hasher, Keyeq, Alloc>& cont, const K& key, void* ptr)
	{
		return map_get_def(cont, key, (V)ptr);
	}




	template<typename CONTAINER, typename FUNC>
	FUNC  for_each(CONTAINER& cont, FUNC func)
	{
		return std::for_each(cont.begin(), cont.end(), func);
	}
	template<typename CONTAINER, typename FUNC>
	FUNC  for_each(const CONTAINER& cont, FUNC func)
	{
		return std::for_each(cont.begin(), cont.end(), func);
	}


	template<typename CONTAINER, typename OBJ_T>
	typename CONTAINER::iterator  find(CONTAINER& cont, const OBJ_T& obj)
	{
		return std::find(cont.begin(), cont.end(), obj);
	}
	template<typename CONTAINER, typename OBJ_T>
	typename CONTAINER::const_iterator  find(const CONTAINER& cont, const OBJ_T& obj)
	{
		return std::find(cont.begin(), cont.end(), obj);
	}
	template<typename K, typename V, typename A, typename C, typename T>
	typename std::map<K,V,A,C>::iterator  find(std::map<K,V,A,C>& cont, const T& key)
	{
		return cont.find(key);
	}
	template<typename K, typename V, typename A, typename C, typename T>
	typename std::map<K,V,A,C>::const_iterator  find(const std::map<K,V,A,C>& cont, const T& key)
	{
		return cont.find(key);
	}

	template<typename CONTAINER, typename FUNC>
	auto  find_if(CONTAINER& cont, FUNC func) -> decltype(std::begin(cont))
	{
		return std::find_if(std::begin(cont), std::end(cont), func);
	}
	template<typename CONTAINER, typename FUNC>
	typename CONTAINER::const_iterator  find_if(const CONTAINER& cont, FUNC func)
	{
		return std::find_if(cont.begin(), cont.end(), func);
	}


	template<typename CONTAINER, typename OBJ_T>
	typename CONTAINER::iterator  find_a(CONTAINER& cont, const OBJ_T& obj)
	{
		typename CONTAINER::iterator p = find(cont.begin(), cont.end(), obj);
		assert(p != cont.end());

		return p;
	}
	template<typename CONTAINER, typename OBJ_T>
	typename CONTAINER::const_iterator  find_a(const CONTAINER& cont, const OBJ_T& obj)
	{
		typename CONTAINER::const_iterator p = find(cont, obj);
		assert(p != cont.end());

		return p;
	}

	template<typename CONTAINER, typename FUNC>
	auto  find_if_a( CONTAINER& cont, FUNC func ) -> decltype(std::begin(cont))
	{
		typename CONTAINER::iterator p = find_if(cont, func);
		assert(p != cont.end());

		return p;
	}
	template<typename CONTAINER, typename FUNC>
	typename CONTAINER::const_iterator  find_if_a(const CONTAINER& cont, FUNC func)
	{
		typename CONTAINER::const_iterator p = find_if(cont, func);
		assert(p != cont.end());

		return p;
	}

	template<typename CONT, typename T>
	bool is_exist(const CONT& cont, const T& value)
	{
		return std::find(cont.begin(), cont.end(), value) != cont.end();
	}

	template<typename CV, typename T>
	bool is_exist(const std::initializer_list<CV>& cont, const T& value)
	{
		return std::find(cont.begin(), cont.end(), value) != cont.end();
	}

	template<typename K, typename V, typename P, typename A>
	bool is_exist(const std::map<K, V, P, A>& cont, const K& value)
	{
		return cont.find(value) != cont.end();
	}
	template<typename K, typename P, typename A>
	bool is_exist(const std::set<K, P, A>& cont, const K& value)
	{
		return cont.find(value) != cont.end();
	}
	template<typename K, typename V, typename H, typename KEq, typename A>
	bool is_exist(const std::unordered_map<K, V, H, KEq, A>& cont, const K& value)
	{
		return cont.find(value) != cont.end();
	}
	template<typename K, typename H, typename KEq, typename A>
	bool is_exist(const std::unordered_set<K, H, KEq, A>& cont, const K& value)
	{
		return cont.find(value) != cont.end();
	}


	template<typename CONT, typename FUNC>
	bool if_exist(const CONT& cont, FUNC func)
	{
		return find_if(cont, func) != std::end(cont);
	}



	template<typename CONT, typename T>
	bool insert( CONT& cont, T&& value )
	{
		cont.push_back( std::forward<T>(value) );
		return true;
	}

	template<typename K, typename V, typename P, typename A, typename V1>
	bool insert( std::map<K, V, P, A>& cont, V1&& value )
	{
		return cont.insert( std::forward<V1>( value ) ).second;
	}

	template<typename K, typename V, typename P, typename A, typename V1>
	bool insert( std::multimap<K, V, P, A>& cont, V1&& value )
	{
		cont.insert( std::forward<V1>( value ) );
		return true;
	}

	template<typename K, typename P, typename A, typename V1>
	bool insert( std::set<K, P, A>& cont, V1&& value )
	{
		return cont.insert( std::forward<V1>( value ) ).second;
	}

	template<typename K, typename P, typename A, typename V1>
	bool insert( std::multiset<K, P, A>& cont, V1&& value )
	{
		cont.insert( std::forward<V1>( value ) );
		return true;
	}


	namespace details
	{
		template<typename T>
		inline void reserve(T&  , size_t  ) {}

		template<typename T, typename A>
		inline void reserve(std::vector<T,A>& cont, size_t alloc_size) 
		{
			cont.reserve(alloc_size);
		}
	}

	template<typename SRC_CONTAINER, typename DST_CONTAINER>
	bool copy(const SRC_CONTAINER& src, DST_CONTAINER& dst)
	{
		return copy_if( src, dst, [] (auto& ) { return true; } );
	}

	template<typename SRC_CONTAINER, typename DST_CONTAINER, typename F>
	auto copy(const SRC_CONTAINER& src, DST_CONTAINER& dst, F transform) -> typename std::is_class< decltype(transform(*src.begin())) >::value_type
	{
		details::reserve(dst, dst.size() + src.size());

		bool ret_val = false;
		for(typename SRC_CONTAINER::const_iterator p = src.begin(); p != src.end(); ++p)
			ret_val = true, insert(dst, transform(*p));

		return ret_val;
	}

	template<typename SRC_CONTAINER, typename DST_CONTAINER, typename PTR_TYPE>
	auto copy(const SRC_CONTAINER& container, DST_CONTAINER& dst, PTR_TYPE member_ptr)->std::enable_if_t< std::is_member_object_pointer<PTR_TYPE>::value, bool >
	{
		return copy(container, dst, [=](auto& val) { return val.*member_ptr; } );
	}

	template<typename SRC_CONTAINER, typename DST_CONTAINER, typename FUNC>
	bool copy_if(const SRC_CONTAINER& src, DST_CONTAINER& dst, FUNC predicate)
	{
		bool ret_val = false;

		for(typename SRC_CONTAINER::const_iterator p = src.begin(); p != src.end(); ++p)
			if( predicate(*p) )
				ret_val = true, insert(dst, *p);

		return ret_val;
	}

	template<typename SRC_CONTAINER, typename DST_CONTAINER>
	bool move( SRC_CONTAINER& src, DST_CONTAINER& dst )
	{
		bool ret_val = false;

		for( typename SRC_CONTAINER::iterator p = src.begin(); p != src.end(); ++p )
			ret_val = true, insert( dst, std::move(*p) );

		return ret_val;
	}



	template<typename CONT, typename PRED>
	auto remove_duplicates(CONT& container, const PRED& pred) -> std::enable_if_t< !std::is_member_pointer<PRED>::value >
	{
		std::sort( std::begin(container), std::end(container), pred );
		container.erase( std::unique(std::begin(container), std::end(container)), std::end(container) );
	}
	/*

	template<typename T, typename A, typename PRED>
	auto remove_duplicates(TArray<T, A>& container, const PRED& pred) -> std::enable_if_t< !std::is_member_pointer<PRED>::value >
	{
		container.Sort(pred);

		//	not ideal, but TArray makes it very hard (and unsafe) to use STL algorithms with it
		for(int i = 1; i < container.Num(); )
			if( pred(container[i-1], container[i]) == false && pred(container[i], container[i-1]) == false )	//	if equal
				container.RemoveAt(i);
			else
				++i;
	}
*/

	template<typename CONT, typename CLASS_TYPE, typename MEM_TYPE>
	void remove_duplicates(CONT& container, MEM_TYPE CLASS_TYPE::*member_ptr)
	{
		using MEM_PTR = MEM_TYPE CLASS_TYPE::*;
		using ElType = std::remove_reference_t<decltype(container[0])>;
		static_assert( std::is_same<CLASS_TYPE, ElType>::value, "" );

		//	define predicate
		struct Pred
		{
			MEM_PTR member_ptr;
			Pred(MEM_PTR a_mem_ptr) : member_ptr(a_mem_ptr) {}
			bool operator()(const ElType& a, const ElType& b) const { return (a.*member_ptr) < (b.*member_ptr); }
		};

		return remove_duplicates(container, Pred(member_ptr));
	}


	template<typename SRC_CONTAINER, typename DST_CONTAINER, typename FUNC>
	bool move_if(SRC_CONTAINER& src, DST_CONTAINER& dst, FUNC predicate)
	{
		bool ret_val = false;

		for (typename SRC_CONTAINER::const_iterator p = src.begin(); p != src.end();  )
			if (predicate(*p))
			{
				ret_val = true;
				alg::insert(dst, std::move(*p));

				//	not very effective for vectors (std::remove_if is better), and for maps (you may move the whole node like this, not only value) ...
				p = src.erase(p);
			}

		return ret_val;
	}



	template<typename SRC_CONTAINER, typename PTR_TYPE, typename FUNC = std::ranges::less>
	auto sort(SRC_CONTAINER& container, PTR_TYPE obj_ptr, FUNC predicate = std::ranges::less()) -> std::enable_if_t< std::is_member_object_pointer<PTR_TYPE>::value >
	{
		return std::sort(container.begin(), container.end(), [=](auto& a, auto& b)
		{
			return predicate(a.*obj_ptr, b.*obj_ptr);
		});
	}
	template<typename SRC_CONTAINER, typename FUNC>
	auto sort(SRC_CONTAINER& container, FUNC predicate) -> std::enable_if_t< !std::is_member_object_pointer<FUNC>::value >
	{
		return std::sort(container.begin(), container.end(), predicate);
	}
	template<typename SRC_CONTAINER>
	void sort(SRC_CONTAINER& container)
	{
		return std::sort(container.begin(), container.end());
	}
	template<typename SRC_CONTAINER, typename FUNC>
	void partial_sort(SRC_CONTAINER& container, size_t elnum, FUNC functor)
	{
		elnum = std::min(elnum, container.size());
		return std::partial_sort(container.begin(), container.begin() + elnum, container.end(), functor);
	}
	template<typename SRC_CONTAINER>
	void partial_sort(SRC_CONTAINER& container, size_t elnum)
	{
		elnum = std::min(elnum, container.size());
		return std::partial_sort(container.begin(), container.begin() + elnum, container.end());
	}


	template<class CONT, class PRED>
	inline typename CONT::const_iterator min_element(const CONT& cont, const PRED& pred)
	{
		return std::min_element(std::begin(cont), std::end(cont), pred);
	}
	template<class CONT, class PRED>
	inline typename CONT::const_iterator max_element(const CONT& cont, const PRED& pred)
	{
		return std::max_element(std::begin(cont), std::end(cont), pred);
	}

	template<typename SRC_CONTAINER, typename T>
	bool binary_search( SRC_CONTAINER& container, const T& val )
	{
		return std::binary_search( begin(container), end(container), val );
	}
	template<typename SRC_CONTAINER, typename T, typename FUNC>
	bool binary_search(SRC_CONTAINER& container, const T& val, const FUNC& extract_value_functor)
	{
		using ContElType = typename std::remove_cv< typename std::remove_reference<decltype( *begin(container) )>::type >::type;
		struct Predicate
		{
			Predicate(const FUNC& f_) : f(f_) {}
			const FUNC& f;

			bool operator()(const ContElType& a, const T& b) const { return f(a) < b; }
			bool operator()(const T& a, const ContElType& b) const { return a < f(b); }
			bool operator()(const ContElType& a, const ContElType& b) const { return f(a) < f(b); }
			bool operator()(const T& a, const T& b) const { return a < b; }
		};

		return std::binary_search(begin(container), end(container), val, Predicate(extract_value_functor));
	}


	template<typename SRC_CONTAINER, typename T>
	auto upper_bound( SRC_CONTAINER& container, const T& val ) -> decltype(begin(container))
	{
		return std::upper_bound( begin( container ), end( container ), val );
	}
	template<typename SRC_CONTAINER, typename T, typename FUNC>
	auto upper_bound(SRC_CONTAINER& container, const T& val, const FUNC& extract_value_functor) -> decltype(begin(container))
	{
		using ContElType = typename std::remove_cv< typename std::remove_reference<decltype(*begin(container))>::type >::type;
		struct Predicate
		{
			Predicate(const FUNC& f_) : f(f_) {}
			const FUNC& f;

			bool operator()(const ContElType& a, const T& b) const { return f(a) < b; }
			bool operator()(const T& a, const ContElType& b) const { return a < f(b); }
			bool operator()(const ContElType& a, const ContElType& b) const { return f(a) < f(b); }
			bool operator()(const T& a, const T& b) const { return a < b; }
		};

		return std::upper_bound(begin(container), end(container), val, Predicate(extract_value_functor));
	}

	template<typename SRC_CONTAINER, typename T>
	auto lower_bound( SRC_CONTAINER& container, const T& val ) -> decltype(begin(container))
	{
		return std::lower_bound( begin( container ), end( container ), val );
	}
	template<typename SRC_CONTAINER, typename T, typename FUNC>
	auto lower_bound(SRC_CONTAINER& container, const T& val, const FUNC& extract_value_functor) -> decltype(begin(container))
	{
		using ContElType = typename std::remove_cv< typename std::remove_reference<decltype(*begin(container))>::type >::type;
		struct Predicate
		{
			Predicate(const FUNC& f_) : f(f_) {}
			const FUNC& f;

			bool operator()(const ContElType& a, const T& b) const { return f(a) < b; }
			bool operator()(const T& a, const ContElType& b) const { return a < f(b); }
			bool operator()(const ContElType& a, const ContElType& b) const { return f(a) < f(b); }
			bool operator()(const T& a, const T& b) const { return a < b; }
		};

		return std::lower_bound(begin(container), end(container), val, Predicate(extract_value_functor));
	}

	template<typename SRC_CONTAINER, typename URBG>
	void shuffle(SRC_CONTAINER& container, URBG& rnd)
	{
		return std::shuffle(container.begin(), container.end(), rnd);
	}

	template<typename SRC_CONTAINER, typename OBJ>
	unsigned count(const SRC_CONTAINER& container, const OBJ& element)
	{
		unsigned  result = 0;

		for(typename SRC_CONTAINER::const_iterator p = container.begin(); p != container.end(); ++p)
			if(*p == element)
				++result;

		return result;
	}

	template<typename SRC_CONTAINER, typename FUNCTOR>
	unsigned count_if(const SRC_CONTAINER& container, FUNCTOR functor)
	{
		unsigned  result = 0;

		for(typename SRC_CONTAINER::const_iterator p = container.begin(); p != container.end(); ++p)
			if( functor(*p) )
				++result;

		return result;
	}

	template<typename SRC_CONTAINER>
	auto sum(const SRC_CONTAINER& container) -> std::remove_reference_t<decltype( *std::begin(container) )>
	{
		using result_type = decltype( *std::begin(container) );

		std::remove_const_t<std::remove_reference_t<result_type>>  sum = {};

		for(auto p = std::begin(container); p != std::end(container); ++p)
			sum += *p;

		return sum;
	}

	template<typename SRC_CONTAINER>
	typename SRC_CONTAINER::value_type  multiply(const SRC_CONTAINER& container)
	{
		typename SRC_CONTAINER::value_type  mul = 1;

		for(typename SRC_CONTAINER::const_iterator p = container.begin(); p != container.end(); ++p)
			mul *= *p;

		return mul;
	}

	template<typename SRC_CONTAINER, typename FUNCTOR>
	auto accumulate(const SRC_CONTAINER& container, FUNCTOR functor) -> std::remove_const_t<std::remove_reference_t<decltype(functor(*container.begin()))>>
	{
		typedef std::remove_const_t<std::remove_reference_t<decltype(functor(*container.begin()))>>  result_type;
		result_type  sum = {};

		for(typename SRC_CONTAINER::const_iterator p = container.begin(); p != container.end(); ++p)
			sum += functor(*p);

		return sum;
	}


	//	to escape local begin(), end() definitions
	auto begin_adl(const auto& cont) { return begin(cont); }
	auto end_adl(const auto& cont) { return end(cont); }
	auto begin_adl(auto& cont) { return begin(cont); }
	auto end_adl(auto& cont) { return end(cont); }

	namespace details
	{
		struct empty_type {};

		template< typename CONT, typename IT>
		struct Select1stIt
		{
			Select1stIt(CONT cont) : holder(std::forward<CONT>(cont)), begin_it(begin_adl(holder)), end_it(end_adl(holder)) {}
			Select1stIt(IT begin_it, IT end_it) : begin_it( begin_it ), end_it( end_it ) {}

			CONT holder;	//	to hold decaying container
			IT begin_it;
			IT end_it;

			using value_type = std::remove_reference_t<decltype(begin_it->first)>;

			template<typename ItValueType>
			struct iterator
			{
				IT  it;

				using iterator_category = IT::iterator_category;
				using value_type = ItValueType;	//	const value_type or value_type
				using difference_type = typename IT::difference_type;
				using pointer = value_type*;
				using reference = value_type&;

				iterator(IT it) : it(it) {}

				bool operator==(const instance_of<iterator> auto& other) const { return it == other.it; }
				bool operator!=(const instance_of<iterator> auto& other) const { return it != other.it; }
				iterator& operator+=(size_t idx) { it += idx; return *this; }
				typename IT::difference_type operator-(const iterator& other) { return it - other.it; }
				iterator& operator++() { ++it; return *this; }
				iterator& operator++(int) { it++; return *this; }
				iterator& operator--() { --it; return *this; }
				iterator& operator--(int) { it--; return *this; }
				reference  operator*() const { return it->first; }
				pointer   operator->() const { return &it->first; }

				IT base_iter() const { return it; }
			};

			auto begin()		{ return iterator<value_type>(begin_it); }
			auto end()			{ return iterator<value_type>(end_it); }
			auto begin() const	{ return iterator<const value_type>(begin_it); }
			auto end() const	{ return iterator<const value_type>(end_it); }
			size_t size() const { return std::distance(begin_it, end_it); }
		};

		template<typename CONT, typename IT>
		struct Select2ndIt
		{
			Select2ndIt(CONT&& cont) : holder(std::forward<CONT>(cont)), begin_it(begin_adl(holder)), end_it(end_adl(holder)) {}
			Select2ndIt(IT begin_it, IT end_it) : begin_it( begin_it ), end_it( end_it ) {}

			CONT holder;	//	to hold possibly decaying container
			IT begin_it;
			IT end_it;

			using value_type = std::remove_reference_t<decltype(begin_it->second)>;

			template<typename ItValueType>
			struct iterator
			{
				IT  it;

				using iterator_category = IT::iterator_category;
				using value_type = ItValueType;
				using difference_type = typename IT::difference_type;
				using pointer = value_type*;
				using reference = value_type&;

				iterator(IT it) : it(it) {}

				bool operator==(const instance_of<iterator> auto& other) const { return it == other.it; }
				bool operator!=(const instance_of<iterator> auto& other) const { return it != other.it; }
				iterator& operator+=(size_t idx) { it += idx; return *this; }
				typename IT::difference_type operator-(const iterator& other) { return it - other.it; }
				iterator& operator++() { ++it; return *this; }
				iterator& operator++(int) { it++; return *this; }
				iterator& operator--() { --it; return *this; }
				iterator& operator--(int) { it--; return *this; }
				reference  operator*() const { return it->second; }
				pointer   operator->() const { return &it->second; }

				IT base_iter() const { return it; }
			};
			
			auto begin()		{ return iterator<value_type>(begin_it); }
			auto end()			{ return iterator<value_type>(end_it); }
			auto begin() const	{ return iterator<const value_type>(begin_it); }
			auto end() const	{ return iterator<const value_type>(end_it); }
			size_t size() const { return std::distance(begin_it, end_it); }
		};

		template<typename CONT, typename MEM_PTR>
		struct SelectMemberIt
		{
			CONT cont;
			MEM_PTR ptr;

			using value_type = std::remove_reference_t<decltype((*cont.begin()).*ptr)>;
			using cont_iterator_type = std::remove_cvref_t<CONT>::iterator;

			SelectMemberIt(CONT cont, MEM_PTR ptr) : cont(std::forward<CONT>(cont)), ptr(ptr) {}

			template<typename ItValueType>
			struct iterator
			{
				//	idea with ItValueType - its either `const value_type` or just `value_type` - deriving two iterators
				using iterator_category = cont_iterator_type::iterator_category;
				using value_type = ItValueType;
				cont_iterator_type  it;
				MEM_PTR ptr;

				using pointer = value_type*;
				using reference = value_type&;

				iterator(cont_iterator_type it, MEM_PTR ptr) : it(it), ptr(ptr) {}

				iterator& operator++() { ++it; return *this; }
				iterator& operator++(int) { it++; return *this; }
				iterator& operator--() { --it; return *this; }
				iterator& operator--(int) { it--; return *this; }
				reference  operator*() const { return (*it).*ptr; }
				pointer   operator->() const { return &(*it).*ptr; }
				bool operator==(const instance_of<iterator> auto& other) const { return it == other.it; }
				bool operator!=(const instance_of<iterator> auto& other) const { return it != other.it; }

				cont_iterator_type base() { return it; }
			};

			auto begin()		{ return iterator<value_type>(cont.begin(), ptr); }
			auto end()			{ return iterator<value_type>(cont.end(), ptr); }
			auto begin() const	{ return iterator<const value_type>(cont.begin(), ptr); }
			auto end() const	{ return iterator<const value_type>(cont.end(), ptr); }
			size_t size() const { return std::distance(cont.begin(), cont.end()); }
		};

		auto begin(instance_of<Select1stIt> auto& val) { return val.begin(); }
		auto begin(instance_of<Select2ndIt> auto& val) { return val.begin(); }
		auto begin(instance_of<SelectMemberIt> auto& val) { return val.begin(); }
		auto end(instance_of<Select1stIt> auto& val) { return val.end(); }
		auto end(instance_of<Select2ndIt> auto& val) { return val.end(); }
		auto end(instance_of<SelectMemberIt> auto& val) { return val.end(); }
	}	//namespace details


	template<typename CONT>
	auto select1st(CONT&& cont) { return details::Select1stIt<CONT, decltype(begin(cont))>(std::forward<CONT>(cont)); }
	template<typename IT>
	auto select1st(IT begin_it, IT end_it) { return details::Select1stIt<details::empty_type, IT>(begin_it, end_it); }

	template<typename CONT>
	auto select2nd(CONT&& cont) { return details::Select2ndIt<CONT, decltype(std::begin(cont))>(std::forward<CONT>(cont)); }
	template<typename IT>
	auto select2nd(IT begin_it, IT end_it) { return details::Select2ndIt<details::empty_type, IT>(begin_it, end_it); }

	template<typename CONT, typename MEM_PTR>
	auto select_member(CONT&& cont, MEM_PTR ptr)
	{
		//	For decaying container, CONT will be just CONT, and container will be moved into SelectMemberIt
		//	For regular reference to container, CONT will be CONT& and reference will be stored instead
		return details::SelectMemberIt<CONT, MEM_PTR>( std::forward<CONT>(cont), ptr);
	}

	template<typename U, typename URBG>
	unsigned random_choose(const U& weights, URBG& rnd)
	{
		assert( std::begin(weights) != std::end(weights) );
		double sum = static_cast<float>(alg::sum(weights));
		if( sum == 0 )
			return rnd() % ( std::distance(std::begin(weights), std::end(weights)) );

		double frand = rnd() / double(URBG::max());
		double current_weight = 0;

		unsigned idx = 0;
		auto weights_it = std::begin(weights);
		for( ; weights_it != std::end(weights); ++weights_it, ++idx)
		{
			current_weight += *weights_it / sum;
			if(frand <= current_weight)
				return idx;
		}

		assert(idx > 0);
		return idx-1;
	}

	template<typename T, typename U, typename URBG>
	typename T::value_type random_choose(const T& values, const U& weights, URBG& rnd)
	{
		assert(values.size() == weights.size() && values.size() > 0);

		unsigned idx = random_choose(weights, rnd);
		auto it = std::begin(values);
		std::advance(it, idx);

		return *it;
	}

	template<typename IT, typename DIFF>
	IT  advance(IT  it, DIFF  diff)
	{
		std::advance(it, diff);

		return it;
	}

	template<typename V, typename A>
	void  reverse(std::vector<V, A>&  cont)
	{
		std::reverse(cont.begin(), cont.end());
	}

	template<typename V, typename A>
	void  reverse(std::list<V, A>&  cont)
	{
		std::reverse(cont.begin(), cont.end());
	}

	//	Function generates key, that does not exist in the map
	template<typename K, typename V, typename P, typename A>
	typename K  new_map_id(const std::map<K, V, P, A>& m)
	{
		typedef typename std::remove_cv<K>::type  IdxType;

		if(m.empty())
			return 0;

		IdxType last_id = m.rbegin()->first;

		if(last_id < std::numeric_limits<IdxType>::max())
			return last_id + 1;

		std::random_device rd;  // a seed source for the random number engine
		std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
		std::uniform_int_distribution<IdxType> rnd;
		IdxType i = std::numeric_limits<IdxType>::max();

		while(i--)
		{
			IdxType id = rnd();
			if( m.find(id) == m.end() )
				return id;
		}

		return -1;	//	should not reach
	}


}	//	namespace

