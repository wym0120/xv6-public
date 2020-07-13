#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"

// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

typedef long Align;

union header {
    struct {
        union header *ptr;
        uint size;
    } s;
    Align x;//不会起任何实际的对齐效果，只是一个hint
};

typedef union header Header;

static Header base;
static Header *freep;//指向上一次最后被找到的空闲块

void
free(void *ap)
{
    Header *bp, *p;

    bp = (Header*)ap - 1;
    for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr) //这段for循环是试图找到两个空闲块使得bp的位置置于两者之间
        if(p >= p->s.ptr && (bp > p || bp < p->s.ptr)) //freep是按照地址从低到高排序好的，那么当出现了p >= p->str的情况就说明已经到达边界了
            break;
    if(bp + bp->s.size == p->s.ptr){//如果正好能合并就合并
        bp->s.size += p->s.ptr->s.size;
        bp->s.ptr = p->s.ptr->s.ptr;
    } else
        bp->s.ptr = p->s.ptr;//即使在p->s.ptr = p，也就是初始情况下，依旧能维持一个循环链表
    if(p + p->s.size == bp){//如果正好能合并就合并
        p->s.size += bp->s.size;
        p->s.ptr = bp->s.ptr;
    } else
        p->s.ptr = bp;
    freep = p;
}

static Header*
morecore(uint nu)
{
    char *p;
    Header *hp;

    if(nu < 4096)
        nu = 4096;
    p = sbrk(nu * sizeof(Header));//为了减少请求次数，在请求nu大小的内存时多申请几倍
    if(p == (char*)-1)
        return 0;
    hp = (Header*)p;
    hp->s.size = nu;
    free((void*)(hp + 1));
    return freep;
}

void*
malloc(uint nbytes)
{
    Header *p, *prevp;
    uint nunits;

    nunits = (nbytes + sizeof(Header) - 1)/sizeof(Header) + 1;//为了对齐
    if((prevp = freep) == 0){//初始化freep，构建一个循环链表，并且第一次向系统申请内存
        base.s.ptr = freep = prevp = &base;
        base.s.size = 0;
    }
    for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr){
        if(p->s.size >= nunits){//first fit strategy
            if(p->s.size == nunits)
                prevp->s.ptr = p->s.ptr;//如果找到的空闲块恰好满足要求，就直接把下一块接到上一块的末尾
            else {
                p->s.size -= nunits;//否则，新增一块更小的空闲块
                p += p->s.size;
                p->s.size = nunits;
            }
            freep = prevp;//改变freep指向的位置，这是为了让链表更加均匀
            return (void*)(p + 1);//在这里确定了header和data是在内存中连续的两个地址
        }
        if(p == freep)
            if((p = morecore(nunits)) == 0)
                return 0;
    }
}
