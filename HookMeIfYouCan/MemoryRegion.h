//
//  MemoryRegion.h
//  MemoryRegion
//
//  Created by BlueCocoa on 15/4/3.
//  Copyright (c) 2015年 0xBBC. All rights reserved.
//

#ifndef __MemoryRegion__
#define __MemoryRegion__

#include <stdio.h>
#include <iostream>

#define MRLL_NO_LOG  1
#define MRLL_INFO    2
#define MRLL_MORE    3
#define MRLL_VERBOSE 4

#ifndef MemoryRegion_LOG_LEVEL
#define MemoryRegion_LOG_LEVEL NO_LOG
#endif

typedef enum
{
    /*!
     *  @brief  找出最合适的块
     */
    BEST_FIT,
    
    /*!
     *  @brief  找到第一个可以容纳所请求内存大小的块即停止
     */
    FAST
} FL_FIND_MEMORY;

class MemoryRegion
{
public:
    /*!
     *  @brief  初始化
     *
     *  @param  这个内存区域的总大小
     *
     *  @return 这个内存区域对象
     */
    MemoryRegion (ssize_t total_size);
    
    /*!
     *  @brief  请求一个内存块
     *
     *  @param  request 所请求的内存块的大小
     *
     *  @param  type    以何种方式搜索这个内存块
     *
     *  @return 新的内存块, 当可用空间不够时, 返回NULL
     */
    void * malloc(const ssize_t request, FL_FIND_MEMORY type = FAST);
    
    /*!
     *  @brief  释放这个内存块
     *
     *  @param memory_start 这个内存块的引用
     *
     */
    void free(void ** memory_start);
    
    /*!
     *  @brief  总的剩余可用空间
     */
    ssize_t available();
    
    /*!
     *  @brief  析枸函数
     */
    ~MemoryRegion();
    
private:
    /*!
     *  @brief  记录这个内存区域的大小
     */
    const ssize_t size;
    
    /*!
     *  @brief  已经分配的空间大小(包括已经被free的块)
     */
    ssize_t used;
    
    /*!
     *  @brief  从系统那儿申请到的一整块内存
     */
    void ** memory;
    
    /*!
     *  @brief  这个memory region对象的哈希值
     */
    int hash;
    
    /*!
     *  @brief  定义内存块链表
     */
    typedef struct mem_control_blk
    {
        /*!
         *  @brief  记录这个内存块是否正在使用
         */
        bool isUsing;
        
        /*!
         *  @brief  这个内存块分配给用户的大小
         */
        ssize_t size;
        
        /*!
         *  @brief  前一个内存块指针
         */
        struct mem_control_blk * pre;
        
        /*!
         *  @brief  后一个内存块指针
         */
        struct mem_control_blk * next;
        
        /*!
         *  @brief  这个内存块的哈希值
         */
        int mcl_code;
        
        /*!
         *  @brief  返回给用户的内存指针
         */
        void ** memory_start;
    } mem_control_blk;
    
    /*!
     *  @brief  这个memory region的内存块链表起始点
     */
    struct mem_control_blk * mmgr;
    
    /*!
     *  @brief  快速搜索可用块
     *
     *  @param  request 所请求的内存大小
     *
     *  @return 返回给用户的内存指针
     */
    void * fast_find(ssize_t request);
    
    /*!
     *  @brief  寻找最合适的空余块
     *
     *
     *  @param  request 所请求的内存大小
     *
     *  @return 返回给用户的内存指针
     */
    void * best_fit_find(ssize_t request);
    
    /*!
     *  @brief  将一个内存块分为两个
     *
     *  @param  将要被分割的内存块
     *
     *  @param  size 第一个内存块保留的大小, 第二个内存块的大小将被自动计算
     *
     */
    void split_mem_control_blk(struct mem_control_blk *, ssize_t size);
    
    /*!
     *  @brief  尝试合并不在使用中的内存块
     */
    void merge();
};

#endif /* defined(__MemoryRegion__) */
