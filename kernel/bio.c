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

#define BUCKETS 13

struct {
	
  struct spinlock lock;
  struct buf buf[NBUF];
	
	// spinlock for each buckets
	struct buf* table[BUCKETS];
	struct spinlock table_lock[BUCKETS];
	struct buf freelist;
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

	// init table
	for (int i = 0; i < BUCKETS; i++) {
		initlock(&bcache.table_lock[i], "bcache");
		bcache.table[i] = 0;
	}

  // Create linked list of buffers
	// LRU list
  bcache.freelist.prev = &bcache.freelist;
  bcache.freelist.next = &bcache.freelist;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.freelist.next;
    b->prev = &bcache.freelist;
    initsleeplock(&b->lock, "buffer");
   	bcache.freelist.next->prev = b;
   	bcache.freelist.next = b;
		b->used = 0;
  }
	printf("binit finish\n");
}

int
TEST_get_free_size()
{
	struct buf* b = 0;
	int res = 0;
	acquire(&bcache.lock);
	b = bcache.freelist.prev;
	while (b != &bcache.freelist) {
		if (!b->used && !b->refcnt) {
			res++;
		}
		b = b->prev;
	}
	release(&bcache.lock);
	return res;
}

struct buf*
get_buf() {
	struct buf* b = 0;
	acquire(&bcache.lock);
	b = bcache.freelist.prev;
	while(b != &bcache.freelist) {
		if (!b->used) {
			b->used = 1;
			b->prev->next = b->next;
			b->next->prev = b->prev;
			break;
		}
		b = b->prev;
	}
	if (b == &bcache.freelist) {
		b = 0;
	}
	release(&bcache.lock);
	return b;
}

void
put_buf(struct buf* b)
{
	acquire(&bcache.lock);
	b->next->prev = b->prev;
	b->prev->next = b->next;
	bcache.freelist.next->prev = b;
	b->next = bcache.freelist.next;
	b->prev = &bcache.freelist;
	bcache.freelist.next = b;	
	b->used = 0;
	release(&bcache.lock);
}

struct spinlock*
get_lock(uint blockno) {
	int n = blockno % BUCKETS;
 	return &bcache.table_lock[n];	
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
	int n = blockno % BUCKETS;
	struct spinlock *lock = get_lock(blockno);

	printf("bget blockno %d n %d\n", blockno, n);
  // acquire(&bcache.lock);
	acquire(lock);
  // Is the block already cached?
  //	for(b = bcache.head.next; b != &bcache.head; b = b->next){
  //	  if(b->dev == dev && b->blockno == blockno){
  //	    b->refcnt++;
  //	    release(&bcache.lock);
  //	    acquiresleep(&b->lock);
  //	    return b;
  //	  }
  //	}
	
	// look up in table cache
	for (b = bcache.table[n]; b; b = b->next_hash) {
		if (b->dev == dev && b->blockno == blockno) {
			b->refcnt++;
			printf("bcache hit, block: %d refcnt: %d\n", b->blockno, b->refcnt);
			release(lock);
			acquiresleep(&b->lock);
			return b;
		}
	}

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
  //   if(b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
	b = get_buf();
	if (!b || !b->used || b->refcnt) {
		if (b) {
			printf("b->bockno: %d, used: %d, refcnt: %d\n", b->blockno, b->used, b->refcnt);
		}
  	panic("bget: no buffers");
	}
	b->dev = dev;
	b->blockno = blockno;
	b->valid = 0;
	b->refcnt = 1;

	// inset to table
	b->next_hash = bcache.table[n];
	printf("bget n: %d, b->blockno: %d refcnt: %d free size: %d\n", n, b->blockno, b->refcnt, TEST_get_free_size());
	bcache.table[n] = b;
	
	release(lock);	
	acquiresleep(&b->lock);
	return b;
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
	int n = b->blockno % BUCKETS;
	struct buf* x = 0;
	struct spinlock* lock = get_lock(b->blockno);

  acquire(lock);
  b->refcnt--;
	printf("brelse block: %d refcnt: %d\n", b->blockno, b->refcnt);
  if (b->refcnt == 0) {
    // no one is waiting for it.
		// remove from table cache
		if (bcache.table[n] == b) {
			bcache.table[n] = 0;
		}
		for (x = bcache.table[n]; x; x = x->next_hash) {
			if (x->next == b) {
				x->next = b->next;
				break;
			}	
		}	
		put_buf(b);
		printf("sucess release block %d free size: %d\n", b->blockno, TEST_get_free_size());
  }
  
  release(lock);
}

void
bpin(struct buf *b) {
	struct spinlock *lock = get_lock(b->blockno);
  acquire(lock);
  b->refcnt++;
	printf("bpin block: %d refcnt: %d\n", b->blockno, b->refcnt);
  release(lock);
}

void
bunpin(struct buf *b) {
	struct spinlock *lock = get_lock(b->blockno);
  acquire(lock);
  b->refcnt--;
	printf("bunpin block: %d refcnt: %d\n", b->blockno, b->refcnt);
  release(lock);
}


