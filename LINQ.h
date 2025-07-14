#pragma once

#include <vector>
#include <set>
#include <map>
#include <assert.h>
#include "IsInstanceOf.h"

//
//	LINQ in C++
//
//	Usage sample:
//	std::vector<std::string>	v;
//	for( size_t val :
//		LINQ(v)
//		.Select([](const std::string& val) { return val.length(); })
//		.Where([](size_t val) { return val == 2; })
//		.Skip(1)
//	) { .. }
//
//	Supported
//		* all containers (maps iterated as pairs)
//		* abstract generator LINQRange[begin, end)
//
//	Supported transformers:
//		* Sequence Select(const F& transform)			//	Transforms sequence of val into sequence of transform(val)
//		* Sequence Where(const F& predicate)			//	Filters by predicate
//		* bool     Any(const F& predicate)				//	Evaluates if any element in sequence fits predicate
//		* int      Count(const F& functor)				//	Evaluates number of elements in sequence that fits predicate
//		* Sequence Take(int num)						//	Trims only first 'num' elements out of sequence
//		* Sequence Skip(int num)						//	Safely skips first 'num' elements
//		* Element  First()								//	Extracts first element of the sequence. Will ASSERT if empty.
//		* Keys Sequence of Sequence	GroupSortedBy(f)	//	Groups elements by key (in sorted sequence) and returns keys sequence that evaluates into
//														//		sequence of original elements with the same key.
//		* T			Aggregate(init, functor)			//	Evaluates sequence using initial value and wrapping functor
//		* vector<El> ToVector()							//	Converts sequence into std::vector
//
//

namespace linq {

namespace details {
	template<typename T, typename U = std::remove_cvref_t<T>>
	concept SupportedContainer = requires (U cont)
	{
		typename U::value_type;
		std::begin(cont) == std::end(cont);
		++std::begin(cont);
		*std::begin(cont);
	};

	//  Value holder that unifies the interface.
	//  Idea that we can store by value or reference.
	//	If reference - we use reference_wrapper so its copyable, to access - use get()
	//	If by value - sure, but we want the same interface, like T& get() to access the underlying value
	template<typename IncomingValueType>
	struct ValueHolder
	{
		ValueHolder(IncomingValueType value)
			: m_value(std::forward<IncomingValueType>(value)) {
		}

		auto& get()
		{
			if constexpr (stored_as_ref)
			{
				return m_value.get();
			}
			else
			{
				return m_value;
			}
		}
		auto& get() const
		{
			if constexpr (stored_as_ref)
			{
				return m_value.get();
			}
			else
			{
				return m_value;
			}
		}

	private:
		static constexpr bool stored_as_ref = std::is_reference_v<IncomingValueType>;
		using storage_type = std::conditional_t< stored_as_ref, std::reference_wrapper<std::remove_reference_t<IncomingValueType>>, IncomingValueType>;
		storage_type m_value;
	};

	//	to escape local begin(), end() definitions
	auto begin_adl(const auto& cont) { return begin(cont); }
	auto end_adl(const auto& cont) { return end(cont); }
	auto begin_adl(auto& cont) { return begin(cont); }
	auto end_adl(auto& cont) { return end(cont); }
}

template<typename ParentT, typename ValueType>
struct LINQSequence;

template<typename SeqT, typename F>
struct LINQSelect : LINQSequence< LINQSelect<SeqT, F>, decltype(F()( std::declval<typename std::decay_t<SeqT>::value_type>() ))>
{
	details::ValueHolder<SeqT> seq;
	F functor;

	LINQSelect(SeqT seq, F functor) : seq(std::forward<SeqT>(seq)), functor(std::move(functor))
	{
	}

	//	Contract for LINQSequence
	bool is_empty() const { return seq.get().is_empty(); }
	void operator++() { ++seq.get(); }
	decltype(auto) operator*() const { return functor(*seq.get()); }
};

template<typename SeqT, typename F>
struct LINQWhere : LINQSequence< LINQWhere<SeqT, F>, decltype(std::declval<SeqT>().operator*()) >
{
	details::ValueHolder<SeqT> seq;
	F functor;

	LINQWhere(SeqT seq, F functor) : seq(std::forward<SeqT>(seq)), functor(std::move(functor))
	{
		JumpToNextValidEntry();
	}

