#include <iostream>
#include <unistd.h>
using namespace std;

void counting_slave(){
	for(int i = 0; i < 10; i++){
		cout << i << endl;
		sleep(1);
	}
}
