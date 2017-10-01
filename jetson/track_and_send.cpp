#include <stdlib.h>
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
//#include <jetsonGPIO.h>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <math.h>
#include <iomanip>
#include "zhelpers.hpp"
using namespace cv;
using namespace std;
using namespace zmq;

RNG rng(29292);
const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;
Mat cannied;
Mat src;
int H_MIN = 60;
int H_MID = 75;
int H_MAX = 255;
int S_MIN = 150;
int S_MAX = 255;
int V_MIN = 28;
int V_MAX = 115;
int focal_length = 717;
int H_PM = 10;
int erode_size = 1;
int dilate_size = 8;
int morphop_max = 10;
int target_height = 1;
int type_of_thresh;
bool found_single_target;

Point2f desired_location = Point2f(FRAME_WIDTH/2, FRAME_HEIGHT/2);
struct WidthAndHeight{
	int width;
	int height;
};


int DistFormula(Point point_a, Point point_b){
	return sqrt((point_a.x-point_b.x)*(point_a.x-point_b.x)+(point_a.y-point_b.y)*(point_a.y-point_b.y));
}

WidthAndHeight CalcWidthAndHeight(Point point_one, Point point_two, Point point_three){
	WidthAndHeight result;
	
	int length_one = DistFormula(point_one, point_two);
	int length_two = DistFormula(point_two, point_three);

	if(length_one > length_two){
		result.width = length_one;
		result.height = length_two;
	}else{ //length_one is < length_two
		result.width = length_two;
		result.height = length_one;
	}
	return result;
}

void trackbarCallback(int, void*){
	//doesn't need to do anything since the trackbars just change a global value
}

void createTrackbars(){
	namedWindow("HSV adjust", 0);
	createTrackbar("0: HSV | 1: RGB | 2: Gray", "HSV adjust", &type_of_thresh, 2, trackbarCallback);
	createTrackbar("H_MID", "HSV adjust", &H_MID, H_MAX, trackbarCallback);
//	createTrackbar("H_MIN", "HSV adjust", &H_MIN, H_MAX, trackbarCallback);
//	createTrackbar("H_MAX", "HSV adjust", &H_MAX, H_MAX, trackbarCallback);
	createTrackbar("H_PM", "HSV adjust", &H_PM, 255, trackbarCallback);
	createTrackbar("S_MIN", "HSV adjust", &S_MIN, H_MAX, trackbarCallback);
	createTrackbar("S_MAX", "HSV adjust", &S_MAX, H_MAX, trackbarCallback);
	createTrackbar("V_MIN", "HSV adjust", &V_MIN, H_MAX, trackbarCallback);
	createTrackbar("V_MAX", "HSV adjust", &V_MAX, H_MAX, trackbarCallback);
	createTrackbar("ERODE_SIZE", "HSV adjust", &erode_size, morphop_max, trackbarCallback);
	createTrackbar("DILATE_SIZE", "HSV adjust", &dilate_size, morphop_max, trackbarCallback);

}
double cvtAngle(double radianVal){

	return (180/3.14) * radianVal;
}

