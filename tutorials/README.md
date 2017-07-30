While going through the image processing tutorials on the opencv website, I ran the code in this folder on some images
Additionally, I took some notes about the different features which can be found below, or at docs.google.com/document/d/1Nj3dgHlAQltInzBOFdNpeFMixIai4Qn6QwiUMSALbUY/edit?usp=sharing

The google docs link has pictures that may help you to visualize what's going on

1. Blurring: Gaussian, Median, Bilateral
Why: Smoothing images (blurring) reduces noise
How:
    Normalized Box Filter - kernel with all ones, takes average
    Gaussian Filter - Like normalized box filter, but weights closer neighbors much heavier than farther neighbors
    Median Filter - replaces each pixel with the median of its neighboring pixels. Always get a value from the original image
    Bilateral Filter - two components: gaussian, and edge preserve. Does gaussian but also takes into account the intensity of variation. If something varies heavily, it is probably an edge, so don’t use that to blur the pixel far away from it. Shadows and edges are preserved, but texturing and shading is removed.

    Watch video for explanation of how bilateral filter works (was never fully explained)


2. Morphological Operations
Erosion: takes the minimum (probably black) pixel in the kernel and replaces the anchor with it. Enlarges the black portion and reduces the white portion. 

Dilation: takes the maximum (probably white) pixel in the kernel and replaces the anchor with it.
Enlarges the white portion and reduces the black portion

Opening: erosion followed by dilation. (Dilation of the erosion). Merges black blobs into each other and opens the black gaps. 

Closing: dilation followed by erosion. (Erosion of the dilation). Merges white blobs into each other and closes the black gaps

Morphological Gradient: Dilation minus erosion. White minus white is black. All shared white areas become black, but the differences are white minus black, so still is left as white. Finds the outline of an image

Top Hat: source minus opening. Opening gets darker with larger kernel sizes, so with larger kernel sizes, top hat becomes closer and closer to original image as you subtract off less and less.
Black Hat: source minus closing. 


Morph_elem = 0/1/2 which is rect/cross/ellipse
Size = size of window
Point = anchor point of kernel
Operation = 0/1/2/3/4/5/6 which is erosion/dilation/opening/closing/gradient/tophat/blackhat

Mat element = getStructuringElement( morph_elem, Size( 2*morph_size + 1, 2*morph_size+1 ), Point( morph_size, morph_size ) );
morphologyEx( src, dst, operation, element );

What is the difference between eroding and erosion and eroding with a larger kernel?

3. Image Pyramids
pyrUp: takes the current image to be upsized, the destination of the upsized image, and the size of the destination image. Adds columns and rows and averages pixels using a gaussian kernel. Blurs the image a bit.

pyrDown: takes the current image to be downsized, the destination of the downsized image, and the size of the destination image. Removes columns and rows and then averages pixels (a blur) 

4. Thresholding
cvtColor, pass some stuff and then pass things like CV_BGR2GRAY or CV_HSV2BGR or anything of the sort
Threshold operation takes source image, destination image, threshold value, max value (255), threshold type (choose from below, 0/1/2/3/4) 


5. Linear Filters
Initialize a kernel as a Matrix, fill it however you want, Mat:ones is good for a blur. Larger kernel blur means more of a blur

When ready, use filter2D
filter2D takes… source image / destination image / depth of destination (use -1 to say it’s the same as source) / kernel / delta (to be added to each pixel, default is 0) / border

6. Borders
RNG is a random number generator type ex. RNG rngName(418);

Can define a Scalar as a BGR color ex. Scalar value = Scalar(33, 118, 254);

copyMakeBorder takes… source image / destination image / top / bottom / left / right (sizes of the borders) / border type (BORDER_CONSTANT or BORDER_REPLICATE) / value (color as a scalar)

7. Sobel Derivatives
For x direction, Sobel kernel is     -1 0 1        scharr kernel is    -3 0 3
                    -2 0 2                          -10 0 10
            -1 0 1                     3 0 3
Scharr kernel accentuates the derivative, and for a kernel size of 3 it gets a more accurate edge detection.
Gradient = derivative = slope
Apply gaussian/any blur first to reduce noise (otherwise there’ll be more changes in intensity)
Take derivatives in x direction and y direction
Take the abs value of them (otherwise it just breaks)
Put the abs values on top of each other (simple mean works well enough) and that gets most of the edges. (you can also account for the diagonal directions, but it isn’t necessary)
Some scaling is necessary since you can only display 8bit? (differences in types of pixels, like CV_8U or something)

convertScaleAbs(input array, output array) calculates absolute values and converts the result to 8-bit

addWeighted(thing one, weight from 0 to 1, thing 2, weight from 0 to 1, extra delta, output array)

Sobel takes… input image / output image / depth of output (CV_8U or something like that) / order of x derivative / order of y derivative / size of the kernel (must be 1,3,5,7) / scale factor (optional) / delta (additional value added to every result) / border type

8. Laplacian
Literally just sobel, but instead of maximums of 1st derivatives, it takes the zeroes of the second derivative. Sometimes it gets zeroes in places that aren’t 1st derivative maximums (meaningless noise added). Shows a double line for edges. Internally calls Sobel.

Laplacian takes… source image / destination image / depth of destination / kernel size (must be positive and odd) / optional scale factor / optional delta / border type

