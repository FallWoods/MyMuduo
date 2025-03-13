#include "MemoryPool.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

void MemoryPool::createPool() {
    /**
     * int posix_memalign (void **memptr, size_t alignment, size_t size);
     * posix_memalign分配的内存块更大，malloc可能分配不了太大的内存块
     * 内部要让这个p指针指向新的内存，所以需要使用二级指针。如果只是返回一个指针，可以使
     */

    int ret = posix_memalign((void**)&pool_, kMpAlignment, kPageSize);

    if (ret) {
        printf("posix_memalign failed\n");
        return;
    }

    // 分配 PAGE_SIZE 内存：Pool + SmallNode + 剩余可用内存
    pool_->largeList_ = nullptr;
    pool_->head_ = reinterpret_cast<SmallNode*>((unsigned char*)pool_ + sizeof(Pool));
    pool_->head_->last_ = (unsigned char*)pool_ + sizeof(Pool) +  sizeof(SmallNode);
    pool_->head_->end_ = (unsigned char*)pool_ + kPageSize;
    pool_->head_->failed_ = pool_->head_->quote_ = 0;
    pool_->current_ = pool_->head_;
    printf("分配的内存池的地址：%p\n",pool_);
    return;
}

void MemoryPool::destroyPool() {
    LargeNode* large = pool_->largeList_;
    while(large) {
        if (large->address_) {
            printf("释放的大块内存地址: %p\n",large->address_);
            free(large->address_);
            large->address_ = nullptr;
            // printf("释放完成\n");
        }
        large = large->next_;
    }

    SmallNode* cur = pool_->head_->next_;
    SmallNode* next = nullptr;
    while(cur != nullptr) {
        next = cur->next_;
        printf("销毁的内存池的地址：%p\n",cur);
        free(cur);
        cur = next;
    }
    printf("销毁的内存池的地址：%p\n",pool_);
    free(pool_);
}

// 分配大块内存
void* MemoryPool::mallocLargeNode(unsigned int size) {
    unsigned char* addr;
    int ret = posix_memalign((void**)&addr, kMpAlignment, size);

    if (ret) {
        return nullptr;
    }
    int count =0 ;
    LargeNode* largenode = pool_->largeList_;
    while (largenode) {
        if (!largenode->address_) {
            largenode->address_ = addr;
            largenode->size_ = size;
            printf("分配的大块内存地址：%p\n",addr);
            return addr;
        }
        if (++count > 4) {
            // 为了避免过多的遍历，限制次数
            break;
        }
        largenode = largenode->next_;
    }
    // 没有找到空闲的large结构体，分配一个新的large
    // 比如第一次分配large的时候
    largenode = (LargeNode*)this->malloc(sizeof(LargeNode));
    if (!largenode) {
        // 申请节点内存失败，需要释放之前申请的大内存
        free(addr);
        return nullptr;
    }
    largenode->address_ = addr; // 设置新块地址
    largenode->size_ = size;    // 设置新块大小
    largenode->next_ = pool_->largeList_;
    pool_->largeList_ = largenode;
    printf("分配的大块内存地址：%p\n",addr);
    return addr;
}

// 创建一个用来分配小内存的块，顺便分配一次内存
void* MemoryPool::mallocSmallNode(unsigned long size) {
    unsigned char* block;
    int ret = posix_memalign((void**)(&block), kMpAlignment, kPageSize);
    if (ret) {
        return nullptr;
    }

    // 获取新块的smallnode节点
    SmallNode* smallnode = (SmallNode*)block;
    smallnode->next_ = nullptr;
    smallnode->end_ = block + kPageSize;
    smallnode->quote_ = 0;
    smallnode->failed_ = 0;

    // 分配新块的起始位置
    // 将要分配的内存地址对齐
    unsigned char* addr = (unsigned char*)mp_align_ptr(block + sizeof(SmallNode), kMpAlignment);
    smallnode->last_ = addr + size;
    smallnode->quote_++;

    // 重新设置current
    SmallNode* cur = pool_->current_;

    while(cur->next_ != nullptr) {
        if (cur->failed_++ > 4) {
            pool_->current_ = cur->next_;
        }
        cur = cur->next_;
    }
    cur->next_ = smallnode;
    printf("分配的内存池的地址：%p\n",addr);
    return addr;
}

void* MemoryPool::malloc(unsigned long size) {
    if (size == 0) {
        return nullptr;
    }

    // 申请大块内存
    if (size > (kPageSize - sizeof(Pool) - sizeof(SmallNode))) {
        return mallocLargeNode(size);
    }

    // 申请小块内存
    unsigned char* addr = nullptr;
    SmallNode* cur = pool_->current_;
    while(cur) {
        // 先把可用空间的首地址对齐
        addr = (unsigned char*)mp_align_ptr(cur->last_,kMpAlignment);
        // 判断是否有足够的空间
        if (cur->end_ - addr >= size) {
            // 该 block 被引用次数增加
            cur->quote_++;
            // 更新已使用位置
            cur->last_ = addr + size;
            return addr;
        }
        // 此block不够用，去下一个block
        cur = cur->next_;
    }
     // 说明已有的 block 都不够用，需要创建新的 block
    return mallocSmallNode(size);
}

// 分配初始化为0的内存
void* MemoryPool::calloc(unsigned long size) {
    void* addr = malloc(size);
    if (addr != nullptr) {
        memset(addr,0,sizeof addr);
    }
    return addr;
}

// 释放指定的大内存块
void MemoryPool::freeMemory(void* p) {
    LargeNode* large = pool_->largeList_;
    while(large) {
        if(large->address_ = p) {
            free(large->address_);
            large->size_ = 0;
            large = nullptr;
            return;
        }
        large = large->next_;
    }
}

void MemoryPool::resetPool() {
    SmallNode* small = pool_->head_;
    LargeNode* large = pool_->largeList_;
    // 释放大内存块
    while(large != nullptr) {
        if (large->address_ != nullptr) {
            printf("释放大内存块\n");
            free(large->address_);
            large->address_ = nullptr;
        }
        large = large->next_;
    }

    pool_->largeList_ = nullptr;

    printf("重置用于分配小内存的块\n");
    // 重置用于分配小内存的块
    while(small) {
        if ((unsigned char *)small == (unsigned char *)(pool_) + sizeof(Pool)) {
            small->last_ = (unsigned char *)small + sizeof(pool_) + sizeof(SmallNode);
            small->failed_ = small->quote_ = 0;
        } else {
            small->last_ = (unsigned char *)small + sizeof(SmallNode);
            small->failed_ = small->quote_ = 0;
        }
        small = small->next_;
    }
    // 所有块都为空了，让current_指向head_
    pool_->current_ = pool_->head_;
}
