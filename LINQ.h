#pragma once

#include <vector>
#include <set>
#include <map>

//	define your ASSERT macro

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
//	Supported containers:
//		* map
//		* vector
//		* list
//		* arr[]
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


template<typename ParentT, typename ResultType>
struct LINQSequence;

template<typename SeqT, typename F>
struct LINQSelect : LINQSequence< LINQSelect<SeqT, F>, decltype( (*(F*)NULL)	( **(SeqT*)NULL ))	>
{
	SeqT seq;
	const F& functor;

	typedef typename SeqT::ResultType	ParentResultType;
	typedef decltype( (*(F*)NULL)	( **(SeqT*)NULL ))	ResultType;
	typedef LINQSelect<SeqT, F>	MyType;

	LINQSelect(SeqT& seq, const F& functor) : seq(seq), functor(functor)
	{}

	//	Contract for LINQSequence
	bool IsEmpty() const { return seq.IsEmpty(); }
	void operator++() { ++seq; }
	ResultType operator*() const { return functor(*seq); }
};

template<typename SeqT, typename F>
struct LINQWhere : LINQSequence < LINQWhere<SeqT, F>, typename SeqT::ResultType >
{
	SeqT seq;
	const F& functor;

	typedef typename SeqT::ResultType	ParentResultType;
	typedef typename SeqT::ResultType	ResultType;
	typedef LINQWhere<SeqT, F>	MyType;

	LINQWhere( SeqT& seq, const F& functor ) : seq( seq ), functor( functor )
	{
		JumpToNextValidEntry();
	}

	void JumpToNextValidEntry()
	{
		while( !IsEmpty() && !functor( *seq ) )
			++seq;
	}

	//	Contract for LINQSequence
	bool IsEmpty() const { return seq.IsEmpty(); }
	void operator++() { ++seq; JumpToNextValidEntry(); }
	ResultType operator*() const { return *seq; }
};

template<typename SeqT>
struct LINQTake : LINQSequence < LINQTake<SeqT>, typename SeqT::ResultType >
{
	SeqT seq;
	const int num;
	int idx = 0;

	typedef typename SeqT::ResultType	ParentResultType;
	typedef typename SeqT::ResultType	ResultType;
	typedef LINQTake<SeqT>		MyType;

	LINQTake( SeqT& seq, int num ) : seq( seq ), num( num )
	{}

	//	Contract for LINQSequence
	bool IsEmpty() const { return idx < num || num <= 0 || seq.IsEmpty(); }
	void operator++() { ++idx; ++seq; }
	ResultType operator*() const { return *seq; }
};


template<typename SeqT, typename F>
struct LINQIdFilter : LINQSequence < LINQIdFilter<SeqT, F>, typename SeqT::ResultType >
{
	typedef typename SeqT::ResultType	ResultType;
	typedef std::remove_reference_t<decltype(std::declval<F>()(std::declval<typename SeqT::ResultType>()))>  IdType;

	SeqT& seq;
	F idExtractor;
	IdType key;

	LINQIdFilter(SeqT& seq, IdType filterId, F idExtractor) : seq(seq), key(filterId), idExtractor(idExtractor)
	{}

	//	Contract for LINQSequence
	bool IsEmpty() const { return seq.IsEmpty() || idExtractor(*seq) != key; }
	void operator++() { ++seq; }
	ResultType operator*() const { return *seq; }
};


template<typename SeqT, typename F>
struct LINQGroupSortedBy : LINQSequence < LINQGroupSortedBy<SeqT, F>, LINQIdFilter<SeqT, F> >
{
	typedef typename SeqT::ResultType	ParentResultType;
	typedef LINQIdFilter<SeqT, F>		ResultType;
	typedef std::remove_const_t<std::remove_reference_t<decltype(std::declval<F>()(std::declval<typename SeqT::ResultType>()))>>  IdType;

	SeqT seq;
	F idExtractor;
	IdType key;

