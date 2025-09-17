#include "../include/utils/data_get.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>

using namespace std;

namespace Electromagnetic_compatibility {
namespace utils {
unordered_map<string, pair<int,int>> read_file(const string& filename,int skip_lines) {
    unordered_map<string, pair<int,int>> data;
    ifstream file(filename);
    if(!file.is_open()){
        cout<<"文件打开失败"<<endl;
        return data;
    }
    string line;
    // 跳过指定行数
    for (int i = 0; i < skip_lines; ++i) {
        if (!getline(file, line)) {
            cerr << "警告：文件行数不足，无法跳过所有指定行。" << endl;
            return data; // 如果文件行数不足，也返回
        }
    }

    // 定义正则表达式来匹配 "[整数,整数]" 格式
    std::regex pattern(R"(\[\s*(-?\d+)\s*,\s*(-?\d+)\s*\])"); // 匹配负数，允许空格

    int i = 0; // Moved declaration outside the loop
    while (std::getline(file, line)) {
        std::smatch matches;
        if (std::regex_match(line, matches, pattern)) {
            // matches[0] 是整个匹配的字符串
            // matches[1] 是第一个捕获组（key）
            // matches[2] 是第二个捕获组（value）
            int key = std::stoi(matches[1].str());
            int value = std::stoi(matches[2].str());
            string str = "ship"+to_string(i);
            data[str] = make_pair(key,value);
            i++;
        } else {
            std::cerr << "警告：跳过格式不正确的行： " << line << std::endl;
        }
    }
    return data;

}
}
}