	void JumpToNextValidEntry()
	{
		while(!is_empty() && !functor(*seq.get()))
			++seq.get();
	}

	//	Contract for LINQSequence
	bool is_empty() const { return seq.get().is_empty(); }
	void operator++() { ++seq.get(); JumpToNextValidEntry(); }
	decltype(auto) operator*() const { return *seq.get(); }
};

template<typename SeqT>
struct LINQTake : LINQSequence< LINQTake<SeqT>, decltype(std::declval<SeqT>().operator*()) >
{
	details::ValueHolder<SeqT> seq;
	int num;
	int idx = 0;

	LINQTake(SeqT seq, int num) : seq(std::forward<SeqT>(seq)), num(num)
	{
	}

	//	Contract for LINQSequence
	bool is_empty() const { return idx < num || num <= 0 || seq.get().is_empty(); }
	void operator++() { ++idx; ++seq.get(); }
	decltype(auto) operator*() const { return *seq.get(); }
};

namespace details
{
	template<typename SeqT, typename F>
	struct LINQIdFilter : LINQSequence<LINQIdFilter<SeqT, F>, decltype( std::declval<SeqT>().operator*() )>
	{
		using key_type = std::decay_t<decltype(F()( *std::declval<SeqT>().begin() ))>;

		std::reference_wrapper<SeqT> seq;
		std::reference_wrapper<F> idExtractor;
		key_type key;

		LINQIdFilter(SeqT& seq, key_type key, F& idExtractor) : seq(seq), key(std::move(key)), idExtractor(idExtractor)
		{
		}

		//	Contract for LINQSequence
		bool is_empty() const { return seq.get().is_empty() || idExtractor(*seq) != key; }
		void operator++() { ++seq.get(); }
		decltype(auto) operator*() const { return *seq.get(); }
	};
}

template<typename SeqT, typename F>
struct LINQGroupSortedBy : LINQSequence< LINQGroupSortedBy<SeqT, F>, details::LINQIdFilter<std::decay_t<SeqT>, std::decay_t<F>> >
{
	using key_type = std::decay_t<decltype(F()( std::declval<SeqT>().begin().operator*() ))>;
	using value_type = details::LINQIdFilter<std::decay_t<SeqT>, std::decay_t<F>>;

	details::ValueHolder<SeqT> seq;
	F idExtractor;
	key_type key;

	LINQGroupSortedBy(SeqT seq, F idExtractor) : seq(std::forward<SeqT>(seq)), idExtractor(std::move(idExtractor))
	{
		if(!seq.get().is_empty())
			key = idExtractor(*seq.get());
	}

	//	Contract for LINQSequence
	bool is_empty() const { return seq.get().is_empty(); }
	void operator++() {
		if(!seq.get().is_empty())
			key = idExtractor(*seq.get());
	}

	auto operator*() const { return value_type{ seq, key, idExtractor }; }
};


//	YieldType is needed due to CRTP instantiation - CRTP base is instantiated first and it does not have definition of derived class - querying parent fails with "use of undefined type"
//	YieldType is exactly what operator*() of parent would return, usually cref of value_type
template<typename ParentT, typename YieldType>
struct LINQSequence
{
	//	Interface contract: every descendant class should implement these methods:
//	 bool is_empty() const;
//	 void operator++();
//	 auto operator*();

	using my_type = LINQSequence<ParentT, YieldType>;
	using value_type = std::remove_cvref_t<YieldType>;


	//	Note, lot's of moves - the LINQ is expected to be used as cascade of decorators, where the topmost decorator would own all other decorators in chain
	template<typename F>
	LINQSelect< ParentT, F > Select(const F& functor) { return LINQSelect< ParentT, F >(std::move(*static_cast<ParentT*>(this)), functor); }
	template<typename F>
	LINQWhere< ParentT, F > Where(const F& functor) { return LINQWhere< ParentT, F >(std::move(*static_cast<ParentT*>(this)), functor); }
	template<typename F>
	bool					Any(const F& functor) { for(auto&& val : *this) if(functor(val)) return true; return false; }
	template<typename F>
	int						Count(const F& functor) { int res = 0; for(auto&& val : *this) res += functor(val) ? 1 : 0; return res; }

	LINQTake< ParentT >		Take(int num) { return LINQTake< ParentT >(std::move(*static_cast<ParentT*>(this)), num); }

