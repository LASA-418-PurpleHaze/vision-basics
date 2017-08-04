getting opencv and java working on the jetson:

first get java stuff working.

sudo apt-get install openjdk-8-jdk
sudo apt-get install openjdk-8-jre

now you can write and run a java program

1. make a .java file 
2. use javac filename.java
  now you have a filename.class file
3. use java filename or java filename.class
  yay it did the thing it was supposed to do

getting opencv working with java is a little bit more work

1. Use Jetson Hacks github to install opencv
http://www.jetsonhacks.com/2017/04/05/build-opencv-nvidia-jetson-tx2/
2.Run build.sh
3. Before you run the make script, turn java on 
 Change setup file to turn java ON: in CMakeVars.txt:
 BUILD_opencv_java=ON
4. Create your sample java script:
Example:
import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.MatOfDouble;
import org.opencv.core.MatOfPoint;
import org.opencv.core.MatOfPoint2f;
import org.opencv.core.MatOfPoint3f;
import org.opencv.core.Point;
import org.opencv.core.Range;
import org.opencv.core.Scalar;
import org.opencv.core.Size;
import org.opencv.imgproc.Imgproc;

class cvhw {


  public static void main(String[] args) {
    System.loadLibrary(Core.NATIVE_LIBRARY_NAME);
    System.out.println("Welcome to OpenCV " + Core.VERSION);
    Mat m = new Mat(5, 10, CvType.CV_8UC1, new Scalar(0));
    System.out.println("OpenCV Mat: " + m);
    Mat mr1 = m.row(1);
    mr1.setTo(new Scalar(1));
    Mat mc5 = m.col(5);
    mc5.setTo(new Scalar(5));
    System.out.println("OpenCV Mat data:\n" + m.dump());
  }

}

5. Use 'javac' to compile and 'java' to run but with special command-line options:
javac -cp /usr/share/OpenCV/java/opencv-320.jar:. filename.java
java -cp /usr/share/OpenCV/java/opencv-320.jar:. filename