	LINQGroupSortedBy(SeqT& seq, F idExtractor) : seq(seq), idExtractor(idExtractor)
	{
		if(!seq.IsEmpty())
			key = idExtractor(*seq);
	}

	//	Contract for LINQSequence
	bool IsEmpty() const { return seq.IsEmpty(); }
	void operator++() {
		if (!seq.IsEmpty())
			key = idExtractor(*seq);
	}
	ResultType operator*() const { return LINQIdFilter<SeqT, F>(seq, key, idExtractor); }
};



template<typename ParentT, typename ResultType>
struct LINQSequence
{
	//	Interface contract: every descendant class should implement these methods:
//	 bool IsEmpty() const;
//	 void operator++();
//	 ResultType operator*() const;

	typedef LINQSequence<ParentT, ResultType>	MyType;

	
	template<typename F>
	LINQSelect< ParentT, F > Select(const F& functor) { return LINQSelect< ParentT, F >(*static_cast<ParentT*>(this), functor); }
	template<typename F>
	LINQWhere< ParentT, F > Where( const F& functor ) { return LINQWhere< ParentT, F >( *static_cast<ParentT*>(this), functor ); }
	template<typename F>
	bool					Any ( const F& functor ) { for(auto&& val : *this) if( functor(val) ) return true; return false; }
	template<typename F>
	int						Count ( const F& functor ) { int res = 0; for(auto&& val : *this) res += functor(val) ? 1 : 0; return res; }

	LINQTake< ParentT >		Take ( int num ) { return LINQTake< ParentT >(*static_cast<ParentT*>(this), num); }

	ResultType				First()		{ ParentT& fullThis = *static_cast<ParentT*>(this);  ASSERT(!fullThis.IsEmpty()); return *fullThis; }

	//	Groups sorted sequence by key
	template<typename F>
	LINQGroupSortedBy< ParentT, F>  GroupSortedBy(const F& IdExtractF) { return LINQGroupSortedBy< ParentT, F >(*static_cast<ParentT*>(this), IdExtractF); }

	template<typename T, typename F>
	T						Aggregate( T init_val, const F& functor  )
	{ 
		ParentT& fullThis = *static_cast<ParentT*>(this);
		while(!fullThis.IsEmpty())
		{
			init_val = functor(init_val, *fullThis);
			++fullThis;
		}

		return init_val;
	}

	ParentT					Skip(int num)
	{
		ParentT& fullThis = *static_cast<ParentT*>(this);
		while(num-- > 0 && !fullThis.IsEmpty())
			++fullThis;

		return fullThis;
	}


	std::vector<ResultType> ToVector()
	{
		std::vector<ResultType> list;

		ParentT& fullThis = *static_cast<ParentT*>(this);
		while(!fullThis.IsEmpty())
		{
			list.push_back( *fullThis );
			++fullThis;
		}

		return list;
	}

	//	for each friendly
	struct RefType
	{
		using iterator_category = std::forward_iterator_tag;
		using value_type = std::remove_cv_t<std::remove_reference_t<ResultType>>;
		using difference_type = std::ptrdiff_t;
		using pointer = const value_type * ;
		using reference = ResultType;

		RefType(MyType& me) : me(me)
		{}

		bool operator==(const RefType& ) const { return static_cast<ParentT&>(me).IsEmpty(); }
		bool operator!=(const RefType& ) const { return !static_cast<ParentT&>(me).IsEmpty(); }
		void operator++() { ++static_cast<ParentT&>(me); }
		reference operator*() const { return *static_cast<const ParentT&>(me); }
		pointer operator->() const { return &*static_cast<const ParentT&>(me); }

		MyType& me;
	};

	//	for each friendly
	RefType begin() { return RefType(*this); }
	RefType end() { return RefType(*this); }

	using iterator = RefType;
	using const_iterator = RefType;
	using value_type = std::remove_cv_t<std::remove_reference_t<ResultType>>;
};



