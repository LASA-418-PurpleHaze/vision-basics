# vision-basics
Basics of processing images with opencv on the jetson, works up to depth calculations and angle offsets

Most files in the tutorials folder require you to run it with the image to process as a parameter-
in a linux environment, after creating a runnable file, use ./tutorialname Path/to/image.jpg

Files in the calculations folder require you to connect a camera to your computer. You may need to change the line capture.open(1) to capture.open(0) or some other number for your computer to use the camera that you want.

regarding the c++ vs java, most of what is in this folder is just calling the opencv functions, so it would look very similar in both languages. I feel that the main difference you would see if this was in java is that there would be arrays instead of vectors, same concept though. Going back to "just calling the opencv functions", since the heart of what's being done is from a library, using c++ doesn't give you significantly more efficient run times, so to allow more people to contribute comfortably we can use java for all of this.

Resources:
the actual image processing module tutorials from opencv.org: 
http://docs.opencv.org/2.4/doc/tutorials/imgproc/table_of_content_imgproc/table_of_content_imgproc.html#table-of-content-imgproc

my notes on those tutorials:
https://docs.google.com/document/d/1Nj3dgHlAQltInzBOFdNpeFMixIai4Qn6QwiUMSALbUY/edit?usp=sharing

more info on opencv:
opencv.org

chief delphi responses for image processing questions are very high level and tend not to get into the details of the code. The white papers from team 900 may be daunting, but are worth a read

Kyle Hunslow shows applications of what you can learn from the image processing module tutorials, especially object tracking, along with links to his code in the description of his youtube videos
https://www.youtube.com/watch?v=cgo0UitHfp8&list=PLvwB65U8V0HHCEyW2UTyOJym5FsdqfbHQ



