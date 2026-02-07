#pragma once
#include <vector>
#include <string>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <fstream>

template <typename... Args>
inline void writeCSVRow(std::ofstream& out, Args... args) {
	spdlog::info("Writing a row to CSV with {} columns", sizeof...(args));
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

    // 每一行结束后换行
    out << "\n";
}