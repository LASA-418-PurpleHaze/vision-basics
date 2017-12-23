//capture images from the camera
//process them to find where the vision target is
//publish information about the target for the roborio to read

//uses an unknown camera height and target height, so it requires the full target to be visible for accurate information
//once camera height and target height are fixed, more flexible geometry can be used

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
#include <chrono>
#include <typeinfo>

using namespace std;
using namespace zmq;
using namespace std::chrono;

//initialize zmq objects
context_t context (1);
socket_t subscriber (context, ZMQ_SUB);

static std::string s_recv_noblock(zmq::socket_t & socket){
	zmq::message_t message;
	socket.recv(&message, ZMQ_NOBLOCK);
	return std::string(static_cast<char*>(message.data()), message.size());
}
long time_at_last_message;

void update_time_at_last_message(){
	time_at_last_message = (system_clock::now().time_since_epoch().count())/1000000000;
}

int seconds_since_last_message(){
	return ((system_clock::now().time_since_epoch().count())/1000000000) - time_at_last_message;
}

void cleanup(string cheese){
	cout << "cleaning up " << cheese << " things" << endl;
}
int main(){
	subscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0);
	subscriber.connect("tcp://localhost:5803");
	string message = "";
	string current_state = "disabled";
	update_time_at_last_message();
	bool initialize = false;
	bool timeout_ending_enabled = false;
	while(message != "END"){
		sleep(1);
		message = s_recv_noblock(subscriber);
		if (message == "AUTO_INIT") {
			if(current_state == "tele"){ cleanup(current_state);}
			update_time_at_last_message();
			timeout_ending_enabled = true;
			current_state = "auto";		
			initialize = true;
		} else if (message == "TELE_INIT") {
			if(current_state == "auto"){
				cleanup(current_state);}
			update_time_at_last_message();
			timeout_ending_enabled = true;
			current_state = "tele";
			initialize = true;
		} else if (message == "DISABLED_INIT") {
			timeout_ending_enabled = false;
			cleanup(current_state);	
			current_state = "lazy";
			initialize = false;

		} else if (message == "END") {
			cleanup(current_state);
			current_state = "lazy";
			initialize = false;
		} else if (timeout_ending_enabled && seconds_since_last_message() > 7){
			message = "END";
			cleanup(current_state);
			current_state = "lazy";
			initialize = false;
		}else{
			initialize = false;
		}
		
		if(initialize == true){ cout << "initialize " << current_state << endl;}
		else if(current_state != "lazy" && current_state != "disabled"){ cout << "doing " << current_state << " things" << endl;}
	}

	cout << "ended" << endl;

	
	
	return 0;

}
