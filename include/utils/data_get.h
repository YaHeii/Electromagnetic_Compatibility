#pragma once
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <string>
using namespace std;


namespace Electromagnetic_compatibility {
namespace utils {
unordered_map<string, pair<int,int>> read_file(const string& filename,int skip_lines);

// unordered_map<double,pair<double,double>> read();
}
}


