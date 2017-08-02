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

git clone git://github.com/opencv/opencv.git
cd opencv
git checkout 2.4
mkdir build
cd build
export JAVA_HOME=/usr/lib/jvm/java-6-oracle
cmake -DBUILD_SHARED_LIBS=OFF ..
  after cmake, check to see that under Java... ant has a path, JNI has a path, and Java tests says YES
  if not, those issues must be resolved before continuing
make -j8 (takes like 15-20 minutes rip)
