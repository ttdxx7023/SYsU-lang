export CHECK_FILE=/workspace/SYsU-lang/tester/mytest.c

$HOME/sysu/bin/sysu-preprocessor $CHECK_FILE | $HOME/sysu/bin/sysu-lexer > myout 2>&1