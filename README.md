# STL-Helpers
More extended support for functional programming in C++. C++ implementation of LINQ

Usage samples:
```
//  LINQ
std::vector<std::string> v;
for( size_t val :
	LINQ(v)
	.Select([](const std::string& val) { return val.length(); })
	.Where([](size_t val) { return val == 2; })
	.Skip(1)
) { .. }


//  select2nd
std::map<int, Star> stars;
for( const Star& star : alg::select2nd(stars) )
{ .. }
```

There are more content, which allows to you work with containers as a first class member of the language. Forget begin(), end().
This is Windows version. There are known compilation issues in Linux, fixes are simple but are not yet made it here.
