#pragma once

#define BOOST_RESULT_OF_USE_DECLTYPE
#define BOOST_NO_EXCEPTIONS
#define BOOST_AUTO_LINK_TAGGED

#include <cstdio>
#include <cstdint>

#include <iostream>
#include <iomanip>
#include <string>
#include <set>
#include <map>
using namespace std;

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <boost/lexical_cast.hpp>
using boost::lexical_cast;

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <boost/algorithm/string.hpp>

extern const string LETTER_FOLDER;