	auto&					First() { ParentT& fullThis = *static_cast<ParentT*>(this);  assert(!fullThis.is_empty()); return *fullThis; }

	//	Groups sorted sequence by key
	template<typename F>
	LINQGroupSortedBy< ParentT, F>  GroupSortedBy(F IdExtractF) { return LINQGroupSortedBy< ParentT, F >(std::move(*static_cast<ParentT*>(this)), std::move(IdExtractF)); }

	template<typename T, typename F>
	T						Aggregate(T init_val, const F& functor)
	{
		ParentT& fullThis = *static_cast<ParentT*>(this);
		while(!fullThis.is_empty())
		{
			init_val = functor(init_val, *fullThis);
			++fullThis;
		}

		return init_val;
	}

	ParentT					Skip(int num)
	{
		ParentT& fullThis = *static_cast<ParentT*>(this);
		while(num-- > 0 && !fullThis.is_empty())
			++fullThis;

		return fullThis;
	}

	auto ToVector()
	{
		std::vector<std::decay_t<decltype(First())>> list;

		ParentT& fullThis = *static_cast<ParentT*>(this);
		while(!fullThis.is_empty())
		{
			list.push_back(*fullThis);
			++fullThis;
		}

		return list;
	}

	struct iterator_sentinel {};

	template<typename ItValueType>
	struct iterator
	{
		using difference_type = std::ptrdiff_t;
		using value_type = ItValueType;

		iterator(ParentT& me) : me(me)
		{}

		bool operator==(iterator_sentinel) const { return me.is_empty(); }
		bool operator!=(iterator_sentinel) const { return !me.is_empty(); }
		void operator++() { ++me; }

		decltype(auto) operator*() const { return *me; }

		ParentT& me;
	};

	//static_assert(std::input_iterator<iterator>);

	//	for each friendly
	auto begin()		{ return iterator<value_type>(*static_cast<ParentT*>(this)); }
	//	constness is provided by underlying const value, but the underlying iterator itself cannot be const - it'll be updating internal values during traversing
	auto begin() const  { return iterator<const value_type>(*const_cast<ParentT*>( static_cast<const ParentT*>(this))); }
	iterator_sentinel end() const { return {}; }
};


auto begin(instance_of<LINQSequence> auto& val)
{
	return val.begin();
}
auto end(instance_of<LINQSequence> auto& val)
{
	return val.end();
}


//////////////////////////////////////////////////////////////////////////
//	Generator
template<typename SeqT>
struct LINQ_GenSeq : LINQSequence< LINQ_GenSeq<SeqT>, SeqT >
{
	const SeqT end_;	//	not included
	SeqT cur;

	LINQ_GenSeq(SeqT begin, SeqT end) : end_(end), cur(begin)
	{
		assert(cur <= end);
	}

	//	Contract for LINQSequence
	bool is_empty() const { return cur == end_; }
	void operator++() { ++cur; }
	auto operator*() const { return cur; }
};

template<typename T> requires std::is_arithmetic_v<std::decay_t<T>>
auto LINQRange(T begin, T end)
{
	return LINQ_GenSeq<std::decay_t<T>>(begin, end);
}


//////////////////////////////////////////////////////////////////////////
//	All containers
//
//	ReturnType rules (this is what std::ranges models)
//	* CONT&& -> VALUE&		, reason why not &&, because item can get requested a few times and it's inconvenient to work with this reference
//	* CONT& -> VALUE&
//	* const CONT& -> const VALUE&
//
template<typename ContainerStorageType>
struct LINQ_container : LINQSequence<LINQ_container<ContainerStorageType>, decltype(*details::begin_adl( std::declval<std::decay_t<ContainerStorageType>>() )) >
{
	//	either std::container<T> or std::container<T>& or const versions of it
	//	or can be T (&) arr[N]
	details::ValueHolder<ContainerStorageType> cont;
	decltype(details::begin_adl(cont.get())) it;

	explicit LINQ_container(ContainerStorageType cont)
		: cont(std::forward<ContainerStorageType>(cont))
		, it(details::begin_adl(this->cont.get()) )
	{}

	//	Contract for LINQSequence
	bool is_empty() const { return it == details::end_adl(cont.get()); }
	void operator++() { ++it; }
	auto& operator*() const { return *it; }
};


template<details::SupportedContainer T>
auto LINQ(T&& cont)
{
	return LINQ_container<T>(std::forward<T>(cont));
}


}