//////////////////////////////////////////////////////////////////////////
//	Generator
template<typename SeqT>
struct LINQ_GenSeq : LINQSequence < LINQ_GenSeq<SeqT>, SeqT >
{
	const SeqT end_;	//	not included
	SeqT cur;

	typedef SeqT	ParentResultType;
	typedef SeqT	ResultType;
	typedef LINQ_GenSeq<SeqT>		MyType;

	LINQ_GenSeq( SeqT begin, SeqT end ) : end_( end ), cur(begin)
	{
		ASSERT(cur <= end);
	}

	//	Contract for LINQSequence
	bool IsEmpty() const { return cur == end_; }
	void operator++() { ++cur; }
	ResultType operator*() const { return cur; }
};

template<typename T>
LINQ_GenSeq<T>	LINQRange(T begin, T end)
{
	return LINQ_GenSeq<T>(begin, end);
}


//////////////////////////////////////////////////////////////////////////
//	Vector

template<typename T>
struct LINQ_vector : LINQSequence<LINQ_vector<T>, const T&>
{
	typedef const T& ResultType;

	const std::vector<T>& list;
	typename std::vector<T>::const_iterator it;

	LINQ_vector(const std::vector<T>& list) : list(list)
	{
		it = list.begin();
	}

	//	Contract for LINQSequence
	bool IsEmpty() const { return it == list.end(); }
	void operator++() { ++it; }
	ResultType operator*() const { return *it; }
};


template<typename T>
LINQ_vector<T>	LINQ(const std::vector<T>& list)
{
	return LINQ_vector<T>(list);
}



//////////////////////////////////////////////////////////////////////////
//	Set

template<typename T, typename L, typename A>
struct LINQ_set : LINQSequence<LINQ_set<T,L,A>, const T&>
{
	typedef const T& ResultType;

	const std::set<T,L,A>& list;
	typename std::set<T,L,A>::const_iterator it;

	LINQ_set(const std::set<T,L,A>& list) : list(list)
	{
		it = list.begin();
	}

	//	Contract for LINQSequence
	bool IsEmpty() const { return it == list.end(); }
	void operator++() { ++it; }
	ResultType operator*() const { return *it; }
};


template<typename T, typename L, typename A>
LINQ_set<T,L,A>	LINQ(const std::set<T,L,A>& list)
{
	return LINQ_set<T,L,A>(list);
}




//////////////////////////////////////////////////////////////////////////
//	Map

template<typename K, typename V, typename PR, typename A>
struct LINQ_map : LINQSequence<LINQ_map<K, V, PR, A>, const std::pair<const K,V>&>
{
	typedef const std::pair<const K,V>& ResultType;

	const std::map<K, V, PR, A>& map;
	typename std::map<K, V, PR, A>::const_iterator it;

	LINQ_map(const std::map<K, V, PR, A>& map) : map(map)
	{
		it = map.begin();
	}

	//	Contract for LINQSequence
	bool IsEmpty() const { return it == map.end(); }
	void operator++() { ++it; }
	ResultType operator*() const { return *it; }
};

template<typename K, typename V, typename PR, typename A>
LINQ_map<K, V, PR, A>	LINQ(const std::map<K, V, PR, A>& map)
{
	return LINQ_map<K, V, PR, A>(map);
}



//////////////////////////////////////////////////////////////////////////
//	Built-in Array

template<typename T, size_t N>
struct LINQ_array : LINQSequence<LINQ_array<T, N>, const T&>
{
	typedef const T& ResultType;

	const T (&arr)[N];
	size_t idx = 0;

	LINQ_array(const T (&arr)[N])
		: arr(arr)
	{
	}

	//	Contract for LINQSequence
	bool IsEmpty() const { return idx == N; }
	void operator++() { ++idx; }
	ResultType operator*() const { return arr[idx]; }
};


