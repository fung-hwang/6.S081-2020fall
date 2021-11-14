# Locks

### Memory allocator
+ 给每个CPU设一个freelist和对应的lock
+ freelist竞争仅可能发生在从其他CPU的freelist中偷空块时，避免了全局一个freelist时多个CPU竞争的情况。但因为仍有竞争，所以需要在处理freelist时加锁，无论是使用所在CPU的freelist还是其他CPU的freelist。
+ 被这个简单的section折磨了很久的原因是漏看了hints，而且没能正确处理偷空块的过程。脑溢血直接犯了。警示自己务必谋而后动。

`kernel/kalloc.c`
```c
struct {
	struct spinlock lock;
	struct run *freelist;
} kmem[NCPU];

void
kinit()
{
	for (int i = 0; i < NCPU; i++) {
		char str[8];
		snprintf(str, 8, "%s %d", "kmem", i);
		initlock(&kmem[i].lock, str);
	}
	freerange(end, (void*)PHYSTOP);
}

void
kfree(void *pa)
{
	struct run *r;

	if (((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
		panic("kfree");

	// Fill with junk to catch dangling refs.
	memset(pa, 1, PGSIZE);

	r = (struct run*)pa;

	push_off();
	int id = cpuid();

	acquire(&kmem[id].lock);
	r->next = kmem[id].freelist;
	kmem[id].freelist = r;
	release(&kmem[id].lock);

	pop_off();
}

void *
kalloc(void)
{
	struct run *r;

	push_off();
	int id = cpuid();
	acquire(&kmem[id].lock);
	r = kmem[id].freelist;
	if (r)
		kmem[id].freelist = r->next;
	else {
        // steal
		for (int i = 1; i < NCPU; i++) {
			int t_id = (id + i) % NCPU;
            // 这里要避免t_id == id
            // 否则会重复acquire(lock)
			acquire(&kmem[t_id].lock);
			r = kmem[t_id].freelist;
			if (r) {
				kmem[t_id].freelist = r->next;
				release(&kmem[t_id].lock);
				break;  // has stole
			}
			release(&kmem[t_id].lock);
		}
	}
	release(&kmem[id].lock);

	pop_off();

	if (r)
		memset((char*)r, 5, PGSIZE); // fill with junk

	return (void*)r;
}
```

### Buffer cache
+ 整体思路是用散列表Hashtable的桶bucket使得并发的颗粒度更小，从bcache.lock降低为bucket.lock。
+ 此处的hashtable结构与lab7并发实验中的hashtable是相同的，可以作为借鉴；而从其他桶中窃取buffer的过程则可以参考本实验的Memory allocator。
+ 关于锁的获取和释放，容易出现问题的地方是在窃取buffer时，因为会要同时持有多个锁。没有使用整体锁bcache.lock的原因是在“持有-觊觎”时可以让步（想要的bucket锁被其他进程持有，那就换一个bucket）。如果既不让步，又没有整体锁控制获取第二个bucket锁的权限，就会发生死锁。当然，让步的策略在某些情况下还是会有问题的，不过测试程序没有涉及这种情况。
+ 写了一天一夜，只能说是我犯大病了。第一次对题目的理解整个错了，第二次反复被小错误折磨。有必要反思一下在处理稍复杂问题时的编码流程。

`kernel/buf.h`
```c
struct buf {
  // add ticks 时间戳
  uint ticks;
};
```

`kernel/bio.c`
```c
#define NBUCKET 13

struct busket {
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
	for (int i = 0; i < NBUCKET; i++) {
		snprintf(lockname, 18, "bcache.busket %d", i);
		initlock(&bcache.hashtable[i].lock, lockname);
		bcache.hashtable[i].head.prev = &bcache.hashtable[i].head;
		bcache.hashtable[i].head.next = &bcache.hashtable[i].head;
	}

	// all buffers are allocated to hashtable[0] at first
	struct buf *b;
	for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
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
	for (b = bcache.hashtable[busketno].head.next; b != &bcache.hashtable[busketno].head; b = b->next) {
		if (b->dev == dev && b->blockno == blockno) {
			b->refcnt++;
			release(&bcache.hashtable[busketno].lock);
			acquiresleep(&b->lock);
			return b;
		}
	}

	// Not cached.
	// Recycle the least recently used (LRU) unused buffer.
	for (int i = 0; i < NBUCKET; i++) {
		int bno = (busketno + i) % NBUCKET;
		// 若bno==busketno，已获得锁，故无动作
		// 若bno!=busketno，尝试获取hashtable[bno]的锁
		if (bno != busketno) {
			// 让步，避免死锁deadlock
			if (bcache.hashtable[bno].lock.locked == 0)
				acquire(&bcache.hashtable[bno].lock);
			else
				continue;
		}

		// find available buffer
		b = &bcache.hashtable[bno].head;
		struct buf* temp;
		for (temp = bcache.hashtable[bno].head.next; temp != &bcache.hashtable[bno].head; temp = temp->next) {
			if (temp->refcnt == 0 && (b == &bcache.hashtable[bno].head || temp->ticks < b->ticks)) {
				b = temp;
			}
		}

		// if find buffer in busket[bno]
		if (b != &bcache.hashtable[bno].head) {
			// 如果是窃取的buffer，将其放在自己bucket的链中
			if (bno != busketno) {
				b->next->prev = b->prev;
				b->prev->next = b->next;
				// 此时要放弃锁，因为不再涉及被窃取buffer的bucket
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
		else {
			// 没有找到可用buffer要释放lock
			if (bno != busketno)
				release(&bcache.hashtable[bno].lock);
		}
	}

	panic("bget: no buffer");
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
	if (!holdingsleep(&b->lock))
		panic("brelse");

	releasesleep(&b->lock);

	int busketno = b->blockno % NBUCKET;
	acquire(&bcache.hashtable[busketno].lock);
	b->refcnt--;
	if (b->refcnt == 0) {
		// 只需要在buffer被使用完后更新时间戳
		// 与原程序中buffer使用完后放到head之后的思想是一样的
		// 不必在获取buffer时更新时间戳
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
```