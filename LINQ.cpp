#include "LINQ.h"
#include <iostream>
#include <vector>
#include <string>


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
	 

	void LINQTest()
	{
		for( auto val :
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
