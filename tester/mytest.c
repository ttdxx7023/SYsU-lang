#define MAXSIZE 128
const int test_const = 1;
int ex = 0;
typedef unsigned char BYTE;
void test_void(){}
int test_extern(){
    extern int ex;
    ex = 1;
    return ex;
}
bool test_op(){
    int a = 1, b = 2;
    int c = a + b;
    int d = a - b;
    int e = !a;
    int f = a * b;
    int g = a / b;
    int h = a % b;
    if(a < b){}
    if(a > b){}
    if(a <= b){}
    if(a >= b){}
    if(a == b){}
    if(a != b){}
    if(a && b){}
    if(a || b){}
    int i = a++;
    int j = a+=1;
    int k = a--;
    int l = a-=1;
    int m = a*=2;
    int n = a/=2;
    int o = a%=2;
}
int main(){
    int i = 1, ix = 0x123, io = 0123, ib = 10, ie = 1e-1;
    auto at = 1.1;
    float f = 2.2;
    double de = .2e5, dp = 0x12.3p5;
    long l = 111111111111111111111;
    long long ll = 11111111111111111111111111;
    enum week {Sun, Mon, Tue, Wed, Thu, Fri, Sat};
    register reg = 12345;
    short sh = 111111;
    signed int si = -123;
    unsigned int usi = 123;
    static int static_int = 456;
    struct student
    {
        char name[50];
        int id; 
    } s;
    union Data
    {
        int i;
        float f;
        char  str[20];
        bool check;
    } data;
    BYTE test_type = 1;
    test_void();
    if(test_type){

    }
    else{

    }
    while(!test_type){

    }
    do{

    }while(!test_type);
    for(int i = 0; i < 2; i++){
        if(i){
            break;
        }
        else{
            continue;
        }
        test_type = 0;
    }
    switch(test_type){
        case 0: test_type = 1;
        case 1: test_type = 0;
        default: test_type = 11;
    }
    /* do 循环执行 */
    int test = 10;
    LOOP:do
    {
        if(test == 10)
        {
            /* 跳过迭代 */
            test += 1;
            goto LOOP;
        }
        test--;
    }while(test > 5 );
    char str[20] = "hello\n";
    int len = sizeof(str);
    test_extern();
    test_op();
    // comment
    return 0;
}