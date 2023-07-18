export TEST_FILE=/workspace/SYsU-lang/mytest.c
# export TEST_FILE=/workspace/SYsU-lang/nopass/73_int_io.sysu.c
# export TEST_FILE=nopass/median2.sysu.c
# export TEST_FILE=/workspace/SYsU-lang/long_tester/third_party/SYsU-lang-tester-perfermance/performance_test2022-public/hoist-3.sysu.c
# export TEST_FILE="long_tester/**/*.sysu.c"
# export TEST_FILE="tester/**/*.sysu.c"

# 总测试
( export PATH=$HOME/sysu/bin:$PATH \
  CPATH=$HOME/sysu/include:$CPATH \
  LIBRARY_PATH=$HOME/sysu/lib:$LIBRARY_PATH \
  LD_LIBRARY_PATH=$HOME/sysu/lib:$LD_LIBRARY_PATH &&
  sysu-compiler --unittest=benchmark_generator_and_optimizer_1 ${TEST_FILE})


# # clang:
# ( export PATH=$HOME/sysu/bin:$PATH \
#   CPATH=$HOME/sysu/include:$CPATH \
#   LIBRARY_PATH=$HOME/sysu/lib:$LIBRARY_PATH \
#   LD_LIBRARY_PATH=$HOME/sysu/lib:$LD_LIBRARY_PATH &&
#   clang -E ${TEST_FILE} |
#   clang -cc1 -O0 -S -emit-llvm ) > clang_out


# # sysu-generator:
( export PATH=$HOME/sysu/bin:$PATH \
  CPATH=$HOME/sysu/include:$CPATH \
  LIBRARY_PATH=$HOME/sysu/lib:$LIBRARY_PATH \
  LD_LIBRARY_PATH=$HOME/sysu/lib:$LD_LIBRARY_PATH &&
  clang -E ${TEST_FILE} |
  clang -cc1 -ast-dump=json |
  sysu-generator |
  sysu-optimizer) > my_out


# 可视化:
# ( export PATH=$HOME/sysu/bin:$PATH \
#   CPATH=$HOME/sysu/include:$CPATH \
#   LIBRARY_PATH=$HOME/sysu/lib:$LIBRARY_PATH \
#   LD_LIBRARY_PATH=$HOME/sysu/lib:$LD_LIBRARY_PATH &&
#   clang -E ${TEST_FILE} |
#   clang -cc1 -O0 -S -emit-llvm |
#   opt -dot-cfg )