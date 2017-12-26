#include <stdlib.h>
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iomanip>
#include "zhelpers.hpp"

using namespace std;
using namespace zmq;

//initialize zmq objects
context_t context (1);
socket_t publisher (context, ZMQ_PUB);

void publish(string message){
	s_send(publisher, message);
}

int main(){
	
	publisher.bind("tcp://*:5804");
	string message = "initial message";
	while(1){
		cin >> message;
		publish(message);
	}
}
