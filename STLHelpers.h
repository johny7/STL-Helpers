#pragma once


#include <algorithm>
#include <limits>
#include "boost/utility/value_init.hpp"
#include "boost/type_traits/is_const.hpp"
#include "boost/type_traits/remove_reference.hpp"
#include "boost/type_traits/remove_cv.hpp"
#include <string>
//#include "algo/rnd.h"

#undef min
#undef max


//
//	Algos:
//
//	foreach adapters, for map/multimap, which allows to iterate only over
//	select1st(container)	//.. keys
//	select2nd(container)	//.. or values
//
//
//	clamp value into min/max
//	T clamp(T value, U min, V max)
//
//  if value is away from allowed min,max - the default is returned
//	T ClampToDefault(T value, U min, V max, T default)
//  
//
//	clamp value into [0-1]
//	T saturate(T value)
//
//	scale in [0-1]
//	T lerp(T from, U to, V scale)
//
//	stretches value [from-to] to [0-1],
//	T scale(T value, U from, V to)
//
//	min for list of args
//	T min(T val1, U val2, ...)
//
//	max for list of args
//	T max(T val1, U val2, ...)
//
//	rounding to nearest integer
//	int round(double val)
//
//	quick pow into integer power
//	T pow(T val, int power)
//
//	removes elements from a container, returns number of deleted
//	size_t remove(container, value)
//
//	removes elements from a container by a predicate F, returns number of deleted
//	size_t remove_if(container, F)
//
//	removes all indexes, moving content and keeping order
//	void remove_indexes(container, indexes)
//	
//	removes element by index, returns true if such index existed
//	bool erase(container, size_t index)
//
//	returns index of object in the array, or size() if not found
//	size_t object_id(container, value)
//
//	taking element in container, ASSERT if not found
//	value& map_get(container, key)
//
//	taking element in container, if not found - returns ref to default
//	value& map_get_def(container, key, default)
//
//
//	does such element exist in container?
//	bool is_exist(container, value)
//
//	does conforming to predicate element exist in container?
//	bool if_exist(container, functor)
//
//
//	for_each over container..
//	functor for_each(container, functor)
//
//
//	get iterator on element, or end()
//	iterator find(container, key)
//
//	get iterator on element, or ASSERT
//	iterator find_a(container, key)
//
//	get iterator on first predicate matching, or end()
//	iterator find_if(container, functor)
//
//	get iterator on first predicate matching, or ASSERT
//	iterator find_if_a(container, functor)
//
//
//	insert element into container, true if success
//	bool insert(container, value)
//
//
//	copy all from one container to another, returns 'true' if anything was copied
//	( ex. copy(select2nd(some_map), some_array) )
//	bool copy(src_container, dst_container)
//
//	copy all by predicate from one container to another, returns 'true' if anything was copied
//	bool copy_if(src_container, dst_container, predicate)
//
//
//	move all from one container to another, returns 'true' if anything was moved
//	( ex. move(select2nd(some_map), some_array) )
//	bool move(src_container, dst_container)
//
//
//	standard algo wrappers
//	void sort(container, functor = operator<)
//	bool binary_search(container, val)
//	iterator upper_bound(container, val)
//	iterator lower_bound(container, val)
//
//	shuffle
//	void shuffle(container)
//	
//  count number of elements == element
//	size_t count(container, element)
//
//	count number of elements conforming to predicate
//	size_t count_if(container, functor)
//
//	sum up all elements
//	T sum(container)
//
//	sum up all elements, preprocessed by functor
//	T sum(container, functor)
//
//	std::advance
//	IT advance(iterator, shift)
//
//	
//	void reverse(container)
//
//	function returns positive key value in map, that is not exists
//	K  new_map_id(const std::map<K, T>& m)
//	
//

namespace std
{
template<typename T, typename A> class vector;
template<typename T, typename A> class list;
template<typename K, typename P, typename A> class set;
template<typename K, typename P, typename A> class multiset;
template<typename K, typename V, typename P, typename A> class map;
template<typename K, typename V, typename P, typename A> class multimap;
};


