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

	private:
		static constexpr bool stored_as_ref = std::is_reference_v<IncomingValueType>;
		using storage_type = std::conditional_t< stored_as_ref, std::reference_wrapper<std::remove_reference_t<IncomingValueType>>, IncomingValueType>;
		storage_type m_value;
	};

	//	to escape local begin(), end() definitions
	auto begin_adl(auto& cont) { return begin(cont); }
	auto end_adl(auto& cont) { return end(cont); }
}

template<typename ParentT>
struct LINQSequence;

template<typename SeqT, typename F>
struct LINQSelect : LINQSequence< LINQSelect<SeqT, F>>
{
	details::ValueHolder<SeqT> seq;
	F functor;

	LINQSelect(SeqT seq, F functor) : seq(std::forward<SeqT>(seq)), functor(std::move(functor))
	{
	}

	//	Contract for LINQSequence
	bool is_empty() const { return seq.get().is_empty(); }
	void operator++() { ++seq.get(); }
	decltype(auto) operator*() { return functor(*seq.get()); }
};

template<typename SeqT, typename F>
struct LINQWhere : LINQSequence < LINQWhere<SeqT, F> >
{
	details::ValueHolder<SeqT> seq;
	F functor;

	LINQWhere(SeqT seq, F functor) : seq(std::forward<SeqT>(seq)), functor(std::move(functor))
	{
		JumpToNextValidEntry();
	}

	void JumpToNextValidEntry()
	{
		while(!is_empty() && !functor(*seq))
			++seq;
	}

	//	Contract for LINQSequence
	bool is_empty() const { return seq.get().is_empty(); }
	void operator++() { ++seq.get(); JumpToNextValidEntry(); }
	decltype(auto) operator*() { return *seq.get(); }
};

template<typename SeqT>
struct LINQTake : LINQSequence < LINQTake<SeqT> >
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
	decltype(auto) operator*() { return *seq.get(); }
};



template<typename SeqT, typename F>
struct LINQGroupSortedBy : LINQSequence < LINQGroupSortedBy<SeqT, F> >
{
	using key_type = std::decay_t<decltype(F()(std::declval<SeqT>()))>;

	details::ValueHolder<SeqT> seq;
	F idExtractor;
	key_type key;

	LINQGroupSortedBy(SeqT seq, F idExtractor) : seq(std::forward<SeqT>(seq)), idExtractor(std::move(idExtractor))
	{
		if(!seq.get().is_empty())
			key = idExtractor(*seq.get());
	}

	struct LINQIdFilter : LINQSequence<LINQIdFilter>
	{
		std::reference_wrapper<SeqT> seq;
		std::reference_wrapper<F> idExtractor;
		key_type key;

		LINQIdFilter(SeqT& seq, key_type key, F& idExtractor) : seq(seq), key(std::move(key)), idExtractor(idExtractor)
		{
		}

		//	Contract for LINQSequence
		bool is_empty() const { return seq.get().is_empty() || idExtractor(*seq) != key; }
		void operator++() { ++seq.get(); }
		decltype(auto) operator*() { return *seq.get(); }
	};

	//	Contract for LINQSequence
	bool is_empty() const { return seq.get().is_empty(); }
	void operator++() {
		if(!seq.get().is_empty())
			key = idExtractor(*seq.get());
	}

	decltype(auto) operator*() const { return LINQIdFilter{ seq, key, idExtractor }; }
};



template<typename ParentT>
struct LINQSequence
{
	//	Interface contract: every descendant class should implement these methods:
//	 bool is_empty() const;
//	 void operator++();
//	 auto operator*();

	using my_type = LINQSequence<ParentT>;

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

	struct iterator
	{
		using difference_type = std::ptrdiff_t;
		using value_type = decltype(*std::declval<ParentT>());

		iterator(ParentT& me) : me(me)
		{
		}

		bool operator==(iterator_sentinel) const { return me.is_empty(); }
		bool operator!=(iterator_sentinel) const { return !me.is_empty(); }
		void operator++() { ++me; }
		decltype(auto) operator*() { return *me; }
		auto* operator->() { return &*me; }

		ParentT& me;
	};

	//static_assert(std::input_iterator<iterator>);

	//	for each friendly
	iterator begin() { return iterator(*static_cast<ParentT*>(this)); }
	iterator_sentinel end() { return {}; }
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
struct LINQ_GenSeq : LINQSequence < LINQ_GenSeq<SeqT> >
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
	auto operator*() { return cur; }
};

template<typename T>
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
template<typename ContainerHolder>
struct LINQ_container : LINQSequence<LINQ_container<ContainerHolder>>
{
	//	either std::container<T> or std::container<T>& or const versions of it
	//	or can be T (&) arr[N]
	details::ValueHolder<ContainerHolder> cont;
	decltype(details::begin_adl(cont.get())) it;

	explicit LINQ_container(ContainerHolder cont)
		: cont(std::forward<ContainerHolder>(cont))
		, it(details::begin_adl(cont) )
	{}

	//	Contract for LINQSequence
	bool is_empty() const { return it == details::end_adl(cont.get()); }
	void operator++() { ++it; }
	auto& operator*() { return *it; }
};


template<details::SupportedContainer T>
auto LINQ(T&& cont)
{
	return LINQ_container<T>(std::forward<T>(cont));
}


}
