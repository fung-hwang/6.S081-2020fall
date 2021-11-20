# MIT6.S081-2020fall
### 课程简介
1. 本课程为 **MIT6.S081-2020fall**
2. 课程在OS理论的宽度和深度上没有延展太多，核心是xv6的实现以及实验内容。在实际的编码过程中，我们将会直观、细致、深入地理解操作系统内核实现，这是单纯的理论学习难以触及的。
3. xv6 book和对应的实验将逐步引领我们阅读、理解xv6源码，并在xv6上做进一步开发和优化。

### 相关资料
1. [课程官网: MIT6.S081-2020fall](https://pdos.csail.mit.edu/6.S081/2020/schedule.html)
2. [课程内容整理与翻译](https://mit-public-courses-cn-translatio.gitbook.io/mit6-s081/) [（贡献者：肖宏辉）](https://www.zhihu.com/people/xiao-hong-hui-15)
3. [All-in-one课程仓库（xv6中文文档、lab翻译及解析）](http://xv6.dgs.zone/)
4. [xv6中文文档（2）](https://github.com/pleasewhy/xv6-book-2020-Chinese)
5. [名校公开课程评价网-MIT6.S081](https://conanhujinming.github.io/comments-for-awesome-courses/%E6%93%8D%E4%BD%9C%E7%B3%BB%E7%BB%9F/MIT6.S081%E6%93%8D%E4%BD%9C%E7%B3%BB%E7%BB%9F%E5%AF%BC%E8%AE%BA/)
6. [Lab环境搭建指南](https://www.bilibili.com/video/BV11K4y127Qk?)
7. [xv6 book阅读笔记](https://github.com/fung-hwang/MIT6.S081-2020fall/blob/main/xv6-book-2020%20CHS.pdf)
8. [OS学习过程中的记录与思考（待填坑）]()

### 实验
1. [Lab1: Xv6 and Unix utilities](https://github.com/fung-hwang/MIT6.S081-2020fall/blob/main/lab/lab1-utilities.md)
2. [Lab2: System calls](https://github.com/fung-hwang/MIT6.S081-2020fall/blob/main/lab/lab2-system-calls.md)
3. [Lab3: Page tables](https://github.com/fung-hwang/MIT6.S081-2020fall/blob/main/lab/lab3-page-tables.md)
4. [Lab4: Traps](https://github.com/fung-hwang/MIT6.S081-2020fall/blob/main/lab/lab4-traps.md)
5. [Lab5: Lazy allocation](https://github.com/fung-hwang/MIT6.S081-2020fall/blob/main/lab/lab5-lazy-page-allocation.md)
6. [Lab6: Cow fork](https://github.com/fung-hwang/MIT6.S081-2020fall/blob/main/lab/lab6-cow-fork.md)
7. [Lab7: Multithreading](https://github.com/fung-hwang/MIT6.S081-2020fall/blob/main/lab/lab7-multithreading.md)
8. [Lab8: Locks](https://github.com/fung-hwang/MIT6.S081-2020fall/blob/main/lab/lab8-locks.md)
9. [Lab9: File system](https://github.com/fung-hwang/MIT6.S081-2020fall/blob/main/lab/lab9-file-system.md)
10. Lab10 和 Lab11 暂时搁置

### 补充说明
1. 实验应独立完成，而是否参考、何时参考他人的实现方式可根据自己的情况决定。
2. 我认为该课程的前置知识包括：C（熟练）、汇编（入门）、操作系统基础理论。三者中应至少具备两者，否则，即使边学边补，需要补的内容太多也会影响主线xv6的学习。
3. 如果对OS理论较为了解，且能完全读懂xv6 book，那么课程视频不看也无妨。在出现理解有困难的地方时，课程视频可能会有所帮助，毕竟课堂讲解会更通俗易懂。
4. 学习曲线不算陡峭，只在hard lab处会感到难度有明显提高。 
5. 在实验过程中需要学习一些工具，gdb、vim、git等等。开始时也许有些不适，但很快就会发现这些工具都非常好用，花时间学习是值得的。（然而我现在gdb的门还没入，sad）
6. 如果英语阅读能力过关，更建议阅读原版xv6 book；或在中文版表述不清时参考原版。
7. lab的完整源码在仓库的分支branch中。

### 写在最后
+ 借此课程查补欠缺的OS基础理论和相关实践
+ 此后OS学习暂定围绕OS基本理论、Linux内核设计和Linux环境编程展开
+ 感谢所有的MIT6.S081资源贡献者
+ 感谢邹姝稚老师带领我完成OS基础理论的学习，我受益至今
+ 讨论交流/指正错误，欢迎issue或与我联系zhoushaofeng@mail.ustc.edu.cn
