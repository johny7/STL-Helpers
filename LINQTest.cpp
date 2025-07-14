#include "LINQ.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <format>
#include "STLHelpers.h"

using namespace linq;

std::string print_container(const auto& cont)
{
	std::stringstream s;
	s << "[ ";
	for(auto& val : cont)
	{
		s << val;
		s << " ";
	}

	s << "]";

	return s.str();
}

void assert_eq(const auto&& a, const auto&& b)
{
	auto it1 = begin(a);
	auto it2 = begin(b);

	while(true)
	{
		bool it1_finished = it1 == end(a);
		bool it2_finished = it2 == end(b);
		if(it1_finished && it2_finished)
			break;

		if(it1_finished != it2_finished || *it1 != *it2)
			throw std::runtime_error( std::format("LINQ tests: Expected containers to be equal {} == {}", print_container(a), print_container(b)) );

		++it1, ++it2;
	}
}

struct LINQTests
{
	std::vector<std::string>	list;
	size_t storez;

	LINQTests()
	{
		list.push_back("A");
		list.push_back("SD");
		list.push_back("DF");
		list.push_back("DFG");
		list.push_back("DG");

		LINQTest();
		Selects();
	}
	
	void Selects()
	{
		//	compilation check
		std::map<int, int> d { {2,4}, {3,5} };
		assert_eq(LINQ(alg::select1st(d)), std::vector { 2,3 });
		assert_eq(LINQ(alg::select2nd(d)), std::vector { 4,5 });
		assert_eq(LINQ(alg::select1st(d)).ToVector(), std::vector { 2,3 });
		assert_eq(LINQ(alg::select2nd(d)).ToVector(), std::vector { 4,5 });
		//assert_eq(alg::select1st(LINQ(d)), std::vector {2,3});
		//assert_eq(alg::select2nd(LINQ(d)), std::vector {4,5});

		struct Test
		{
			int field;
		};

		std::vector<Test> v;
		alg::select_member(v, &Test::field);
		alg::select_member(LINQ(v), &Test::field);
		LINQ(alg::select_member(v, &Test::field));
	}

	void LINQTest()
	{
		for( size_t val :
			LINQ( list )
			.Select( []( const std::string& val ) { return val.length(); } )
			.Where( [](size_t val )	{ return val == 2; } )
			.Skip(1)
			)
		{
			std::cout << val << "\n";
		}

		for( auto val : LINQRange(1,10))
			std::cout << val << " ";
	}

};


void LINQTest()
{
	LINQTests tests;
}
