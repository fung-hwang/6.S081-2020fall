# Lab9: File System

### Large files
+ 仿照一级间接块地址 singly-indirect 摹写二级间接块地址 doubly-indirect
+ `fs.h`中保留NINDIRECT是出于mkfs的需要

`kernel/fs.h`
```c
#define NDIRECT 11
#define NSINDIRECT (BSIZE / sizeof(uint))        // singly-indirect
#define NDINDIRECT (BSIZE / sizeof(uint) * BSIZE / sizeof(uint))        // doubly-indirect
#define MAXFILE (NDIRECT + NSINDIRECT + NDINDIRECT)
#define NINDIRECT (NSINDIRECT + NDINDIRECT)
```

`kernel/fs.c/bmap()`
```c
static uint
bmap(struct inode *ip, uint bn)
{
  uint addr, *a;
  struct buf *bp;

  if(bn < NDIRECT){
    if((addr = ip->addrs[bn]) == 0)
      ip->addrs[bn] = addr = balloc(ip->dev);
    return addr;
  }
  bn -= NDIRECT;

  if(bn < NSINDIRECT){
    // Load singly-indirect block, allocating if necessary.
    if((addr = ip->addrs[NDIRECT]) == 0)
      ip->addrs[NDIRECT] = addr = balloc(ip->dev);
    bp = bread(ip->dev, addr);
    a = (uint*)bp->data;
    if((addr = a[bn]) == 0){
      a[bn] = addr = balloc(ip->dev);
      log_write(bp);
    }
    brelse(bp);
    return addr;
  }
  bn -= NSINDIRECT;    

  if(bn < NDINDIRECT){
    // Load doubly-indirect block, allocating if necessary.
    if((addr = ip->addrs[NDIRECT+1]) == 0)
        ip->addrs[NDIRECT+1] = addr = balloc(ip->dev);
    bp = bread(ip->dev, addr);
    a = (uint*)bp->data;
    if((addr = a[bn/NSINDIRECT]) == 0){
      a[bn/NSINDIRECT] = addr = balloc(ip->dev);
      log_write(bp);
    }
    brelse(bp);
        
    bp = bread(ip->dev, addr);
    a = (uint*)bp->data;
    if((addr = a[bn%NSINDIRECT]) == 0){
      a[bn%NSINDIRECT] = addr = balloc(ip->dev);
      log_write(bp);
    }
    brelse(bp);
    return addr;
  }

  panic("bmap: out of range");
}
```

`kernel/fs.c/itrunc()`
```c
void
itrunc(struct inode *ip)
{
  int i, j;
  struct buf *bp;
  struct buf *bp2;
  uint *a;
  uint *a2;

  for(i = 0; i < NDIRECT; i++){
    if(ip->addrs[i]){
      bfree(ip->dev, ip->addrs[i]);
      ip->addrs[i] = 0;
    }
  }

  if(ip->addrs[NDIRECT]){
    bp = bread(ip->dev, ip->addrs[NDIRECT]);
    a = (uint*)bp->data;
    for(j = 0; j < NSINDIRECT; j++){
      if(a[j])
        bfree(ip->dev, a[j]);
    }
    brelse(bp);
    bfree(ip->dev, ip->addrs[NDIRECT]);
    ip->addrs[NDIRECT] = 0;
  }
    
  if(ip->addrs[NDIRECT+1]){
    bp = bread(ip->dev, ip->addrs[NDIRECT+1]);
    a = (uint*)bp->data;
    for(int j = 0; j < NSINDIRECT; j++){
      if(a[j]){
        bp2 = bread(ip->dev, a[j]);
        a2 = (uint*)bp2->data;
        for(int k = 0; k < NSINDIRECT; k++){
          if(a2[k])
            bfree(ip->dev, a2[k]);
        }
        brelse(bp2);
        bfree(ip->dev, a[j]);
        a[j] = 0;
      }//if
    }//for
    brelse(bp);
    bfree(ip->dev, ip->addrs[NDIRECT+1]);
    ip->addrs[NDIRECT+1] = 0;
  }

  ip->size = 0;
  iupdate(ip);
}
```

### Symbolic links
+ 在系统调用层面（高层）上调用inode, buf, log相关低层函数，即用低层提供的接口组织高层的内容
+ 在本节实验中，创建软链 = 创建文件create + 向文件写入目标路径writei，打开软链 = 从文件中读出目标路径readi + 根据路径查找文件namei
+ 难点之一是低层函数中关于锁的部分，fs的基本设计方法之一是函数执行成功则返回内容（比如inode*）持有锁，函数执行失败则会在返回前释放锁，不会带出函数。所以在处理（调用这些低层函数）正常执行的流程时，应该在之后释放锁，而处理异常时往往不需要考虑锁。
+ 文件系统fs是一个层次化系统，低层的设计是否干净、简洁、便于组合是非常重要的。这也正是Unix的设计理念。

`kernel/sysfile.c/create()`
```c
if((ip = dirlookup(dp, name, 0)) != 0){
  iunlockput(dp);
  ilock(ip);
  if(type == T_FILE && (ip->type == T_FILE || ip->type == T_DEVICE))
    return ip;
      // ======== lab9 =========
  if(type == T_SYMLINK && ip->type == T_SYMLINK)
    return ip;
      // ======== lab9 =========
  iunlockput(ip);
  return 0;
}
```

`kernel/sysfile.c/sys_symlink()`
```c
uint64
sys_symlink(void)
{
  char target[MAXPATH], path[MAXPATH];
  struct inode* ip;
  if(argstr(0, target, MAXPATH) < 0 || argstr(1, path, MAXPATH) < 0)
    return -1;
      
  begin_op();
  ip = create(path, T_SYMLINK, 0, 0);
  if(ip == 0){
    end_op();
    return -1;
  }
  // write target to inode
  if(writei(ip, 0, (uint64)target, 0, MAXPATH) < MAXPATH){
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}
```

`kernel/sysfile.c/sys_open()`
```c
// symlink
int cnt = 0;      // recursion count
while(ip->type == T_SYMLINK && !(omode & O_NOFOLLOW)){
  if(readi(ip, 0, (uint64)path, 0, MAXPATH) < MAXPATH){
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlockput(ip);
  if((ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);
  if(++cnt >= 10){      // 10 is threshold
    iunlockput(ip);
    end_op();
    return -1;
  }
}
```