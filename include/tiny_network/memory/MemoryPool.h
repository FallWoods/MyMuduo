#pragma once

constexpr int kPageSize = 1024;
constexpr int kMpAlignment = 16;

#define mp_align(n, alignment) (((n)+(alignment-1)) & ~(alignment-1))
#define mp_align_ptr(p, alignment) (void *)((((size_t)p)+(alignment-1)) & ~(alignment-1))

struct SmallNode {
    unsigned char* end_;    // 该块的结尾
    unsigned char* last_;   // 该块目前使用的位置
    unsigned int quote_;    // 该块被引用次数
    unsigned int failed_;   // 该块查找失败次数
    SmallNode* next_;       // 指向下一个块
};

struct LargeNode {
    void* address_;     // 该块的起始地址
    unsigned int size_; // 该块大小
    LargeNode* next_;   // 指向下一个大块
};

struct Pool {
    // 分配大块内存
    LargeNode* largeList_;  // 管理大块内存链表
    
    // 分配小块内存
    SmallNode* head_;       // 小块内存链表的头节点
    SmallNode* current_;    // 指向当前可以进行分配的块，这样可以避免遍历前面已经不能分配的块
};

class MemoryPool {
public:
    /**
     * @brief 默认构造，真正初始化工作交给 createPool
     */
    MemoryPool() = default;

    /**
     * @brief 默认析构，真正销毁工作交给 destroyPool
     */
    ~MemoryPool() = default;

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    /**
     * @brief 初始化内存池，为 pool_ 分配 PAGE_SIZE 内存
     */
    void createPool();

    /**
     * @brief 销毁内存池，遍历大块内存和小块内存且释放它们
     */
    void destroyPool();

    /**
     * @brief 申请内存的接口，内部会判断创建大块还是小块内存
     * @param[in] size 分配内存大小
     */
    void* malloc(unsigned long size);

    /**
     * @brief 申请内存且将内存清零，内部调用 malloc
     * @param[in] size 分配内存大小
     */ 
    void* calloc(unsigned long size);

    /**
     * @brief 释放指定内存
     * @param[in] p 释放内存头地址
     */
    void freeMemory(void* p);

    /**
     * @brief 重置内存池
     */  
    void resetPool();

    Pool* getPool() { return pool_; }
private:
    /**
     * @brief 分配大块节点，被 malloc 调用
     * @param[in] size 分配内存大小
     */
    void* mallocLargeNode(unsigned int size);

    /**
     * @brief 分配小块节点，被 malloc 调用
     * @param[in] size 分配内存大小
     */
    void* mallocSmallNode(unsigned long size);

    Pool* pool_ = nullptr;
};