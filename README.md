# vision-basics
Basics of processing images with opencv on the jetson, works up to depth calculations and angle offsets

Most files in the tutorials folder require you to run it with the image to process as a parameter-
in a linux environment, after creating a runnable file, use ./tutorialname Path/to/image.jpg

Files in the calculations folder require you to connect a camera to your computer. You may need to change the line capture.open(1) to capture.open(0) or some other number for your computer to use the camera that you want.
