#include <stdio.h>
#include <kiwi.h>

int multiply(const int *i) {
  return *i * 2;
}

void print_int(unsigned int idx, const int *i) {
  if (idx == 0) printf("%d", *i);
  else printf(", %d", *i);
}

int main() {
  int_list *l = make(_int_list);
  append(append(append(append(l, 1), 7), 8), 9);
  int_list *l2 = map(l, multiply);
  iteri(l, print_int);
  printf("\n");
  iteri(l2, print_int);
  printf("\n");
  drop(l);
  drop(l2);
}
