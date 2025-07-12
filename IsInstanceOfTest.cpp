#include "IsInstanceOf.h"

#include <vector>
#include <tuple>
#include <string>

static_assert( std::true_type() == is_instance_of<std::vector, std::vector<int>>() );
static_assert( std::false_type() == is_instance_of<std::vector, std::tuple<int>>() );
static_assert( std::false_type() == is_instance_of<std::vector, std::tuple<>>());

static_assert( std::false_type() == is_instance_of<std::tuple, std::vector<int>>() );
static_assert( std::true_type() == is_instance_of<std::tuple, std::tuple<int, float, std::string>>() );

static_assert( std::true_type() == is_instance_of<std::basic_string, std::string>() );
