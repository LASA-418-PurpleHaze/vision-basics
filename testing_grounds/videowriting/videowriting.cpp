//capture images from the camera
//process them to find where the vision target is
//publish information about the target for the roborio to read

//uses an unknown camera height and target height, so it requires the full target to be visible for accurate information
//once camera height and target height are fixed, more flexible geometry can be used

#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio/videoio.hpp>
#include <string>

using namespace std;
using namespace cv;

int main(int argc, char** argv){
	// Load input pic
	Mat image_src = imread(argv[1]);
    Mat image_src_two = imread(argv[2]);
	Mat image_src_three = imread(argv[3]);
	
	// Setup output video
	VideoWriter output_cap(argv[4], 
	               CV_FOURCC('M','J','P','G'),
	               120,
	               cv::Size(480, 640));
	
	if (!output_cap.isOpened())
	{
		std::cout << "!!! Output video could not be opened" << std::endl;
		return -1;
	}


	// Loop to read from input and write to output
	Mat frame(640,480, CV_8UC3);
	Mat frame_two(640, 480, CV_8UC3);
	Mat frame_three(640, 480, CV_8UC3);


	Mat bnw_frame(640, 480, CV_8UC1);
	Mat bnw_frame_two(640, 480, CV_8UC1);
	Mat bnw_frame_three(640, 480, CV_8UC1);
	
	resize(image_src_two, frame_two, frame_two.size(), 0, 0, CV_INTER_LINEAR);
    cvtColor(frame_two, bnw_frame_two, CV_BGR2GRAY);
	cvtColor(bnw_frame_two, bnw_frame_two, CV_GRAY2BGR);

	resize(image_src_three, frame_three, frame_three.size(), 0, 0, CV_INTER_LINEAR);
    cvtColor(frame_three, bnw_frame_three, CV_BGR2GRAY);
	cvtColor(bnw_frame_three, bnw_frame_three, CV_GRAY2BGR);

	resize(image_src, frame, frame.size(), 0, 0, CV_INTER_LINEAR);
    cvtColor(frame, bnw_frame, CV_BGR2GRAY);
	cvtColor(bnw_frame, bnw_frame, CV_GRAY2BGR);
	
	const int thing = 47;
	cout << "about to start writing" << endl;
	for(int i = 0; i < 500; i++)
	{       
		if(i%thing > thing/2) {
	    	output_cap << frame;
			output_cap << frame_two;
			output_cap << frame_three;
		} else {
			output_cap << bnw_frame;
			output_cap << bnw_frame_two;
			output_cap << bnw_frame_three;}
	}
	cout << "done writing" << endl;
	
	output_cap.release();
	return 0;
}