namespace alg
{
template<typename T, typename U, typename V>
T clamp( T value, U min, V max )
{
    return std::min( std::max( value, (T)min ), (T)max );
}

template<typename T>
inline T ClampToDefault( T val, T min, T max, T default_ )
{
    return
        val < min ? default_ :
        val > max ? default_ :
        val;
}

template<typename T>
T saturate( T value )
{
    return std::min( std::max( value, (T)0 ), (T)1 );
}

template<typename T, typename U, typename V>
T lerp( T from, U to, V scale )
{
    CT_ASSERT( is_floating_point<V>::result );
    //		DASSERT(scale >= 0 && scale <= 1);

    return T( from * (1 - scale) + to * scale );
}

template<typename T, typename U, typename V>
T scale( T value, U from, V to )
{
    ASSERT( to != from );

    return T( (value - from) / (to - from) );
}

template<typename T, typename U>
T min( T arg1, U arg2 )
{
    return (arg1 < arg2) ? arg1 : arg2;
}

template<typename T, typename U, typename V>
T min( T arg1, U arg2, V arg3 )
{
    return (arg1 < arg2) ? (arg1 < arg3 ? arg1 : arg3) : (arg2 < arg3 ? arg2 : arg3);
}

template<typename T, typename U, typename V, typename W>
T min( T arg1, U arg2, V arg3, W arg4 )
{
    return min( arg1, arg2, arg3 ) < arg4 ? min( arg1, arg2, arg3 ) : arg4;
}

template<typename T, typename U>
T max( T arg1, U arg2 )
{
    return (arg1 > arg2) ? arg1 : arg2;
}

template<typename T, typename U, typename V>
T max( T arg1, U arg2, V arg3 )
{
    return (arg1 > arg2) ? (arg1 > arg3 ? arg1 : arg3) : (arg2 > arg3 ? arg2 : arg3);
}

template<typename T, typename U, typename V, typename W>
T max( T arg1, U arg2, V arg3, W arg4 )
{
    return max( arg1, arg2, arg3 ) > arg4 ? max( arg1, arg2, arg3 ) : arg4;
}

inline int round( double val )
{
    return (int)floor( val + 0.5 );
}


namespace Details
{
template<bool even, unsigned ORDER, typename T>
struct pow_branch
{
    static T f( T val )
    {
        //	false branch
        return val * alg::pow<ORDER - 1, T>( val );
    }
};

template<unsigned ORDER, typename T>
struct pow_branch<true, ORDER, T>
{
    static T f( T val )
    {
        T v = alg::pow<ORDER / 2, T>( val );

        return v*v;
    }
};

template<typename T>
struct pow_branch<false, 1, T>
{
    static T f( T val ) { return val; }
};
template<typename T>
struct pow_branch<true, 0, T>
{
    static T f( T val ) { return 1; }
};
}

//  some compile time pleasure to break pow<5>(x) into x * ((x^2)^2)
template<unsigned ORDER, typename T>
T pow( T val )
{
    enum { even = ((ORDER / 2) * 2 == ORDER) };

    return Details::pow_branch<even, ORDER, T>::f( val );
}




template<typename CONT>
size_t remove( CONT& cont, const typename CONT::value_type& value )
{
    typename CONT::iterator trash = std::remove( cont.begin(), cont.end(), value );
    const size_t value_number = cont.end() - trash;

    if( value_number )
        cont.erase( trash, cont.end() );

    return value_number;
}

template<typename T, typename A, typename VALUE>
size_t remove( std::list<T, A>& cont, const VALUE& value )
{
    size_t ret_val = 0;
    typename std::list<T, A>::iterator  obj_it = cont.begin();

    while( true )
    {
        obj_it = std::find( obj_it, cont.end(), value );
        const bool value_existed = obj_it != cont.end();

        if( value_existed )
            ++ret_val, obj_it = cont.erase( obj_it );
        else
            break;
    }

    return ret_val;
}

template<typename T>
size_t remove( std::basic_string<T>&  str, const T* value )
{
    size_t  amount = 0;

    while( 1 )
    {
        size_t pos = str.find( value );

        if( pos == std::basic_string<T>::npos )
            break;

        str.erase( pos, letter_traits<T>::strlen( value ) );
        ++amount;
    }

    return amount;
}

template<typename CONT, typename F>
size_t remove_if( CONT& cont, F func )
{
    typename CONT::iterator trash = std::remove_if( cont.begin(), cont.end(), func );
    const size_t value_number = unsigned( cont.end() - trash );

    if( value_number )
        cont.erase( trash, cont.end() );

    return value_number;
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
bool erase( CONT& cont, size_t index )
{
    if( index >= cont.size() )
        return false;

    typename CONT::iterator  p = cont.begin();
    std::advance( p, index );
    cont.erase( p );

    return true;
}


//	returns size() if there is no element
template<typename T, typename A>
size_t object_id( const std::vector<T, A>& cont, const T& value )
{
    return std::find( cont.begin(), cont.end(), value ) - cont.begin();
}


template<typename K, typename V, typename P, typename A>
const V& map_get( const std::map<K, V, P, A>& cont, const K& key )
{
    ASSERT( is_exist( cont, key ) );
    return cont.find( key )->second;
}
template<typename K, typename V, typename P, typename A>
V& map_get( std::map<K, V, P, A>& cont, const K& key )
{
    ASSERT( is_exist( cont, key ) );
    return cont.find( key )->second;
}


template<typename K, typename V, typename P, typename A>
const V&  map_get_def( const std::map<K, V, P, A>& cont, const K& key, const V& default_value = V() )
{
    typename std::map<K, V, P, A>::const_iterator  p = cont.find( key );
    if( p == cont.end() )
        return default_value;

    return p->second;
}
//	& specialization for 'nullptr'
template<typename K, typename V, typename P, typename A>
const V&  map_get_def( const std::map<K, V, P, A>& cont, const K& key, void* ptr )
{
    return map_get_def( cont, key, (V)ptr );
}

/*	template<typename K, typename V, typename P, typename A, typename D>
V  map_get_def(std::map<K, V, P, A>& cont, const K& key, const D&  default_value = V())
{
typename std::map<K, V, P, A>::iterator  p = cont.find(key);
if(p == cont.end())
return (V)default_value;

return p->second;
}
*/

template<typename CONT, typename T>
bool is_exist( const CONT& cont, const T& value )
{
    return std::find( cont.begin(), cont.end(), value ) != cont.end();
}

template<typename K, typename V, typename P, typename A, typename T>
bool is_exist( const std::map<K, V, P, A>& cont, const T& value )
{
    return cont.find( value ) != cont.end();
}

template<typename K, typename V, typename P, typename A, typename T>
bool is_exist( const std::multimap<K, V, P, A>& cont, const T& value )
{
    return cont.find( value ) != cont.end();
}

template<typename K, typename P, typename A, typename T>
bool is_exist( const std::set<K, P, A>& cont, const T& value )
{
    return cont.find( value ) != cont.end();
}

template<typename K, typename P, typename A, typename T>
bool is_exist( const std::multiset<K, P, A>& cont, const T& value )
{
    return cont.find( value ) != cont.end();
}

template<typename CONT, typename FUNC>
bool if_exist( const CONT& cont, FUNC func )
{
    return find_if( cont, func ) != cont.end();
}



template<typename CONTAINER, typename FUNC>
FUNC  for_each( CONTAINER& cont, FUNC func )
{
    return std::for_each( cont.begin(), cont.end(), func );
}
template<typename CONTAINER, typename FUNC>
FUNC  for_each( const CONTAINER& cont, FUNC func )
{
    return std::for_each( cont.begin(), cont.end(), func );
}


template<typename CONTAINER, typename OBJ_T>
typename CONTAINER::iterator  find( CONTAINER& cont, const OBJ_T& obj )
{
    return std::find( cont.begin(), cont.end(), obj );
}
template<typename CONTAINER, typename OBJ_T>
typename CONTAINER::const_iterator  find( const CONTAINER& cont, const OBJ_T& obj )
{
    return std::find( cont.begin(), cont.end(), obj );
}
template<typename K, typename V, typename A, typename C, typename T>
typename std::map<K, V, A, C>::iterator  find( std::map<K, V, A, C>& cont, const T& key )
{
    return cont.find( key );
}
template<typename K, typename V, typename A, typename C, typename T>
typename std::map<K, V, A, C>::const_iterator  find( const std::map<K, V, A, C>& cont, const T& key )
{
    return cont.find( key );
}

template<typename CONTAINER, typename OBJ_T>
typename CONTAINER::iterator  find_a( CONTAINER& cont, const OBJ_T& obj )
{
    typename CONTAINER::iterator p = find( cont.begin(), cont.end(), obj );
    ASSERT( p != cont.end() );

    return p;
}
template<typename CONTAINER, typename OBJ_T>
typename CONTAINER::const_iterator  find_a( const CONTAINER& cont, const OBJ_T& obj )
{
    typename CONTAINER::const_iterator p = find( cont, obj );
    ASSERT( p != cont.end() );

    return p;
}


template<typename CONTAINER, typename FUNC>
typename CONTAINER::iterator  find_if( CONTAINER& cont, FUNC func )
{
    return std::find_if( cont.begin(), cont.end(), func );
}
template<typename CONTAINER, typename FUNC>
typename CONTAINER::const_iterator  find_if( const CONTAINER& cont, FUNC func )
{
    return std::find_if( cont.begin(), cont.end(), func );
}


template<typename CONTAINER, typename FUNC>
typename CONTAINER::iterator  find_if_a( CONTAINER& cont, FUNC func )
{
    typename CONTAINER::iterator p = find_if( cont, func );
    ASSERT( p != cont.end() );

    return p;
}
template<typename CONTAINER, typename FUNC>
typename CONTAINER::const_iterator  find_if_a( const CONTAINER& cont, FUNC func )
{
    typename CONTAINER::const_iterator p = find_if( cont, func );
    ASSERT( p != cont.end() );

    return p;
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
    return cont.insert( std::forward<T>( value ) ).second;
}

template<typename K, typename V, typename P, typename A, typename V1>
bool insert( std::multimap<K, V, P, A>& cont, V1&& value )
{
    cont.insert( std::forward<T>( value ) );
    return true;
}

template<typename K, typename P, typename A, typename V1>
bool insert( std::set<K, P, A>& cont, V1&& value )
{
    return cont.insert( std::forward<T>( value ) ).second;
}

template<typename K, typename P, typename A, typename V1>
bool insert( std::multiset<K, P, A>& cont, V1&& value )
{
    cont.insert( std::forward<T>( value ) );
    return true;
}


template<typename SRC_CONTAINER, typename DST_CONTAINER>
bool copy( const SRC_CONTAINER& src, DST_CONTAINER& dst )
{
	return copy_if( src, dst, [] (auto& ) { return true; } );
}

template<typename SRC_CONTAINER, typename DST_CONTAINER, typename FUNC>
bool copy_if( const SRC_CONTAINER& src, DST_CONTAINER& dst, FUNC predicate )
{
    bool ret_val = false;

    for( typename SRC_CONTAINER::const_iterator p = src.begin(); p != src.end(); ++p )
        if( predicate( *p ) )
            ret_val = true, insert( dst, *p );

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


template<typename SRC_CONTAINER, typename FUNC>
void sort( SRC_CONTAINER& container, FUNC functor )
{
    return std::sort( container.begin(), container.end(), functor );
}
template<typename SRC_CONTAINER>
void sort( SRC_CONTAINER& container )
{
    return std::sort( container.begin(), container.end() );
}
template<typename SRC_CONTAINER, typename T>
bool binary_search( SRC_CONTAINER& container, const T& val )
{
	return std::binary_search( begin(container), end(container), val );
}
template<typename SRC_CONTAINER, typename T>
typename SRC_CONTAINER::iterator upper_bound( SRC_CONTAINER& container, const T& val )
{
	return std::upper_bound( begin( container ), end( container ), val );
}

template<typename SRC_CONTAINER, typename T>
typename SRC_CONTAINER::iterator lower_bound( SRC_CONTAINER& container, const T& val )
{
	return std::lower_bound( begin( container ), end( container ), val );
}


template<typename SRC_CONTAINER>
void shuffle( SRC_CONTAINER& container )
{
    return std::random_shuffle( container.begin(), container.end() );
}

template<typename SRC_CONTAINER, typename OBJ>
size_t count( const SRC_CONTAINER& container, const OBJ& element )
{
    size_t  result = 0;

    for( typename SRC_CONTAINER::const_iterator p = container.begin(); p != container.end(); ++p )
        if( *p == element )
            ++result;

    return result;
}

template<typename SRC_CONTAINER, typename FUNCTOR>
size_t count_if( const SRC_CONTAINER& container, FUNCTOR functor )
{
    size_t  result = 0;

    for( typename SRC_CONTAINER::const_iterator p = container.begin(); p != container.end(); ++p )
        if( functor( *p ) )
            ++result;

    return result;
}

template<typename SRC_CONTAINER>
typename SRC_CONTAINER::value_type  sum( const SRC_CONTAINER& container )
{
    boost::value_initialized<typename SRC_CONTAINER::value_type>  sum;

    for( typename SRC_CONTAINER::const_iterator p = container.begin(); p != container.end(); ++p )
        sum.data() += *p;

    return sum;
}

template<typename SRC_CONTAINER, typename FUNCTOR>
typename boost::remove_cv< typename boost::remove_reference<typename FUNCTOR::result_type>::type >::type
sum( const SRC_CONTAINER& container, FUNCTOR functor )
{
    typedef typename boost::remove_cv< typename boost::remove_reference<typename FUNCTOR::result_type>::type >::type  result_type;
    boost::value_initialized<result_type>  sum;

    for( typename SRC_CONTAINER::const_iterator p = container.begin(); p != container.end(); ++p )
        sum.data() += functor( *p );

    return sum;
}



namespace Details
{
template<typename CONT>
struct Select1stIt
{
    CONT& cont;

    Select1stIt( CONT& cont ) : cont( cont ) {}

    typedef typename const CONT::key_type  value_type;	//	keys are always const
    typedef typename CONT::const_iterator  cont_iterator_type;
    //			typedef typename if_type< boost::is_const<CONT>::value, const typename CONT::key_type, typename CONT::key_type>::result  value_type;
    //			typedef typename if_type< boost::is_const<CONT>::value, typename CONT::const_iterator, typename CONT::iterator>::result  cont_iterator_type;

    struct iterator : public std::iterator
        <std::forward_iterator_tag, value_type>
    {
        typename cont_iterator_type  it;

        //				typedef typename cont_iterator_type::difference_type  difference_type;
        typedef value_type*  pointer;
        typedef value_type&  reference;

        iterator( cont_iterator_type it ) : it( it ) {}

        bool operator==( const iterator& other ) const { return it == other.it; }
        bool operator!=( const iterator& other ) const { return it != other.it; }
        iterator& operator++() { ++it; return *this; }
        iterator& operator++( int ) { it++; return *this; }
        iterator& operator--() { --it; return *this; }
        iterator& operator--( int ) { it--; return *this; }
        reference  operator*() const { return it->first; }
        pointer   operator->() const { return &it->first; }
		bool	  operator<( const iterator& other ) { return it < other.it; }

		cont_iterator_type base() { return it; }
    };

    typedef iterator  const_iterator;

    iterator begin() const
    {
        return iterator( cont.begin() );
    }
    iterator end() const
    {
        return iterator( cont.end() );
    }
};

template<typename CONT>
struct Select2ndIt
{
    CONT& cont;

    Select2ndIt( CONT& cont ) : cont( cont ) {}

    typedef typename std::conditional< boost::is_const<CONT>::value, const typename CONT::value_type::second_type, typename CONT::value_type::second_type>::type value_type;
    typedef typename std::conditional< boost::is_const<CONT>::value, typename CONT::const_iterator, typename CONT::iterator>::type  cont_iterator_type;

    struct iterator : public std::iterator
        <std::forward_iterator_tag, value_type>
    {
        cont_iterator_type  it;

        //				typedef typename CONT::iterator::difference_type  difference_type;
        typedef value_type*  pointer;
        typedef value_type&  reference;

        iterator( cont_iterator_type it ) : it( it ) {}

        bool operator==( const iterator& other ) const { return it == other.it; }
        bool operator!=( const iterator& other ) const { return it != other.it; }
        iterator& operator++() { ++it; return *this; }
        iterator& operator++( int ) { it++; return *this; }
        iterator& operator--() { --it; return *this; }
        iterator& operator--( int ) { it--; return *this; }
        reference  operator*() const { return it->second; }
        pointer   operator->() const { return &it->second; }
		bool	  operator<( const iterator& other ) { return it < other.it; }

		cont_iterator_type base() { return it; }
	};

    typedef iterator  const_iterator;

    iterator begin() const
    {
        return iterator( cont.begin() );
    }
    iterator end() const
    {
        return iterator( cont.end() );
    }
};

template<typename CONT, typename MEM_PTR, typename VALUE_TYPE>
struct SelectMemberIt
{
	CONT& cont;
	MEM_PTR ptr;

	SelectMemberIt( CONT& cont, MEM_PTR ptr ) : cont( cont ), ptr(ptr) {}

	typedef typename std::conditional< boost::is_const<CONT>::value, const VALUE_TYPE, VALUE_TYPE>::type value_type;
	typedef typename std::conditional< boost::is_const<CONT>::value, typename CONT::const_iterator, typename CONT::iterator>::type  cont_iterator_type;

	struct iterator : public std::iterator
		<std::forward_iterator_tag, value_type>
	{
		cont_iterator_type  it;
		MEM_PTR ptr;

		//				typedef typename CONT::iterator::difference_type  difference_type;
		typedef value_type*  pointer;
		typedef value_type&  reference;

		iterator( cont_iterator_type it, MEM_PTR ptr ) : it( it ), ptr ( ptr ) {}

		bool operator==( const iterator& other ) const { return it == other.it; }
		bool operator!=( const iterator& other ) const { return it != other.it; }
		iterator& operator++() { ++it; return *this; }
		iterator& operator++( int ) { it++; return *this; }
		iterator& operator--() { --it; return *this; }
		iterator& operator--( int ) { it--; return *this; }
		reference  operator*() const { return (*it).*ptr; }
		pointer   operator->() const { return &(*it).*ptr; }
		bool	  operator<(const iterator& other) { return it < other.it; }

		cont_iterator_type base() { return it; }
	};

	typedef iterator  const_iterator;

	iterator begin() const
	{
		return iterator( cont.begin(), ptr );
	}
	iterator end() const
	{
		return iterator( cont.end(), ptr );
	}
};

}//namespace Details

template<typename CONT>
Details::Select1stIt<CONT> select1st( CONT& cont ) { return Details::Select1stIt<CONT>( cont ); }
template<typename CONT>
Details::Select2ndIt<CONT> select2nd( CONT& cont ) { return Details::Select2ndIt<CONT>( cont ); }
template<typename CONT, typename MEM_PTR>
auto select_member( CONT& cont, MEM_PTR ptr ) -> Details::SelectMemberIt<CONT, MEM_PTR, std::remove_reference_t<decltype(std::declval<typename CONT::value_type>().*ptr)> >
{
	typedef std::remove_reference_t<decltype(std::declval<typename CONT::value_type>().*ptr)>  value_type;

	return Details::SelectMemberIt<CONT, MEM_PTR, value_type>( cont, ptr );
}


template<typename IT, typename DIFF>
IT  advance( IT  it, DIFF  diff )
{
    std::advance( it, diff );

    return it;
}

template<typename V, typename A>
void  reverse( std::vector<V, A>&  cont )
{
    std::reverse( cont.begin(), cont.end() );
}

template<typename V, typename A>
void  reverse( std::list<V, A>&  cont )
{
    std::reverse( cont.begin(), cont.end() );
}

/*

//	function returns positive key value in map, that does not yet exist
template<typename CONT>
typename CONT::key_type  new_map_id( const CONT& m )
{
    typedef typename boost::remove_cv<CONT::key_type>::type  K;

    if( m.empty() )
        return 0;

    K last_id = m.rbegin()->first;

    if( last_id < std::numeric_limits<K>::max() )
        return last_id + 1;

    Rnd  rnd;
    K i = std::numeric_limits<K>::max();

    while( i-- )
    {
        K id = rnd.rnd() % (unsigned)std::numeric_limits<K>::max();
        if( m.find( id ) == m.end() )
            return id;
    }

    ASSERT( false );
    return 0;
}

*/

};	//	namespace

