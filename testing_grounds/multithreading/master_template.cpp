#include <thread>
#include <iostream>
#include "counting_slave.cpp"
#include "talking_slave.cpp"

//using namespace std;

void call_from_thread() {
	std::cout << "we'd be glad to haze with you" << std::endl;
}

int main() {
	std::thread t1(counting_slave);
	std::thread t3(talking_slave);
	t1.join();	
	t3.detach();

	return 0;
}


