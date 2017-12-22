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

void publishVisionCalculations(double pan_value, double tilt_value, double depth_value){
	string pan_key = "JETSON.PAN_VALUE";
	stringstream pan_ss;
	pan_ss << setprecision(3) << pan_value << endl;
	publishKeyAndValue(pan_key, pan_ss.str());

	string tilt_key = "JETSON.TILT_VALUE";
	stringstream tilt_ss;
	tilt_ss << setprecision(3) << tilt_value << endl;
	publishKeyAndValue(tilt_key, tilt_ss.str());

	string depth_key = "JETSON.DEPTH";
	stringstream depth_ss;
	depth_ss << setprecision(3) << depth_value << endl;
	publishKeyAndValue(depth_key, depth_ss.str());

}
//go through an array of contours and return the index of the contour with the greatest area
//takes in the address of the array, and the size
//used to find biggest target, essentially a filter for morphops-resistant noise
int ContourSelector(vector<vector<Point> > contours){
	int max_area_index = 0;
	for(int i = 0; i < contours.size(); i++){
		//if a contour's area is larger than the current max, update the current max
		if(contourArea(contours[i]) > contourArea(contours[max_area_index])){ 
			max_area_index = i;
			
		}
	}
	return max_area_index;
}

int main(int argc, char* argv[]){
	
	//create a matrix for each step to improve readability
	Mat src;
	Mat color_converted;
	Mat thresh;
	Mat morphops;
	Mat erodeElement;
	Mat dilateElement;
	Mat box_drawings;

	//setup camera
	VideoCapture capture;
	capture.open(1);
	capture.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT,FRAME_HEIGHT);

	//create trackbars
	createTrackbars();

	//drop the exposure of the camera to 2ms
	system("v4l2-ctl -d /dev/video1 -c exposure_auto=1 -c exposure_absolute=3");
	
	

	//zmq_setsockopt(&publisher, ZMQ_SNDHWM, &sndhwm, sizeof(sndhwm));
	publisher.bind("tcp://*:5803");

	//create vars that will contain the information we're trying to send
	double pan_adjust;
	double tilt_adjust;
	double depth;
	
	//check match mode, set a boolean to determine whether or not to use graphical information
	bool graphics_mode;
	graphics_mode = true;
	if(argc > 1){
		if(string(argv[1]) == "-match"){
			graphics_mode = false;
		}
	}
	
	//process the image and send results forever (not really)
	while(1) {

		//read image from camera and show it in its own window
		capture.read(src);
		
		if(graphics_mode){
			imshow("Source (1)", src);
		}

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
		if(graphics_mode){
			imshow("Converted (2)", color_converted);
	  	}


		//threshold the image using ranges from the trackbars and show the thresholded image
		inRange(color_converted, Scalar(H_MID - H_PM, S_MIN, V_MIN), Scalar(H_MID + H_PM, S_MAX, V_MAX), thresh);

		if(graphics_mode){
			imshow("Thresh (3)", thresh);
		}

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
		
		if(graphics_mode){
			imshow("Morphops (4)", morphops);	
		}

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
		box_drawings = Mat::zeros(thresh.size(), thresh.type());
	
		int target_index = ContourSelector(contours);
		//for each object in the noise reduced image, draw a box around it.
		//if it seems big enough (probably not noise and is the actual object, calculate the angle
		for( int i = 0; i< contours.size(); i++ ){
			
	       		Scalar color = Scalar(255, 255, 255);
						//show the contour of each contour
		        drawContours( box_drawings, contours, i, color, 1, 8, vector<Vec4i>(), 0, Point() );

		        //show each contour's rectangle
			Point2f rect_points[4]; 
			minRect[i].points( rect_points );
			for( int j = 0; j < 4; j++ ){
	          		line( box_drawings, rect_points[j], rect_points[(j+1)%4], color, 1, 8 );
			}
			
			//if the rectangle seems to be about the right size for the target and isn't noise, do some calculations with it
			WidthAndHeight box_measurements = CalcWidthAndHeight(rect_points[0], rect_points[1], rect_points[2]);
					
			if(i == target_index){
				
				//calculate depth and center of the target
				//draw some dots where the object is and where it should be if we're lined up perfectly
				//do some conversions between inches and pixels, and use some trig to get the depth and then the angle offset in both directions
				//keep the numbers to send to the roborio, but also put them at the top of the final display image
				depth = (focal_length*target_height) / box_measurements.height;
				Moments target_moment = moments(contours[i], false);
				Point2f target_center = Point2f(target_moment.m10/target_moment.m00, target_moment.m01/target_moment.m00);
				circle(box_drawings, target_center, 4, color, -1, 8, 0);
				circle(box_drawings, desired_location, 8, color, -1, 8, 0);
				double inches_offset_x = (((desired_location.x - target_center.x) / box_measurements.height)*target_height);
				pan_adjust = -1*cvtAngle(atan(inches_offset_x / depth));
				double inches_offset_y = (((desired_location.y - target_center.y) / box_measurements.height)*target_height);
				tilt_adjust = -1*cvtAngle(atan(inches_offset_y / depth));
				char print_buffer[50];
				sprintf(print_buffer, "%.1f and %.1f (%.0f)", pan_adjust, tilt_adjust, depth);
				putText(box_drawings, print_buffer, Point(0, 60), 2, 2, Scalar(255, 0, 255), 2);

				
			}
		 }

		//display the contour of each object, 
		//the rectangle of each object, 
	  //the two centers, and the angle offsets and depth in one image
		if(graphics_mode){
			imshow("Boxes (5)", box_drawings);
		}		

			
		// not needed, since ContourSelector should guarantee only one target
		//determine whether or not we found a single target and store it in a boolean for readability
		//***REPLACE WITH CORNER COUNTING CHECK OR AREA VS CONVEX HULL AREA PROPORTION CHECK***
		//***********************CURRENT CHECK ISN'T DOING MUCH********************************
		if(contours.size() > 0){
			found_single_target = true;
		}else{
			found_single_target = false;
		}
		
		//send angle offsets and depth to roborio	
		publishVisionCalculations(pan_adjust, tilt_adjust, depth);

		findAndSendTemperature();
    //unsigned int ms = duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
		//cout << ms << endl;
			
		
		//stop doing things forever if the escape button is pressed 
		int c = waitKey(10);
		if((char)c == 27){
			break;
		}
	}
	
	return 0;

}
