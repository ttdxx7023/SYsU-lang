export CHECK_FILE=/workspace/SYsU-lang/tester/mytest.c

$HOME/sysu/bin/sysu-preprocessor $CHECK_FILE | clang -cc1 -dump-tokens > baseout 2>&1