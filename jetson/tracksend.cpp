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
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <math.h>
#include <iomanip>
#include "zhelpers.hpp"
#include <chrono>
#include <fstream>
#include <thread>
#include <videoio>

using namespace cv;
using namespace std;
using namespace zmq;
using namespace std::chrono;

//define size of image captured
const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;

//define trackbar values
int H_MID = 70;
int H_PM = 10;
int H_MAX = 255;
int S_MIN = 150;
int S_MAX = 255;
int V_MIN = 28;
int V_MAX = 115;
int type_of_thresh; //HSV, RGB, Gray
int num_targets_found; //debugging variable to ensure only one target is found

//focal length is a measurement of the camera
//calculated by doing some calibration with a known distance
int focal_length = 717;

//define morphops trackbar variables
int erode_size = 1;  
int dilate_size = 8; 
int morphop_max = 10;

//initialize zmq objects
context_t context (1);
socket_t publisher (context, ZMQ_PUB);
socket_t subscriber (context, ZMQ_SUB);

//define target size
int target_height = 1; //target height in inches. just using a roll of tape is 1 inch

//boolean for readability in sending data to roborio
bool found_single_target = true; //set to true since contourSelector takes care of it. can be deleted eventually, but roborio code should get updated accordingly

//calculate angle offset between what and the tape?
//choose some point, call it the center of the image, that the camera should try to line up with
Point2f desired_location = Point2f(FRAME_WIDTH/2, FRAME_HEIGHT/2);

/////////////////////////////////////////
//   GLOBAL VARS FOR IMAGE PROCESSING  //
/////////////////////////////////////////

//create a matrix for each step to improve readability
	Mat src;
	Mat color_converted;
	Mat thresh;
	Mat morphops;
	Mat erodeElement;
	Mat dilateElement;
	Mat boxes;

	VideoCapture capture;

	//create vars that will contain the information we're trying to send
	double pan_adjust;
	double tilt_adjust;
	double depth;


////////////////////////////////////////
// GLOBAL VARS FOR ROBORIO CONNECTION //
////////////////////////////////////////
	string roborio_message = "";
	string current_match_state = "disabled";
	bool need_to_initialize_videowriting = false;
	bool timeout_ending_enabled = false;


//////////////////////////////////////////////
// GLOBAL VARS FOR DISPLAY AND VIDEOWRITING //
//////////////////////////////////////////////
	//var to decide whether or not a display screen is connected
	bool graphics_mode;

	VideoWriter src_writer;
	VideoWriter thresh_writer;
	VideoWriter boxes_writer;

	string videowriting_path;

////////////////////////////////////
//    TIMEOUT VARS AND FUNCTIONS  //
////////////////////////////////////

//global var and functions for timeout of program (if a disabled message should've been sent, but hasn't)
//used to make sure that all data gets written to file, even if the disabled message doesn't come
long time_at_last_message;



int getCurrentTime() {
	return (system_clock::now().time_since_epoch().count())/1000000000;
}
void updateTimeAtLastMessage() {
	time_at_last_message = getCurrentTime();
}

int secondsSinceLastMessage() {
	return getCurrentTime() - time_at_last_message;
}

int getTimeoutAllowance(string state){
	if (state == "auto") {
		return 15;
	} else if (state == "tele") {
		return 135;
	}
}

////////////////////////////
// VIDEOWRITING FUNCTIONS //
////////////////////////////

