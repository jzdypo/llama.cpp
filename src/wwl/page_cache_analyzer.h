#pragma once

#include <string>
#include <vector>

/**
 * @brief 表示一个连续的页面缓存区间。
 */
struct PageCacheRange {
    size_t start_page; // 区间的起始页码 (包含)
    size_t end_page;   // 区间的结束页码 (包含)
    bool is_cached;    // 该区间的状态 (true: 已缓存, false: 未缓存)
};

/**
 * @brief 使用 mincore 系统调用检查文件的 Page Cache 命中情况。
 * 
 * @param file_path 要检查的文件的路径。
 */
std::vector<PageCacheRange> get_model_page_cache_ranges(const std::string& file_path);
/**
 * @brief 根据单纯的模型文件名，从预设的 './models/' 目录加载并检查其 Page Cache 状态。
 *        这是一个辅助函数，会自动拼接路径并调用 get_model_page_cache_ranges。
 *        它会校验 model_name 是否包含路径分隔符。
 * 
 * @param model_name 模型的文件名 (不含路径和扩展名)。
 * @return std::vector<PageCacheRange> 描述文件缓存状态的区间列表。
 */
std::vector<PageCacheRange> get_model_page_cache_ranges_by_name(const std::string& model_name);