#include "/root/sysu/include/sysy/sylib.h"
int a[5][5]={1,2,3,4,5};

int func(int* a[][5], int f){
    int i=0;
    int j=0;
    int sum=0;
    while(i<5){
        while(j<5){
            sum=sum+a[i][j];
            j=j+1;
        }
        i=i+1;
    }
    return sum;
}

int main(){
  int f =1;
    putint(func(&a, f));
    return 0;
}