void initializeVideoWriting(){
	src_writer.open(videowriting_path + current_match_state + "src.avi",
			CV_FOURCC('M', 'J', 'P', 'G'),
			30,
			cv::Size(FRAME_HEIGHT, FRAME_WIDTH);
	thresh_writer.open(videowriting_path + current_match_state + "thresh.avi",
			CV_FOURCC('M', 'J', 'P', 'G'),
			30,
			cv::Size(FRAME_HEIGHT, FRAME_WIDTH);
	boxes_writer.open(videowriting_path + current_match_state + "boxes.avi",
			CV_FOURCC('M', 'J', 'P', 'G'),
			30,
			cv::Size(FRAME_HEIGHT, FRAME_WIDTH);
}
			  
void cleanupVideowriting(string match_state){
	src_writer.release();
	thresh_writer.release();
	boxes_writer.release();
}

void displayAndWrite(string label, Mat input){
	if (graphics_mode) {
		imshow(label, input);
	}
	
	if (label == "src" && src_writer.isOpened()) {
		src_writer << input;
	} else if (label == "thresh" && thresh_writer.isOpened()) {
		cvtColor(input, input, CV_GRAY2BGR);
		thresh_writer << input;
	} else if (label == "boxes" && boxes_writer.isOpened()) {
		cvtColor(input, input, CV_GRAY2BGR);
		boxes_writer << input;
	}
}
///////////////////////////////// 
//  IMAGE PROCESSING UTILITIES //
/////////////////////////////////

//dimensions of rectangles that will be drawn around objects
struct WidthAndHeight{
	int width;
	int height;
};

//calculate distance between two points in the image
//used for determining orientation of the target, esp. if rectangle is rotated
int distanceFormula(Point point_a, Point point_b){
	return sqrt((point_a.x-point_b.x)*(point_a.x-point_b.x)+(point_a.y-point_b.y)*(point_a.y-point_b.y));
}

//determine dimensions/orientation of a rectangle
WidthAndHeight calcWidthAndHeight(Point point_one, Point point_two, Point point_three){
	WidthAndHeight result;
	
	int length_one = distanceFormula(point_one, point_two);
	int length_two = distanceFormula(point_two, point_three);

	if(length_one > length_two){
		result.width = length_one;
		result.height = length_two;
	}else{ //length_one is < length_two
		result.width = length_two;
		result.height = length_one;
	}
	return result;
}

//required function for using trackbars
void trackbarCallback(int, void*){
	//doesn't need to do anything since the trackbars just change a global value
}

//create trackbars to adjust parameters for filtering image
void createTrackbars(){
	if(graphics_mode){
		namedWindow("HSV adjust", 0);
		createTrackbar("0: HSV | 1: RGB | 2: Gray", "HSV adjust", &type_of_thresh, 2, trackbarCallback);
		createTrackbar("H_MID", "HSV adjust", &H_MID, H_MAX, trackbarCallback);
		createTrackbar("H_PM", "HSV adjust", &H_PM, 255, trackbarCallback);
		createTrackbar("S_MIN", "HSV adjust", &S_MIN, H_MAX, trackbarCallback);
		createTrackbar("S_MAX", "HSV adjust", &S_MAX, H_MAX, trackbarCallback);
		createTrackbar("V_MIN", "HSV adjust", &V_MIN, H_MAX, trackbarCallback);
		createTrackbar("V_MAX", "HSV adjust", &V_MAX, H_MAX, trackbarCallback);
		createTrackbar("ERODE_SIZE", "HSV adjust", &erode_size, morphop_max, trackbarCallback);
		createTrackbar("DILATE_SIZE", "HSV adjust", &dilate_size, morphop_max, trackbarCallback);
	}
}


//go from radians to degrees
double cvtAngle(double radianVal){

	return (180/3.14) * radianVal;
}


//go through an array of contours and return the index of the contour with the greatest area
//takes in the address of the array, and the size
//used to find biggest target, essentially a filter for morphops-resistant noise
int contourSelector(vector<vector<Point>> contours){
	int max_area_index = 0;
	for(int i = 0; i < contours.size(); i++){
		//if a contour's area is larger than the current max, update the current max
		if(contourArea(contours[i]) > contourArea(contours[max_area_index])){ 
			max_area_index = i;
			
		}
	}
	return max_area_index;
}

/////////////////////
//  ZMQ FUNCTIONS  //
/////////////////////

static std::string s_recv_noblock(zmq::socket_t & socket) {
	zmq::message_t message;
	socket.recv(&message, ZMQ_NOBLOCK);
	return std::string(static_cast<char*>(message.data()), message.size());
}

void publishKeyAndValue(string key, string value){
	//cout << "sending" << endl;
	//cout << key << ":" << value << endl;
	string key_value_pair = key + ":" + value;
	s_send(publisher, key_value_pair);
}

void findAndSendTemperature(){
	try{
		ifstream temperature_file;
		temperature_file.open("/sys/class/thermal/thermal_zone2/temp", ios::in);
		string temperature_key = "JETSON.TEMPERATURE";

		double temperature_value;
		temperature_file >> temperature_value;
		
		stringstream temperature_ss;
		temperature_ss << setprecision(3) << temperature_value/1000 << endl;
		publishKeyAndValue(temperature_key, temperature_ss.str()); 
		temperature_file.close();
	}catch(...){} //don't do anything if you can't get the temperature reading
}

void publishVisionCalculations(){
	string pan_key = "JETSON.PAN_VALUE";
	stringstream pan_ss;
	pan_ss << setprecision(3) << pan_adjust << endl;
	publishKeyAndValue(pan_key, pan_ss.str());

	string tilt_key = "JETSON.TILT_VALUE";
	stringstream tilt_ss;
	tilt_ss << setprecision(3) << tilt_adjust << endl;
	publishKeyAndValue(tilt_key, tilt_ss.str());

	string depth_key = "JETSON.DEPTH";
	stringstream depth_ss;
	depth_ss << setprecision(3) << depth << endl;
	publishKeyAndValue(depth_key, depth_ss.str());
}

//////////////////////////////
// MAIN PROCESSING FUNCTION //
//////////////////////////////
void getAndProcessImage(){
	//read image from camera and show it in its own window
		capture.read(src);
		
		displayAndWrite("src", src);

		//set up next matrix with proper size and type
		color_converted.create(src.size(), src.type());

		//convert the colorspace according to the threshold type trackbar
		switch(type_of_thresh){
			case 0:
				cvtColor(src, color_converted, COLOR_BGR2HSV);
				break;
			case 1:
				src.copyTo(color_converted);
				break;
			case 2:
				cvtColor(src, color_converted, COLOR_BGR2GRAY);
	   	} 
	   
		//show the converted color image in its own window
		
		displayAndWrite("converted", color_converted);

		//threshold the image using ranges from the trackbars and show the thresholded image
		inRange(color_converted, Scalar(H_MID - H_PM, S_MIN, V_MIN), Scalar(H_MID + H_PM, S_MAX, V_MAX), thresh);

		displayAndWrite("thresh", thresh);

		//apply some morphops to reduce noise and show the resulting image
		//this changes the size of the object in the image.
		//if either erode and dilate size is set to 0, skip the morphops process. mainly for testing it without changing the size of the object, but also so it doesn't die when one of them is 0.
		if(erode_size != 0 && dilate_size != 0){ 
			erodeElement = getStructuringElement( MORPH_RECT,Size(erode_size, erode_size));
			//dilate with larger element object is nicely visible
			dilateElement = getStructuringElement( MORPH_RECT,Size(dilate_size, dilate_size));
			erode(thresh, morphops, erodeElement);
			dilate(morphops, morphops, dilateElement);
		}else{
			morphops = thresh.clone(); //take the value of thresh, but don't edit it.
			char buffer[50];
			sprintf(buffer, "no morphops");
			putText(morphops, buffer, Point(0, 60), 2, 2, Scalar(255, 0, 255), 2);
		}
		
		displayAndWrite("morphops", morphops);
	
		//find all objects in the thresholded image (hopefully there's only one)
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		findContours(thresh, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
		vector<RotatedRect> minRect(contours.size());
		
		//create rectangles around all of the objects in the thresholded image
		for(int i = 0; i < contours.size(); i++){
			minRect[i] = minAreaRect(Mat(contours[i]));
		}
		
		//create an empty matrix that will be used to show the final important things in the image
		boxes = Mat::zeros(thresh.size(), thresh.type());
	
		int target_index = contourSelector(contours);
		//for each object in the noise reduced image, draw a box around it.
		//if it seems big enough (probably not noise and is the actual object, calculate the angle
		for( int i = 0; i< contours.size(); i++ ){
			
	       		Scalar color = Scalar(255, 255, 255);
						//show the contour of each contour
						if(i == target_index){
		        	drawContours( boxes, contours, i, color, 1, 8, vector<Vec4i>(), 0, Point() );
						}
		        //show each contour's rectangle
			Point2f rect_points[4]; 
			minRect[i].points( rect_points );
			if(i == target_index){
			for( int j = 0; j < 4; j++ ){
	          		line( boxes, rect_points[j], rect_points[(j+1)%4], color, 1, 8 );
			}
			}
			
			//if the rectangle seems to be about the right size for the target and isn't noise, do some calculations with it
			WidthAndHeight box_measurements = calcWidthAndHeight(rect_points[0], rect_points[1], rect_points[2]);
					
			if(i == target_index){
				
				//calculate depth and center of the target
				//draw some dots where the object is and where it should be if we're lined up perfectly
				//do some conversions between inches and pixels, and use some trig to get the depth and then the angle offset in both directions
				//keep the numbers to send to the roborio, but also put them at the top of the final display image
				depth = (focal_length*target_height) / box_measurements.height;
				Moments target_moment = moments(contours[i], false);
				Point2f target_center = Point2f(target_moment.m10/target_moment.m00, target_moment.m01/target_moment.m00);
				circle(boxes, target_center, 4, color, -1, 8, 0);
				circle(boxes, desired_location, 8, color, -1, 8, 0);
				double inches_offset_x = (((desired_location.x - target_center.x) / box_measurements.height)*target_height);
				pan_adjust = -1*cvtAngle(atan(inches_offset_x / depth));
				double inches_offset_y = (((desired_location.y - target_center.y) / box_measurements.height)*target_height);
				tilt_adjust = -1*cvtAngle(atan(inches_offset_y / depth));
				char print_buffer[50];
				sprintf(print_buffer, "%.1f and %.1f (%.0f)", pan_adjust, tilt_adjust, depth);
				putText(boxes, print_buffer, Point(0, 60), 2, 2, Scalar(255, 0, 255), 2);

				
			}
		 }

		//display the contour of each object, 
		//the rectangle of each object, 
	  //the two centers, and the angle offsets and depth in one image
		displayAndWrite("boxes", boxes);	

			
		// not needed, since contourSelector should guarantee only one target
		//determine whether or not we found a single target and store it in a boolean for readability
		//***REPLACE WITH CORNER COUNTING CHECK OR AREA VS CONVEX HULL AREA PROPORTION CHECK***
		//***********************CURRENT CHECK ISN'T DOING MUCH********************************
		if(contours.size() > 0){
			found_single_target = true;
		}else{
			found_single_target = false;
		}

}


void handleMatchFlow(){
//handle messaging from roborio and which match mode the robot is in
		//determines where to write video to and whether or not to exit based on time
		roborio_message	= s_recv_noblock(subscriber);	
		if (roborio_message == "ROBORIO.STATE:AUTO_INIT") {
			if (current_match_state == "tele") {	//refers to previous state  redundant cleanup if mode switches without disable
				cleanupVideowriting(current_match_state); 			//refers to previous state
			}
			if (current_match_state != "auto" ) {	//refers to previous state make sure it isn't a duplicate message
				updateTimeAtLastMessage();
				timeout_ending_enabled = true;
				current_match_state = "auto";
				need_to_initialize_videowriting = true;
			}
		} else if (roborio_message == "ROBORIO.STATE:TELE_INIT") {
			if (current_match_state == "auto") {	//refers to previous state  redundant cleanup if mode switches without disable
				cleanupVideowriting(current_match_state);				//refers to previous state	 
			}
			if (current_match_state != "tele") {  //refers to previous state  make sure it isn't a duplicate message
				updateTimeAtLastMessage();
				timeout_ending_enabled = true;
				current_match_state = "auto";
				need_to_initialize_videowriting = true;
			}
		} else if (roborio_message == "ROBORIO.STATE:DISABLED_INIT") {
			if (current_match_state != "disabled") {
				timeout_ending_enabled = false;
				cleanupVideowriting(current_match_state);
				current_match_state = "disabled";
				need_to_initialize_videowriting = false;
			}
		} else if (roborio_message == "EXIT") { //exit on forced exit from user, mainly for testing, kinda useless
			cleanupVideowriting(current_match_state);
			current_match_state = "disabled";
			need_to_initialize_videowriting = false;
		} else if (timeout_ending_enabled && (secondsSinceLastMessage() > getTimeoutAllowance(current_match_state))) {
			roborio_message = "EXIT";
			cleanupVideowriting(current_match_state);
			current_match_state = "disabled";
			need_to_initialize_videowriting = false;
		} else {
			need_to_initialize_videowriting = false;
		}
	
		if(need_to_initialize_videowriting){
			initializeVideowriting();
		}

}

void setupCamera(){
	capture.open(1);
	capture.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
}

void setupSockets(){
	publisher.bind("tcp://*:5803");
	subscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0);
	subscriber.connect("tcp://localhost:5804"); //change this for getting from roborio
}
/////////////////
// ACTUAL MAIN //
/////////////////
int main(int argc, char* argv[]){
	
	//default values
	graphics_mode = true;
	videowriting_path = "/media/nvidia";
	
	//overwrite default values with command line arguments
	if(argc > 1){
		if(string(argv[1]) == "-match"){
			graphics_mode = false;
		}
	}
	
	if(argc > 2){
		videowriting_path = string(argv[2]);
	}
	
	//setup things
	setupCamera();
	setupSockets();
	
	createTrackbars();

	//drop the exposure of the camera to 2ms
	system("v4l2-ctl -d /dev/video1 -c exposure_auto=1 -c exposure_absolute=3");
	
	//check match mode, set a boolean to determine whether or not to display images to a connected screen
	
	
	
	//process the image and send results forever (not really)
	while(roborio_message != "EXIT") {
		handleMatchFlow();

		if(current_match_state != "disabled"){
			getAndProcessImage();
			publishVisionCalculations();
			findAndSendTemperature();
		}
		
    //unsigned int ms = duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
		//cout << ms << endl;
				
		//have a waitkey to make opencv happy 
		int c = waitKey(10);

		//break on esc, only for development 
		if((char)c == 27){
			break;
		}
	}
	
	return 0;

}
