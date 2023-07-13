// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

struct {
  struct spinlock buflock;
  struct spinlock biglock;
  struct spinlock bucketlock[NBUCKET];
  struct buf buf[NBUF];
  struct buf bucket[NBUCKET];
  int usedbuf;
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  //struct buf head;
} bcache;

int hash(int blockno)
{
  return blockno % NBUCKET;
}

void
binit(void)
{
  struct buf *b;
  initlock(&bcache.buflock,"bcache_buf");
  initlock(&bcache.biglock, "bcache_outer");
  for(int i=0;i<NBUCKET;i++)
  {
    initlock(&bcache.bucketlock[i],"bcache_inner");
  }
  for(int i=0;i<NBUF;i++)
  {
    initsleeplock(&bcache.buf[i].lock, "buffer");
  }
  bcache.usedbuf = 0;

  /*
  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }*/
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.

static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int id = hash(blockno);
  struct buf *minb=0, *minpre;
  uint mintimestamp = __INT32_MAX__;


  acquire(&bcache.bucketlock[id]);
  for(b = &(bcache.bucket[id]);b!=0;b=b->next)
  {
    if(b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release(&bcache.bucketlock[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  

  acquire(&bcache.buflock);
  if(bcache.usedbuf<NBUF)
  {
    b = &(bcache.buf[bcache.usedbuf]);
    bcache.usedbuf++;
    release(&bcache.buflock);
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    //Insert
    b->next = bcache.bucket[id].next;
    bcache.bucket[id].next = b;
    release(&bcache.bucketlock[id]);
    acquiresleep(&b->lock);
    return b;
  }
  release(&bcache.buflock);
  release(&bcache.bucketlock[id]);

  acquire(&bcache.biglock);
  //寻找当前bucket
  for(struct buf* pre = &bcache.bucket[id];pre->next!=0;pre=pre->next)
  {
    b=pre->next;
    if(b->dev==dev&&b->blockno==blockno)
    {
      b->refcnt++;
      release(&bcache.bucketlock[id]);
      release(&bcache.biglock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  //当前bucket无，寻找其他bucket
  for(int temp=hash(id+1);temp != id;temp = hash(temp+1))
  {
    mintimestamp=__INT32_MAX__;
    acquire(&bcache.bucketlock[id]);
    for(struct buf* pre = &bcache.bucket[id];pre->next!=0;pre=pre->next)
    {
      b=pre->next;
      if(b->refcnt==0 && b->timestamp<mintimestamp)
      {
        minb=b;
        minpre=pre;
        mintimestamp=b->timestamp;
      }
      if(minb)
      {
        minb->dev=dev;
        minb->blockno=blockno;
        minb->valid=0;
        minb->refcnt=1;
        if(id!=hash(blockno))
        {
          minpre->next = minb->next;
          release(&bcache.bucketlock[id]);
          id = HASH(blockno);  // the correct bucket index
          acquire(&bcache.bucketlock[id]);
          minb->next = bcache.bucket[id].next;    // move block to correct bucket
          bcache.bucket[id].next = minb;
        }
        release(&bcache.bucketlock[id]);
        release(&bcache.biglock);
        acquiresleep(&minb->lock);
        return minb;
      }
    }
  }

  
  
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int id = hash(b->blockno);
  acquire(&bcache.bucketlock[id]);
  b->refcnt--;
  if (b->refcnt == 0) {
    /*
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
    */
    b->timestamp = ticks;
  }
  
  release(&bcache.bucketlock[id]);
}

void
bpin(struct buf *b) {
  int id = hash(b->blockno);
  acquire(&bcache.bucketlock[id]);
  b->refcnt++;
  release(&bcache.bucketlock[id]);
}

void
bunpin(struct buf *b) {
  int id = hash(b->blockno);
  acquire(&bcache.bucketlock[id]);
  b->refcnt--;
  release(&bcache.bucketlock[id]);
}


