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
#include <limits.h>

#define NBUCKET 13

struct busket{
	struct spinlock lock;
	struct buf head;
};

struct {
  //struct spinlock lock; 
	struct buf buf[NBUF];
  struct busket hashtable[NBUCKET];
} bcache;

void
binit(void)
{
  //initlock(&bcache.lock, "bcache");

	// init busket
	char lockname[18];
	for(int i = 0; i < NBUCKET; i++){
		snprintf(lockname,18,"bcache.busket %d",i);
		initlock(&bcache.hashtable[i].lock,lockname);
		bcache.hashtable[i].head.prev = &bcache.hashtable[i].head;
		bcache.hashtable[i].head.next = &bcache.hashtable[i].head;
	}

	// all buffers are allocated to hashtable[0] at first
  struct buf *b;
	for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.hashtable[0].head.next;
    b->prev = &bcache.hashtable[0].head;
    initsleeplock(&b->lock, "buffer");
    bcache.hashtable[0].head.next->prev = b;
    bcache.hashtable[0].head.next = b;
	}
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
	struct buf* b;

	int busketno = blockno % NBUCKET;
	acquire(&bcache.hashtable[busketno].lock);

	// Is the block already cached?
	for(b = bcache.hashtable[busketno].head.next; b != &bcache.hashtable[busketno].head; b = b->next){
		if(b->dev == dev && b->blockno == blockno){
			b->refcnt++;
			release(&bcache.hashtable[busketno].lock);
			acquiresleep(&b->lock);
			return b;
		}
	}

	// Not cached.
	// Recycle the least recently used (LRU) unused buffer.
	for(int i=0;i<NBUCKET;i++){
		int bno = (busketno+i) % NBUCKET;
		if(bno != busketno){
			if(bcache.hashtable[bno].lock.locked == 0)
				acquire(&bcache.hashtable[bno].lock);
			else 
				continue;
		}
		
		// find available buffer
		b =  &bcache.hashtable[bno].head;
		struct buf* temp;
		for(temp = bcache.hashtable[bno].head.next; temp != &bcache.hashtable[bno].head; temp = temp->next){
			if(temp->refcnt == 0 && (b == &bcache.hashtable[bno].head || temp->ticks < b->ticks)){
				b = temp;
			}
		}

		// if find buffer in busket[bno]
		if(b != &bcache.hashtable[bno].head){
			if(bno != busketno){
					b->next->prev = b->prev;
					b->prev->next = b->next;
					release(&bcache.hashtable[bno].lock);
					b->next = bcache.hashtable[busketno].head.next;
					b->prev = &bcache.hashtable[busketno].head;
					bcache.hashtable[busketno].head.next->prev = b;
					bcache.hashtable[busketno].head.next = b;
			}

			b->dev = dev;
			b->blockno = blockno;
			b->valid = 0;
			b->refcnt = 1;

			release(&bcache.hashtable[busketno].lock);
			acquiresleep(&b->lock);
			return b;

		}
		else{
			if(bno != busketno)
					release(&bcache.hashtable[bno].lock);
		}
	}

	panic("bget: no buffer");
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

	int busketno = b->blockno % NBUCKET;
	acquire(&bcache.hashtable[busketno].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
		acquire(&tickslock);
		b->ticks = ticks;
		release(&tickslock);
	}
	release(&bcache.hashtable[busketno].lock);

}

void
bpin(struct buf *b) {
	int busketno = b->blockno % NBUCKET;
  acquire(&bcache.hashtable[busketno].lock);
  b->refcnt++;
  release(&bcache.hashtable[busketno].lock);
}

void
bunpin(struct buf *b) {
	int busketno = b->blockno % NBUCKET;
  acquire(&bcache.hashtable[busketno].lock);
  b->refcnt--;
  release(&bcache.hashtable[busketno].lock);
}


