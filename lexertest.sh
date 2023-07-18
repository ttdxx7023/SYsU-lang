( export PATH=$HOME/sysu/bin:$PATH \
  CPATH=$HOME/sysu/include:$CPATH \
  LIBRARY_PATH=$HOME/sysu/lib:$LIBRARY_PATH \
  LD_LIBRARY_PATH=$HOME/sysu/lib:$LD_LIBRARY_PATH &&
  sysu-compiler --unittest=lexer-0 "**/*.sysu.c" &&
  sysu-compiler --unittest=lexer-1 "**/*.sysu.c" &&
  sysu-compiler --unittest=lexer-2 "**/*.sysu.c" &&
  sysu-compiler --unittest=lexer-3 "**/*.sysu.c" &&
  echo pass all!)