#include "LINQ.h"
#include <iostream>
#include <vector>
#include <string>
#include "STLHelpers.h"


struct ZZ
{
	std::vector<std::string>	list;
	volatile size_t storez;

	ZZ()
	{
		list.push_back("A");
		list.push_back("SD");
		list.push_back("DF");
		list.push_back("DFG");

		LINQTest();
	}
	
	void Select1St()
	{
		//	compilation check
		std::map<int, int> d;
		alg::select1st(d);
 		alg::select1st(LINQ(d));
		LINQ(alg::select1st(d));
		alg::select2nd(d);
		alg::select2nd(LINQ(d));
		LINQ(alg::select2nd(d));

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
			storez = val;
		}

		for( auto val : LINQRange(1,10))
			storez = val;
	}

};


static void LINQTest()
{
	ZZ zz;
}
