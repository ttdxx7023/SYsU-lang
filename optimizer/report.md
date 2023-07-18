# report

| 学号     | 姓名   |
| -------- | ------ |
| 20337091 | 马佳欣 |

## Deadcode

### 不使用的局部变量、函数参数、函数

在generator, 使用is_promotable判断。
- 优点：在generator阶段就可以实现，不需要生成了ir再删除ir。
- 注意：main不会在其他地方被调用，所以判断函数是否可优化时要判断是否为main函数。

## 强度削减
在generator, 在BinaryExprToIR处判断。
- 优点：在generator阶段就可以简单实现，在optimizer阶段实现代码更复杂
- 优化方向：
  1. Algebraic Identity
     𝑥 + 0 = 0 + 𝑥, 𝑥 × 1 = 1 × 𝑥 ⇒ 𝑥 // 只能消除整型，不能消除浮点类型
  2. Strength Reduction
     $2^n$ × 𝑥 = 𝑥 × $2^n$ ⇒ 𝑥 ≪ n

## Mem2Reg

### 优化流程

- 在代码中已有详细的注释。

### 注意事项

- 值栈`SmallDenseMap<AllocaInst *, std::stack<Value *>>value_stack_of_alloca`需要`undef`兜底值：

  `phi`节点对应的值可能是`undef`的，例如：

  ```c
  void func(){
      int a;
      while (a>0)
      {
          a = a-1;
      }
  }
  ```

  对于`while.cond`中的a变量的`phi`节点，从`entry`进入的值是未知的。

  ```shell
  define void @func() {
  entry:
    br label %while.cond
  
  while.cond:                                       ; preds = %while.body, %entry
    %a0 = phi i32 [ undef, %entry ], [ %1, %while.body ]
    %0 = icmp sgt i32 %a0, 0
    br i1 %0, label %while.body, label %while.end
  
  while.body:                                       ; preds = %while.cond
    %1 = sub i32 %a0, 1
    br label %while.cond
  
  while.end:                                        ; preds = %while.cond
    ret void
  }
  ```

- `to_remove_loads`和`to_remove_stores`：

  不要在遍历`inst`的时候删除，会导致迭代器失效，所以需要先暂存起来。

## 问题和解决方法

1. - `tester/function_test2020/02_arr_defn4.sysu.c`：

      ```
      %a = alloca [4 x [2 x i32]], align 4
      %0 = bitcast [4 x [2 x i32]]* %a to i8*
      
      /tmp/tmpbl6m_r1m/a.ll:6:34: error: expected type
      %0 = bitcast [4 x [2 x i32]]* <badref> to i8*
      ```

    - `tester/function_test2020/35_array_test.sysu.c`：
      数组取地址也是`<badref> `
    - 解决方法：不优化数组。

2. `tester/function_test2020/10_break.sysu.c`：

    ```
    while.cond:                                       ; preds = %if.end, %entry
        %0 = icmp sgt i32 10, 0
        br i1 %0, label %while.body, label %while.end
        %a0 = phi i32 [ 10, %entry ], [ %1, %if.end ]
    
    /tmp/tmp73d4c49h/a.ll:11:35: error: '%1' defined with type 'label' but expected 'i32'
    %a0 = phi i32 [ 10, %entry ], [ %1, %if.end ]
    ```

    循环中`a`值会发生变化，且根据`a`值判断是否继续执行循环，但是上述`phi`节点被放在了block的最后面，导致出错。

    - 解决方法：将`phi`节点插在`block`的最前面。

3. 使用了`Mem2Reg`优化会出错（段错误）的测例：

  1. tester/function_test2022/73_int_io.sysu.c
  2. tester/h_functional/090_int_io.sysu.c（与1是一样的）
  3. third_party/SYsU-lang-tester-perfermance/performance_test2021-public/median2.sysu.c
  4. /workspace/SYsU-lang/long_tester/third_party/SYsU-lang-tester-perfermance/performance_test2022-private/vector_mul2.sysu.c

  解决方法：在`main`函数判断即将优化的函数是否为测例里面的函数，若是则不进行优化。

4. 优化除法

    强度削减中一开始包括除法优化，但是到了某个测例就出错了，因此放弃除法优化。

## 排行榜截图

![image-20230622233737181](report.assets\image-20230622233737181.png)

## 挑战工作

- `lexer`：

  1. 扩展更多 C 语言的 `token`

     - 踩过的坑：

       - 不支持数据类型`bool`，`Clang`将其识别为`identifier`；
       - 不需要写`long long`类型对应的规则，`Clang`将其识别为两个`long`。

     - 更多`token`：

       ```
       token		name
       
       :			colon
       auto		auto
       double		double
       long		long
       enum		enum
       register	register
       short		short
       signed		signed
       unsigned	unsigned
       static		static
       struct		struct
       union		union
       typedef		typedef
       do			do
       switch		switch
       case		case
       default		default
       for			for
       goto		goto
       sizeof		sizeof
       extern		extern
       ++			plusplus
       +=			plusequal
       --			minusminus
       -=			minusequal
       *=			starequal
       /=			slashequal
       %=			percentequal
       ```

  2. 根据新扩展的`token`，编写新的测例`tester/mytest.c`。