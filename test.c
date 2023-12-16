#include<string.h>
#include<stdio.h>
#include<stdlib.h>
int main() {
    char buf[] = "asdfasdfasdf";
    char *ptr1 = buf;
    char *ptr2 = buf + 2;
    printf("%c, %c, %ld\n", *ptr1, *ptr2, ptr2-ptr1);
}