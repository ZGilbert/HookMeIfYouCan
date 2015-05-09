//
//  MemoryRegion.cpp
//  MemoryRegion
//
//  Created by BlueCocoa on 15/4/3.
//  Copyright (c) 2015年 0xBBC. All rights reserved.
//

#include "MemoryRegion.h"

#pragma mark
#pragma mark - Public Functions

MemoryRegion::MemoryRegion(ssize_t total_size) : size(total_size)
{
    this->memory = (void **)::malloc(total_size);
    if (this->memory == NULL) throw "System malloc failed!\n";
    
#if MemoryRegion_LOG_LEVEL >= MRLL_INFO
    printf("Memory region start at %p\n",this->memory);
#endif
    
    /*!
     *  @brief  随机一个Hash值用于校验
     */
    this->hash = arc4random();
    
#if MemoryRegion_LOG_LEVEL >= MRLL_INFO
    printf("Memory region has a hash value %X\n",this->hash);
#endif
    
    /*!
     *  @brief  mmgr初始化
     */
    this->mmgr = (struct mem_control_blk *)(this->memory);
    this->mmgr->size = 0;
    this->mmgr->pre = NULL;
    this->mmgr->next = NULL;
    this->mmgr->memory_start = NULL;
    this->mmgr->isUsing = true;
    this->mmgr->mcl_code = this->hash + 0xBBC;
    
    /*!
     *  @brief  因为我们使用了mmgr作为这个内存区域的链表头
     */
    this->used = sizeof(struct mem_control_blk);
}

void * MemoryRegion::malloc(const ssize_t request, FL_FIND_MEMORY type)
{
    struct mem_control_blk * mem = this->mmgr;
    struct mem_control_blk * mem_pre = this->mmgr;
    
    /*!
     *  @brief  请求比最大可提供容量还大, 放弃
     *
     *  @discussion 即使你请求的大小等于在初始化memory region对象时传递的大小, 这也是不可能的
     *              因为我们需要在mcl这个链表里记录分配情况
     *
     */
    if (request >= this->size - 2 * sizeof(struct mem_control_blk)) return NULL;
    
    /*!
     *  @brief  没有直接发现足够的空余空间
     */
    if (request > (this->size - this->used  - sizeof(struct mem_control_blk)))
    {
        
#if MemoryRegion_LOG_LEVEL >= MRLL_MORE
        printf("No enough spare space.\n");
#endif
        
        if (type == FAST)
        {
#pragma mark
#pragma mark - FAST
            void * fast_find = this->fast_find(request);
            if (fast_find) return fast_find;
            else
            {
                /*!
                 *  @brief  可能没有足够的空余空间, 现在先尝试合并
                 */
                this->merge();
                fast_find = this->fast_find(request);
                if (fast_find) return fast_find;
            }
            
            /*!
             *  @brief  所有的块都在使用中, 没有足够空间再分配
             */
            return NULL;
        }
        else
        {
#pragma mark
#pragma mark - BEST_FIT
            void * best_fit = this->best_fit_find(request);
            if (best_fit) return best_fit;
            else
            {
                /*!
                 *  @brief  可能没有足够的空余空间, 现在先尝试合并
                 */
                this->merge();
                best_fit = this->best_fit_find(request);
                if (best_fit) return best_fit;
            }
            
            /*!
             *  @brief  所有的块都在使用中, 没有足够空间再分配
             */
            return NULL;
        }
    }
    else
    {
#pragma mark
#pragma mark - Spare Space
        
        /*!
         *  @brief  当有明显的空余空间时, 我们总能分配最合适的块
         */
        while (mem != NULL)
        {
            mem_pre = mem;
            mem = mem->next;
        }
        mem = (struct mem_control_blk *)(this->memory + this->used);
        mem->size = request;
        mem->pre = mem_pre;
        mem->isUsing = true;
        mem->next = NULL;
        mem->memory_start = (this->memory + this->used + sizeof(struct mem_control_blk));
        mem->mcl_code = this->hash + 0xBBC;
        mem_pre->next = mem;
    }
    
#if MemoryRegion_LOG_LEVEL >= MRLL_INFO
    printf("Memory block start at %p\n",mem);
    printf("User's memory start at %p\n",mem->memory_start);
#endif
    
    this->used += sizeof(struct mem_control_blk) + request;
    return mem->memory_start;
}

void MemoryRegion::free(void ** memory_start)
{
#if MemoryRegion_LOG_LEVEL >= MRLL_MORE
    printf("Get memory_start at %p\n",*memory_start);
#endif
    
    void ** p = (void **)(*memory_start);
    struct mem_control_blk * mem = (struct mem_control_blk *)(p - sizeof(struct mem_control_blk));
    
#if MemoryRegion_LOG_LEVEL >= MRLL_MORE
    printf("Get mem block at %p\n",mem);
#endif
    
    if (mem->mcl_code == 0xBBC + this->hash)
    {
        
#if MemoryRegion_LOG_LEVEL >= MRLL_VERBOSE
        printf("  This memory block is vaild and its length is %zd\n",mem->size);
        printf("  This memory block is %s\n",(mem->isUsing ? "using. Now free." : "NOT using!"));
#endif
        
        if (mem->isUsing)
        {
            mem->isUsing = false;
            ::memset(mem->memory_start, 0, mem->size);
            *memory_start = NULL;
        }
    }
    else
    {
        throw "This memory block should not been freed with this object.\n";
    }
}

