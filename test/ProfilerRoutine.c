// RUN: echo "skip me"
#include <stdio.h>
void bb_exec(char *bb_name, char *func_name, char *module_name) {
  printf("BB_EXEC: %s %s %s \n", bb_name, func_name, module_name);
}