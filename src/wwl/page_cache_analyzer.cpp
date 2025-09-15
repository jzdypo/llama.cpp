#include "page_cache_analyzer.h"

// 平台兼容性处理
#if defined(__unix__) || defined(__APPLE__)

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <iostream>
#include <chrono>

// 根据文件路径获取该文件 Page Cache 情况
std::vector<PageCacheRange> get_model_page_cache_ranges(const std::string& file_path) {
    // char cwd_buffer[100]; // 创建一个足够大的缓冲区
    // if (getcwd(cwd_buffer, sizeof(cwd_buffer)) != NULL) {
    //     std::cout << "[Debug] Current Working Directory: " << cwd_buffer << std::endl;
    // } else {
    //     perror("[Debug] Error getting current path with getcwd");
    // }
    auto t_start = std::chrono::high_resolution_clock::now(); // 开始计时
    // 1. 打开文件并获取信息 (与之前相同)
    int fd = open(file_path.c_str(), O_RDONLY);
    if (fd == -1) { /* 错误处理 */ return {}; }
    
    struct stat file_stats;
    if (fstat(fd, &file_stats) == -1) { /* 错误处理 */ close(fd); return {}; }
    size_t file_size = file_stats.st_size;

    void* mapped_addr = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
    if (mapped_addr == MAP_FAILED) { /* 错误处理 */ close(fd); return {}; }

    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) { page_size = 4096; }
    size_t num_pages = (file_size + page_size - 1) / page_size;
    
    std::vector<unsigned char> page_info(num_pages);
    if (mincore(mapped_addr, file_size, page_info.data()) != 0) {
        perror("[Page Cache] Error: mincore failed");
        munmap(mapped_addr, file_size);
        close(fd);
        return {};
    }

    // --- 核心算法：将页面状态整理成连续区间 ---
    std::vector<PageCacheRange> ranges;
    if (num_pages == 0) {
        // 清理并返回空列表
        munmap(mapped_addr, file_size);
        close(fd);
        return ranges;
    }

    // 从第一个页面开始
    size_t current_range_start = 0;
    bool is_current_range_cached = (page_info[0] & 1);

    for (size_t i = 1; i < num_pages; ++i) {
        bool page_is_cached = (page_info[i] & 1);
        // 如果当前页面的状态与正在记录的区间状态不同，则一个区间结束了
        if (page_is_cached != is_current_range_cached) {
            // 将上一个完整的区间添加到列表中
            ranges.push_back({current_range_start, i - 1, is_current_range_cached});
            // 开始一个新的区间
            current_range_start = i;
            is_current_range_cached = page_is_cached;
        }
    }

    // 循环结束后，将最后一个区间（一直到文件末尾）添加进去
    ranges.push_back({current_range_start, num_pages - 1, is_current_range_cached});
    
    // 清理资源
    munmap(mapped_addr, file_size);
    close(fd);
    auto t_end = std::chrono::high_resolution_clock::now(); // 结束计时
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count(); // 计算毫秒差值
    std::cout << "         - get pagecache condition call took: " << duration_ms << " ms" << std::endl;
    return ranges; // 返回整理好的区间列表
}

// 根据模型名字从指定文件夹检查 Page Cache 状态
std::vector<PageCacheRange> get_model_page_cache_ranges_by_name(const std::string& model_name){
    if (model_name.find('/') != std::string::npos) {
        // 为了在 POSIX 系统上更健壮，最好也检查反斜杠 '\'，尽管它在 Linux/macOS 上是合法文件名的一部分
        if (model_name.find('\\') != std::string::npos) {
            std::cerr << "[Page Cache] Error: model_name '" << model_name << "' should not contain path separators ('/' or '\\')." << std::endl;
            return {}; // 返回空列表表示失败
        }
    }
    std::string model_path = "./models/" + model_name; // 假设模型文件在 models 目录下
    return get_model_page_cache_ranges(model_path);
}
#else

#include <iostream>
#include <vector>

// 在非 POSIX 系统上，返回一个空列表
std::vector<PageCacheRange> get_model_page_cache_ranges(const std::string& file_path) {
    (void)file_path;
    std::cout << "\n[System] Page Cache status check (mincore) is not available on this system." << std::endl;
    return {}; // 返回空 vector
}

#endif