ssize_t MemoryRegion::available()
{
    ssize_t avai = this->size - this->used;
    struct mem_control_blk * mem = this->mmgr;
    while (mem != NULL)
    {
        if (!mem->isUsing) avai += mem->size;
        mem = mem->next;
    }
    return avai;
}

MemoryRegion::~MemoryRegion()
{
    ::free(this->memory);
}

#pragma mark
#pragma mark - Private Functions

void * MemoryRegion::fast_find(ssize_t request)
{
    struct mem_control_blk * mem = this->mmgr;
    
#if MemoryRegion_LOG_LEVEL >= MRLL_VERBOSE
    printf("  Start FAST TYPE looping!\n");
#endif
    
    while (mem != NULL)
    {
        
#if MemoryRegion_LOG_LEVEL >= MRLL_VERBOSE
        printf("   Found a block %p, using : %d\n    size : %zd\n",mem,mem->isUsing,mem->size);
#endif
        
        if (!mem->isUsing && mem->size >= request)
        {
            
#if MemoryRegion_LOG_LEVEL >= MRLL_VERBOSE
            printf("    Found! size is %zd\n",mem->size);
#endif
            
            if (mem->size == request)
            {
                /*!
                 *  @brief  这个块正好合适
                 */
                mem->isUsing = true;
                return mem->memory_start;
            }
            else
            {
                if (mem->size - request <= (sizeof(struct mem_control_blk) + 1))
                {
                    /*!
                     *  @brief  对于这个块, 我们已经不可能把它分为两部分了
                     *          虽然会比申请的空间大一些, 但这毕竟是最快查找
                     */
                    mem->isUsing = true;
                    return mem->memory_start;
                }
                else
                {
                    /*!
                     *  @brief  我们得把这个内存块分为两部分
                     */
                    this->split_mem_control_blk(mem, request);
                    return mem->memory_start;
                }
            }
        }
        mem = mem->next;
    }
    /*!
     *  @brief  所有的块都在使用中或者没有空余空间
     */
    return NULL;
}

void * MemoryRegion::best_fit_find(ssize_t request)
{
    struct mem_control_blk * mem = this->mmgr;
    
#if MemoryRegion_LOG_LEVEL >= MRLL_VERBOSE
    printf("  Start BEST_FIT TYPE looping!\n");
#endif
    
    ssize_t diff = LONG_MAX;
    struct mem_control_blk * best_fit = NULL;
    while (mem != NULL)
    {
        if (!mem->isUsing && mem->size >= request)
        {
            
#if MemoryRegion_LOG_LEVEL >= MRLL_VERBOSE
            printf("   Found a block %p, using : %d\n    size : %zd\n",mem,mem->isUsing,mem->size);
#endif
            
            if (mem->size - request < diff)
            {
                diff = mem->size - request;
                best_fit = mem;
            }
        }
        mem = mem->next;
    }
    
    if (best_fit)
    {
        if (best_fit->size - request <= (sizeof(struct mem_control_blk) + 1))
        {
            /*!
             *  @brief  对于这个块, 我们已经不可能把它分为两部分了
             *          虽然会比申请的空间大一些, 但这已经是最接近申请的大小的块了
             */
            best_fit->isUsing = true;
            return best_fit->memory_start;
        }
        else
        {
            /*!
             *  @brief  我们得把这个内存块分为两部分
             */
            this->split_mem_control_blk(best_fit, request);
            return best_fit->memory_start;
        }
    }
    else
    {
        /*!
         *  @brief  所有的块都在使用中或者没有空余空间
         */
        return NULL;
    }
}

void MemoryRegion::split_mem_control_blk(struct mem_control_blk * mem, ssize_t size)
{
#if MemoryRegion_LOG_LEVEL >= MRLL_VERBOSE
    printf("   Split %zd into %zd and %zd\n",mem->size,size,mem->size - size - sizeof(struct mem_control_blk));
#endif
    
    struct mem_control_blk * second_blk = (struct mem_control_blk *)(mem->memory_start + size);
    second_blk->memory_start = (mem->memory_start + size + sizeof(struct mem_control_blk));
    second_blk->size = (mem->size - size - sizeof(struct mem_control_blk));
    second_blk->next = mem->next;
    if (mem->next) mem->next->pre = second_blk;
    mem->next = second_blk;
    second_blk->pre = mem;
    second_blk->isUsing = false;
    second_blk->mcl_code = 0xBBC + this->hash;
    mem->isUsing = true;
    mem->size = size;
}

void MemoryRegion::merge()
{
    struct mem_control_blk * mem = this->mmgr;
    struct mem_control_blk * mem_start = NULL;
    while (mem != NULL)
    {
        if (!mem->isUsing)
        {
            /*!
             *  @brief  记住起始的不在使用的块
             */
            if (mem_start == NULL) mem_start = mem;
        }
        else
        {
            /*!
             *  @brief  可以合并
             */
            if (mem_start && mem->pre != mem_start)
            {
                ssize_t spare = 0;
                struct mem_control_blk * mem_iter = mem;
                while (mem_iter != mem_start)
                {
                    spare += mem_iter->size + sizeof(struct mem_control_blk);
                    mem_iter = mem_iter->pre;
                }
                
#if MemoryRegion_LOG_LEVEL >= MRLL_VERBOSE
                printf("Merged %zd size\n",spare);
#endif
                
                mem_start->next = mem;
                mem->pre = mem_start;
                mem_start->size = spare - sizeof(struct mem_control_blk);
                ::memset(mem_start->memory_start, 0, mem_start->size);
            }
            mem_start = NULL;
        }
        mem = mem->next;
    }
}
