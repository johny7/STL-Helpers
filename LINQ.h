#ifndef LINQ_H_INCLUDED
#define LINQ_H_INCLUDED

#include <vector>
#include <set>
#include "types/macroses.h"

#if defined _DEBUG && !defined NDEBUG
#define NDEBUG
#endif


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
	ResultType operator*() { return functor(*seq); }
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
	ResultType operator*() { return *seq; }
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
	ResultType operator*() { return *seq; }
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
	ResultType operator*() { return *seq; }
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
	ResultType operator*() { return LINQIdFilter<SeqT, F>(seq, key, idExtractor); }
};



template<typename ParentT, typename ResultType>
struct LINQSequence
{
	//	Interface contract: every descendant class should implement these methods:
//	 bool IsEmpty() const;
//	 void operator++();
//	 ResultType operator*();


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


	std::vector<ResultType> ToList()
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
		RefType(MyType& me) : me(me)
		{}

		bool operator==(const RefType& ) const { return static_cast<ParentT&>(me).IsEmpty(); }
		bool operator!=(const RefType& ) const { return !static_cast<ParentT&>(me).IsEmpty(); }
		void operator++() { ++static_cast<ParentT&>(me); }
		ResultType operator*() { return *static_cast<ParentT&>(me); }

		MyType& me;
	};

	//	for each friendly
	RefType begin() { return RefType(*this); }
	RefType end() { return RefType(*this); }
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
	ResultType operator*() { return cur; }
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
	ResultType operator*() { return *it; }
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
	ResultType operator*() { return *it; }
};


template<typename T, typename L, typename A>
LINQ_set<T,L,A>	LINQ(const std::set<T,L,A>& list)
{
	return LINQ_set<T,L,A>(list);
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
	ResultType operator*() { return arr[idx]; }
};


template<typename T, size_t N>
LINQ_array<T, N>	LINQ(const T (&arr)[N])
{
	return LINQ_array<T, N>(arr);
}

#endif	// LINQ_H_INCLUDED

