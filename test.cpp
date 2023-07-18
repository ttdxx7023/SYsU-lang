#include <iostream>
#include <regex>

int main(){
	const char r[20] = "\\\\\\\n"
    "slslsls";
	std::string str(r);
    std::cout<<str<<std::endl;
    return 0;
}