int main(){
Mat blurred;
   Mat erodeElement;
   Mat dilateElement;
   Mat thresh;
   Mat color_converted;
   VideoCapture capture;
   capture.open(1);
     capture.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
   capture.set(CV_CAP_PROP_FRAME_HEIGHT,FRAME_HEIGHT);
   createTrackbars();
	system("v4l2-ctl -d /dev/video1 -c exposure_auto=1 -c exposure_absolute=2");
   context_t context (1);
   socket_t publisher (context, ZMQ_PUB);
   publisher.bind("tcp://*:5804");
   double pan_adjust;
   double tilt_adjust;
   while(1){
	   capture.read(src);
	   imshow("Source (1)", src);
	   color_converted.create(src.size(), src.type());
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
	   imshow("Converted (2)", color_converted);
	  
	
	   inRange(color_converted, Scalar(H_MID - H_PM, S_MIN, V_MIN), Scalar(H_MID + H_PM, S_MAX, V_MAX), thresh);
		imshow("Thresh (3)", thresh);
	   erodeElement = getStructuringElement( MORPH_RECT,Size(erode_size, erode_size));
	    //dilate with larger element so make sure object is nicely visible
	   dilateElement = getStructuringElement( MORPH_RECT,Size(dilate_size, dilate_size));
	   //erode(thresh, thresh, erodeElement);
	   erode(thresh, thresh, erodeElement);
	   //dilate(thresh, thresh, dilateElement);
	   dilate(thresh, thresh, dilateElement);
	   imshow("Morphops (4)", thresh);
	
	   vector<vector<Point> > contours;
	   vector<Vec4i> hierarchy;
	
		findContours(thresh, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
	
		vector<RotatedRect> minRect(contours.size());
		for(int i = 0; i < contours.size(); i++){
			minRect[i] = minAreaRect(Mat(contours[i]));
		}
	
		Mat box_drawings = Mat::zeros(thresh.size(), thresh.type());
	
		for( int i = 0; i< contours.size(); i++ ){
	       		Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
		       // contour
		       drawContours( box_drawings, contours, i, color, 1, 8, vector<Vec4i>(), 0, Point() );
		       // rotated rectangle
		       Point2f rect_points[4]; minRect[i].points( rect_points );
			for( int j = 0; j < 4; j++ ){
	          		line( box_drawings, rect_points[j], rect_points[(j+1)%4], color, 1, 8 );
			}
			WidthAndHeight box_measurements = CalcWidthAndHeight(rect_points[0], rect_points[1], rect_points[2]);
			if(box_measurements.width > 10){
				double depth = (focal_length*target_height) / box_measurements.height;
				char buffer[50];
				sprintf(buffer, "%.2f", depth);
			//	putText(box_drawings, buffer, Point(0, 60), 2, 2, Scalar(255, 0, 255), 2);
				Moments target_moment = moments(contours[i], false);
				Point2f target_center = Point2f(target_moment.m10/target_moment.m00, target_moment.m01/target_moment.m00);
				circle(box_drawings, target_center, 4, color, -1, 8, 0);
				circle(box_drawings, desired_location, 8, color, -1, 8, 0);
				double inches_offset_x = (((desired_location.x - target_center.x) / box_measurements.height)*target_height);
				pan_adjust = -1*cvtAngle(atan(inches_offset_x / depth));
					 //cvtAngle(atan(((box_measurements.height * target_height) /
					//(desired_location.x - target_center.x))/ (depth)));
				double inches_offset_y = (((desired_location.y - target_center.y) / box_measurements.height)*target_height);
				tilt_adjust = -1*cvtAngle(atan(inches_offset_y / depth));
				char print_pan_adjust[50];
				sprintf(print_pan_adjust, "%.1f and %.1f (%.0f)", pan_adjust, tilt_adjust, depth);
				putText(box_drawings, print_pan_adjust, Point(0, 60), 2, 2, Scalar(255, 0, 255), 2);
				
			}
		 }

	      imshow("Boxes (5)", box_drawings);
	      if(contours.size() == 1){
			found_single_target = true;
		}else{
			found_single_target = false;
		}
		
		char buffer[80];
		stringstream data_ss;
		//		data_ss << "fst:" << found_single_target << 
		//					":pa:" << setprecision(3) << pan_adjust //<< 
		//					":ta:" << setprecision(3) << tilt_adjust << endl;

				data_ss << found_single_target << 
					" " << setprecision(3) << pan_adjust << 
					" " << setprecision(3) << tilt_adjust << endl;

				string data_s = data_ss.str();
		cout << "sending: " << 	data_s << endl;
		s_send(publisher, data_s);
	    /*  stringstream to_send;
	      to_send << "fst:" << found_single_target << 
			":pa:" << setprecision(3) << yaw_adjust << 
			":ta:" << setprecision(3) << pitch_adjust << endl;
		message_t to_send_m((void*)to_send.str().c_str(), to_send.str().size()+1, NULL);
		cout << "sending: " << to_send.str();
		socket.send(to_send_m);
		s_sendmore(socket, "A");
`		s_send(socket, "ayyy");
		s_sendmore(socket, "B");
		s_send(socket, "booo");
		*/
		
		int c = waitKey(50);
		if((char)c == 27){
			break;
		}
	}
return 0;
}