9. Canny
Set up by Filter noise with a Gaussian or Normalized Box blur
What canny does:
Sobel operator in x and y directions.
Combine
Threshold to get “bright” or “strong” edges
Apply hysteresis with an upper and lower threshold
Pixel gradients below lower are rejected, above higher are accepted
In between, only accepted if they are connected to a pixel above the upper threshold

Blur grayscale image of source
Ratio of low to high threshold should be 2:1 or 3:1. When inputting high threshold to canny, use lowthreshold*2 or *3
Canny takes… input image with edges / output destination for edges / low threshold / high threshold / kernel size (positive and odd)
10. Hough Line

Standard Hough and Probabilistic Hough Line Transform

Takes cartesian points and converts to polar coordinates and intersections in polar indicate a point, which represents a line in cartesian.

How to use:
Threshold image
Blur image
Detect edges (canny works best)
Create a vector of vec4i (vector of lines ex. vector<Vec4i> lines;)
Apply HoughLinesP with the output being a vector of lines (houghlines exists, but it gives full lines, not segments. Not as useful for us)
HoughLinesP takes… input image / vector of lines / distance resolution (1) / angle resolution (CV_PI/180) / threshold / min line length / max line gap

11. Hough Circle
For setup, blur first to reduce noise
Does canny internally
Finds circle with xcenter, ycenter, and radius
Puts all circles found into a vector of these values (vector<Vec3f> circles)
HoughCircles takes… input image / output vector / detection method (CV_HOUGH_GRADIENT is the only one available) / inverse ratio of resolution (just use 1) / minimum distance between detected centers / upper threshold for internal canny edge detector / threshold for center detection / minimum radius to be detected (if unknown put 0 as default) / maximum radius to be detected (if unknown put 0 as default)

12. Remapping
Remap uses custom defined map_x and map_y to remap an image. Usually used for scaling, or flipping an image.

Remap takes…

src: Source image
dst: Destination image of same size as src
map_x: The mapping function in the x direction. It is equivalent to the first component of 
map_y: Same as above, but in y direction. Note that map_y and map_x are both of the same size as src
CV_INTER_LINEAR: The type of interpolation to use for non-integer pixels. This is by default.
BORDER_CONSTANT: Default

13. Affine Transformations
Make a source triangle, make a destination triangle based on mathematical differences between source and destination (like source*.85). These triangles are defined by Point2F vectors with 3 points in each vector. 
Call getAffineTransform with these two triangles and store the result in a matrix, which will have the properties needed for the warp. Then call warpAffine with the source image, destination matrix, and the warp properties.
Destination matrix now has the warped image.

14. Histogram Equalization
Improve contrast in an image
Histogram quantifies the number of pixels with each intensity
Usage: equalizeHist(input image, output image);
Very simple. 
Good for washed out images, not dark images (doesn’t help tremendously in edge detection)


15. Histogram Calculation
Histogram terms:
Dims - number of parameters (for intensity, it’s 1)
Bins - number of subdivisions (can be like 20)
Range - range of data (for intensity, it can be [0, 255])

For different colors, split the channels, use calcHist to actually generate the histograms, use normalize to get the range that you want, and loop through the bins and plot lines for each channel using the line command

16. Histogram Comparison
Generate histograms from images to compare
Compare histograms using 1 of 4 methods
Returns either error/difference from original, or returns value from 0 to 1 with 0 being different and 1 being the same.

Can be used to see if an unknown situation is similar to a known situation (say you take samples from many different places, set the bot down and have it take an image of the same thing. Whichever environment it’s closest to, the program would use the magic range/settings calibrated to the similar environment)

17. Back Projection
The application directly seen is similar to histogram comparison application, but back projection seems slightly better suited. 
Takes the image you give it, you give just the image of the hue/shade/color that you want and it takes a histogram of that. 
Then use that histogram (perfect match) and compare it to each pixel and see how probable it is that the pixel of the questionable image matches the original/intended image. More probable, brighter the pixel. 


18. Template Matching
Takes an image and a template.
Calculates a matching results function using 1 of 5 techinques
Slides the function across the image 
Normalizes output and returns location with the highest matching probability



19. Contours
Uses two main functions:
findContours
    Takes canny output as input (canny makes it good, doesn’t need it)
    Creates a vector of vectors of points. A contour is defined as a vector of points, so there are multiple contours that can be found.
    
drawContours
    Takes the output from findContours and draws to the screen with some help


20. Convex Hull
Uses sklansky’s algorithm

Any vertex has an interior angle greater than 0 and less than a straight line

Takes a vector of points (like a contour)
Outputs a vector of points (a convex hull)

To use => convert color, blur, threshold/edge detect, find contours, convexhull of each contour.

21. Bounding Boxes/Bounding Circles
For Both:
Cvtcolor => grayscale
Blur => reduce noise
Threshold => get edges (don’t need canny, since thresholding synergizes with findContours by creating large differences in objects)
find Contours => get contours
For each contour,
Approxpolydp => close contours/connect contours to close gaps (not necessary, but might help)


For box:
Make storage space for a rectangle
Loop through contours and use bounding rectangle storage area = boundingRect(some specific contour)
You now have a bounding rectangle

For circle:
Make storage space for a center (point2f) and for a radius (float)
Loop through contours and use minEnclosingCircle(specific contour, center storage, radius storage)
You now have an enclosing circle
22. Image moments
Center of mass of a contour

23. Point in Polygon
See if a point is inside, on the, or outside a contour. Also see how far from the contour a point is.


