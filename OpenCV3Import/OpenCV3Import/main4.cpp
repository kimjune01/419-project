#include <iostream>
#include "stdlib.h"
#include <fstream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <thread>
#include <chrono>


#include "GRT/GRT.h"

using namespace cv;
using namespace GRT;
using namespace std;
using namespace std::chrono;


// https://github.com/nickgillian/grt/blob/master/examples/ClassificationModulesExamples/SVMExample/SVMExample.cpp


//capture the video from webcam
VideoCapture cap(0);

// hsv control window initial values
int iLowH = 170;
int iHighH = 179;

int iLowS = 150;
int iHighS = 255;

int iLowV = 60;
int iHighV = 255;

void capThread(Mat &imgOriginal) {
    
    //Mat imgOriginal;
    
    bool bSuccess = cap.read(imgOriginal);
    
    if (!bSuccess){
        cout << "Cannot read a frame from video stream" << endl;
        
    }
    
}

void capThread2(Mat &imgOriginal) {
    
    //Mat imgOriginal;
    
    bool bSuccess = cap.read(imgOriginal);
    
    if (!bSuccess){
        cout << "Cannot read a frame from video stream" << endl;
        
    }
    
}

struct XYVV {
    float x, y;
    float velX, velY;
};

int doMagic() {
        
    int stor_X[1000];
    int stor_Y[1000];
    int stor_VX[1000];
    int stor_VY[1000];
    
    int iLastX = -1;
    int iLastY = -1;
    
    int frame_count = 0;
    
    //Capture a temporary image from the camera
    Mat imgTmp;
    cap.read(imgTmp);
    
    //Create a black image with the size as the camera output
    Mat imgLines = Mat::zeros( imgTmp.size(), CV_8UC3 );
    
    // create our GRT stuff
    const UINT N = 400;       //The number of dimensions in an input vector
    const UINT T = 1;       //The number of dimensions in a target vector
    
    RegressionData gestureData;
    
    gestureData.setInputAndTargetDimensions(N, T);
    gestureData.setDatasetName("GestureData");
    
    VectorFloat inputVector(N);
    VectorFloat targetVector(T);
    
    
    high_resolution_clock::time_point timex = high_resolution_clock::now();
    high_resolution_clock::time_point time2;
    long totalDuration;
    float frameRate;
    
    Mat imgOriginal;
    capThread(imgOriginal);
    
    while (true) {

        //TODO: blocking function. spawn thread.
        // read a new frame from video.
        //measure time of capture...
        high_resolution_clock::time_point time0 = high_resolution_clock::now();
        
        std::thread t1(capThread, std::ref(imgOriginal));
        
        XYVV frame;
        VectorFloat gesture;
        extract(imgOriginal
        
        //measure time of execution...
        high_resolution_clock::time_point time1 = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>( time1 - time0 ).count();
//        cout << "Milliseconds to capture: " << duration << endl;
        //
        
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
            
            // store current frame data
            stor_X[frame_count] = posX;
            stor_Y[frame_count] = posY;
            stor_VX[frame_count] = posX - iLastX;
            stor_VY[frame_count] = posY - iLastY;
            
            
            // copy last 100 frames and output to test pipeline
            if (frame_count > 98 && frame_count < 1000) {
                
                // copy last 100 frames to GRT test vector
                
                for (int i=0; i < 100; i++) {
                    inputVector[i*4] = stor_X[frame_count-99+i];
                    inputVector[i*4+1] = stor_Y[frame_count-99+i];
                    inputVector[i*4+2] = stor_VX[frame_count-99+i];
                    inputVector[i*4+3] = stor_VY[frame_count-99+i];
                }
                
                targetVector[0] = 0;
                gestureData.addSample(inputVector, targetVector);
                
                /*cout << "Saving data to file\n";
                 if( !gestureData.save("RegressionData.csv") ){
                 cout << "Failed to save training data to CSV file!\n";
                 return EXIT_FAILURE;
                 }
                 break; */
                
                
//                cout << "Frame: " << frame_count << endl;
            }
            else if (frame_count == 1000) {
                
                //copy last 100 frames to first part of page buffer and reset...
                
                cout << "Resetting frame buffer: " << endl;
                
                for (int i=0; i < 100; i++) {
                    
                    stor_X[i] = stor_X[901+i];
                    stor_Y[i] = stor_Y[901+i];
                    stor_VX[i] = stor_VX[901+i];
                    stor_VY[i] = stor_VY[901+i];
                    
                    inputVector[i*4] = stor_X[i];
                    inputVector[i*4+1] = stor_Y[i];
                    inputVector[i*4+2] = stor_VX[i];
                    inputVector[i*4+3] = stor_VY[i];
                    
                }
                
                targetVector[0] = 0;
                gestureData.addSample(inputVector, targetVector);
                
                
                // reset the frame counter
                
                frame_count = 0;
                
            }
            
            //Output some diagnostic info
            
            //cout << "X: " << posX << ", Y: " << posY << endl;
            //cout << "X-Last: " << abs(posX - iLastX) << ", Y-Last: " << abs(posY-iLastY) << endl;
            //cout << "capture ticks: " << capture_ticks << endl;
            
            iLastX = posX;
            iLastY = posY;
        }
        
        // add lines to image
        imgOriginal = imgOriginal + imgLines;
        
        // generate mirror image
        cv::Mat dst;
        cv::flip(imgOriginal,dst,0);
        Point2f src_center(dst.cols/2.0F, dst.rows/2.0F);
        cv::Mat rot_matrix = getRotationMatrix2D(src_center, 180.0, 1.0);
        cv::Mat rotated_img(Size(dst.size().height, dst.size().width), dst.type());
        warpAffine(dst, rotated_img, rot_matrix, dst.size());
        
        
        imshow("Thresholded Image", imgThresholded); //show the thresholded image
        
        imshow("flipped",rotated_img);
        
        frame_count++;
        

        //measuring time...
        time2 = high_resolution_clock::now();
        auto duration2 = duration_cast<milliseconds>( time2 - time1 ).count();
        cout << "Milliseconds to calculate stuff after capturing: " << duration2 << endl;
        
        // THREAD
        high_resolution_clock::time_point time3 = high_resolution_clock::now();
        t1.join();
        high_resolution_clock::time_point time4 = high_resolution_clock::now();
        auto duration4 = duration_cast<milliseconds>( time4 - time3 ).count();
        cout << "join time: " << duration4 << endl;
        
        
        totalDuration = duration_cast<seconds>( time2 - timex ).count();
        frameRate = float(frame_count) / float(totalDuration);
        cout << "average frame rate: " << frameRate << endl;
        
        int key = waitKey(1);
        
        if (key == 27) {
            cout << "esc key is pressed by user" << endl;
            break;
        }
        else if (key == 32) {
            imgLines.release();
            imgLines = Mat::zeros( imgTmp.size(), CV_8UC3 );
            cout << "lines eliminate" << endl;
        }
        
    }
    
    
    return 0;
    
 
}

int main( int argc, char** argv ) {
    
    if ( !cap.isOpened() ) {
        cout << "Cannot open the web cam" << endl;
        return -1;
    }
    
    //create a window called "Control"
    namedWindow("Control", CV_WINDOW_AUTOSIZE);
    
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
    
    
 
    doMagic();
   
    
    return 0;
}
