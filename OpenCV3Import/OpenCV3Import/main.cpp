#include <iostream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "GRT/GRT.h"

using namespace cv;
using namespace std;

int main( int argc, char** argv ) {
  
//  cout << "GRT Version: " << GRT_VERSION << endl;
//  cout << "OpenCV Version: " << CV_VERSION << endl;
//  return 1;
  
  //capture the video from webcam
  VideoCapture cap(0);
  //720 rows * 1280 cols * ?? fps
//  cap.set(CV_CAP_PROP_FPS,30);
//  cap.set(CV_CAP_PROP_FRAME_WIDTH, cap.get(CV_CAP_PROP_FRAME_WIDTH)/2);
//  cap.set(CV_CAP_PROP_FRAME_HEIGHT, cap.get(CV_CAP_PROP_FRAME_HEIGHT)/2);
  //720/2 rows * 1280/2 cols * 30 fps
  if ( !cap.isOpened() ) {
    cout << "Cannot open the web cam" << endl;
    return -1;
  }
  
  //create a window called "Control"
  namedWindow("Control", CV_WINDOW_AUTOSIZE);
  
  int iLowH = 170;
  int iHighH = 179;
  
  int iLowS = 150;
  int iHighS = 255;
  
  int iLowV = 60;
  int iHighV = 255;
  
  //Create trackbars in "Control" window
  //Hue (0 - 179)
  createTrackbar("LowH", "Control", &iLowH, 179);
  createTrackbar("HighH", "Control", &iHighH, 179);
  //Saturation (0 - 255)
  createTrackbar("LowS", "Control", &iLowS, 255);
  createTrackbar("HighS", "Control", &iHighS, 255);
  //Value (0 - 255)
  createTrackbar("LowV", "Control", &iLowV, 255);
  createTrackbar("HighV", "Control", &iHighV, 255);
  
  int iLastX = -1;
  int iLastY = -1;
  
  //Capture a temporary image from the camera
  Mat imgTmp;
  cap.read(imgTmp);
  
  
  
  //Create a black image with the size as the camera output
  Mat imgLines = Mat::zeros( imgTmp.size(), CV_8UC3 );;
  
  
  while (true) {
    Mat imgOriginal;
    
    //TODO: blocking function. spawn thread.
    // read a new frame from video.
    bool bSuccess = cap.read(imgOriginal);
    
    if (!bSuccess){
      cout << "Cannot read a frame from video stream" << endl;
      break;
    }
    
    //Convert the captured frame from BGR to HSV
    Mat imgHSV;
    cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV);
    
    Mat imgThresholded;
    inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded); //Threshold the image
    
    //morphological opening (removes small objects from the foreground)
    erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate( imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    
    //morphological closing (removes small holes from the foreground)
    dilate( imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    
    //Calculate the moments of the thresholded image
    Moments oMoments = moments(imgThresholded);
    
    double dM01 = oMoments.m01;
    double dM10 = oMoments.m10;
    double dArea = oMoments.m00;
    
    // if the area <= 10000 assume no object in the image
    // it's because of the noise, the area is not zero
    if (dArea > 10000) {
      //calculate the position of the ball
      int posX = dM10 / dArea;
      int posY = dM01 / dArea;
      
      //Draw a red line from the previous point to the current point
      if (iLastX >= 0 && iLastY >= 0 && posX >= 0 && posY >= 0) {
        line(imgLines, Point(posX, posY), Point(iLastX, iLastY), Scalar(0,0,255), 2);
      }
      
      //TODO: store the coordinates in a vector
      iLastX = posX;
      iLastY = posY;
      cout << "X: " << posX << ", Y: " << posY << endl;
    }
    
    imshow("Thresholded Image", imgThresholded); //show the thresholded image
    
    imgOriginal = imgOriginal + imgLines;

//    imshow("Original", imgLines); //show the original image
    imshow("Original", imgOriginal); //show the original image

    if (waitKey(30) == 27) {
      cout << "esc key is pressed by user" << endl;
      break; 
    }
  
  }
  return 0;
}
