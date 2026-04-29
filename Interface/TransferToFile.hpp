#pragma once
#include <spdlog/spdlog.h>
#include <fstream>
// 换行逻辑函数外进行处理
// ”,“逻辑函数内处理
// 日志输出函数外处理
template <typename... Args>
inline void writeCSVRow(std::ofstream& out, Args... args) {
	//spdlog::info("Writing a row to CSV with {} columns", sizeof...(args));
    bool first = true;
    auto print_arg = [&](const auto& val) {
        if (!first) {
            out << ",";
        }
        out << val;
        first = false;
        };

    // 折叠表达式：对 args 参数包中的每一个元素调用 print_arg
    (print_arg(args), ...);
}