template<typename T, size_t N>
LINQ_array<T, N>	LINQ(const T (&arr)[N])
{
	return LINQ_array<T, N>(arr);
}


//////////////////////////////////////////////////////////////////////////
//	Select 1st & 2nd wrappers

namespace alg { namespace Details
{
template<typename CONT, typename IT> struct Select1stIt;
template<typename CONT, typename IT> struct Select2ndIt;
template<typename CONT, typename MEM_PTR, typename VALUE_TYPE> struct SelectMemberIt;
}}

template<typename CONT, typename IT>
struct LINQ_Select2nd : LINQSequence<LINQ_Select2nd<CONT, IT>, typename const alg::Details::Select2ndIt<CONT, IT>::value_type&>
{
	typedef typename const alg::Details::Select2ndIt<CONT, IT>::value_type& ResultType;

	const alg::Details::Select2ndIt<CONT, IT>& cont;
	typename alg::Details::Select2ndIt<CONT, IT>::const_iterator it;

	LINQ_Select2nd(const alg::Details::Select2ndIt<CONT, IT>& cont)
		: cont(cont), it(cont.begin())
	{}

	//	Contract for LINQSequence
	bool IsEmpty() const { return it == cont.end(); }
	void operator++() { ++it; }
	ResultType operator*() const { return *it; }
};


template<typename CONT, typename IT>
auto LINQ(const alg::Details::Select2ndIt<CONT, IT>& cont)
{
	return LINQ_Select2nd<CONT, IT>(cont);
}


template<typename CONT, typename IT>
struct LINQ_Select1st : LINQSequence<LINQ_Select1st<CONT, IT>, typename const alg::Details::Select1stIt<CONT, IT>::value_type&>
{
	typedef typename const alg::Details::Select1stIt<CONT, IT>::value_type& ResultType;

	const alg::Details::Select1stIt<CONT, IT>& cont;
	typename alg::Details::Select1stIt<CONT, IT>::const_iterator it;

	LINQ_Select1st(const alg::Details::Select1stIt<CONT, IT>& cont)
		: cont(cont), it(cont.begin())
	{}

	//	Contract for LINQSequence
	bool IsEmpty() const { return it == cont.end(); }
	void operator++() { ++it; }
	ResultType operator*() const { return *it; }
};


template<typename CONT, typename IT>
auto LINQ(const alg::Details::Select1stIt<CONT, IT>& cont)
{
	return LINQ_Select1st<CONT, IT>(cont);
}



template<typename CONT, typename MEM_PTR, typename VALUE_TYPE>
struct LINQ_SelectMemberIt : LINQSequence<LINQ_SelectMemberIt<CONT, MEM_PTR, VALUE_TYPE>, typename const alg::Details::SelectMemberIt<CONT, MEM_PTR, VALUE_TYPE>::value_type&>
{
	typedef typename const alg::Details::SelectMemberIt<CONT, MEM_PTR, VALUE_TYPE>::value_type& ResultType;

	const alg::Details::SelectMemberIt<CONT, MEM_PTR, VALUE_TYPE>& cont;
	typename alg::Details::SelectMemberIt<CONT, MEM_PTR, VALUE_TYPE>::const_iterator it;

	LINQ_SelectMemberIt(const alg::Details::SelectMemberIt<CONT, MEM_PTR, VALUE_TYPE>& cont)
		: cont(cont), it(cont.begin())
	{}

	//	Contract for LINQSequence
	bool IsEmpty() const { return it == cont.end(); }
	void operator++() { ++it; }
	ResultType operator*() const { return *it; }
};


template<typename CONT, typename MEM_PTR, typename VALUE_TYPE>
LINQ_SelectMemberIt<CONT, MEM_PTR, VALUE_TYPE>	LINQ(const alg::Details::SelectMemberIt<CONT, MEM_PTR, VALUE_TYPE>& cont)
{
	return LINQ_SelectMemberIt<CONT, MEM_PTR, VALUE_TYPE>(cont);
}


