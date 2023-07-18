# export TEST_FILE="/workspace/SYsU-lang/tester/**/*.sysu.c"
export TEST_FILE=/workspace/SYsU-lang/mytest.c
# ( export PATH=$HOME/sysu/bin:$PATH \
#   CPATH=$HOME/sysu/include:$CPATH \
#   LIBRARY_PATH=$HOME/sysu/lib:$LIBRARY_PATH \
#   LD_LIBRARY_PATH=$HOME/sysu/lib:$LD_LIBRARY_PATH &&
#   sysu-compiler --unittest=parser-0 ${TEST_FILE} &&
#   sysu-compiler --unittest=parser-1 ${TEST_FILE} )
# echo "All pass(ignore long codes)!"

( export PATH=$HOME/sysu/bin:$PATH \
  CPATH=$HOME/sysu/include:$CPATH \
  LIBRARY_PATH=$HOME/sysu/lib:$LIBRARY_PATH \
  LD_LIBRARY_PATH=$HOME/sysu/lib:$LD_LIBRARY_PATH &&
  clang -cc1 -ast-dump=json ${TEST_FILE})