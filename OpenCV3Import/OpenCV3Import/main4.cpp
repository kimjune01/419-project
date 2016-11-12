#include <iostream>
#include "stdlib.h"
#include <fstream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <thread>
#include <chrono>
#include "Normalizer.hpp"

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

int FRAMES_PER_GESTURE = 100;

Normalizer normalizer;

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


const int PAGE_BUFFER_SIZE = 1000;




// initialize precapture window
int doCapture() {
    
    int iLastX = -1;
    int iLastY = -1;
    
    VectorFloat frameFloats = VectorFloat(FRAMES_PER_GESTURE*4 + 1);
    
    //Capture a temporary image from the camera
    Mat imgTmp;
    cap.read(imgTmp);
    
    //Create a black image with the size as the camera output
    Mat imgLines = Mat::zeros( imgTmp.size(), CV_8UC3 );;
    
    // init counter for frames since last movement
    int frame_counter = 0;
    int sample_counter = 0;
    
    Mat imgOriginal;
    imgOriginal = imgTmp;
    
    bool now_sampling = false;
    
    high_resolution_clock::time_point flopTime = high_resolution_clock::now();
    
    while (true) {
        
        
        //TODO: blocking function. spawn thread.
        // read a new frame from video.
        
        flopTime = high_resolution_clock::now();
        
        std::thread t1(capThread, std::ref(imgOriginal));

        
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
            
//            flopTime = high_resolution_clock::now();
            
            // if < 30 for x frames, then start capture sequence
            
            
            if (now_sampling == true && frame_counter < FRAMES_PER_GESTURE) {
//                 cout << "FRAME CAPTURE INITIATED" << endl;
                
                
                if (iLastX >= 0 && iLastY >= 0 && posX >= 0 && posY >= 0) {
                    line(imgLines, Point(posX, posY), Point(iLastX, iLastY), Scalar(0,0,255), 2);

                    //TODO: test boundary times...
                    high_resolution_clock::time_point tempTime = flopTime;
                    flopTime = high_resolution_clock::now();
                    long mSec = duration_cast<milliseconds>( flopTime - tempTime ).count();
                    if (mSec == 0 || mSec > 100) {
                        cout << "frame_counter: " << frame_counter << endl;
                    }
                    //TODO: record each frame into a gesture vector...
                    int baseIndex = frame_counter * 4;
                    frameFloats[baseIndex + 1] = float(posX);
                    frameFloats[baseIndex + 2] = float(posY);
                    frameFloats[baseIndex + 3] = float(iLastX - posX)/float(mSec);
                    frameFloats[baseIndex + 4] = float(iLastY - posY)/float(mSec);

                }
                                frame_counter++;
            } else if (frame_counter == FRAMES_PER_GESTURE) {
                imgLines = Mat::zeros( imgTmp.size(), CV_8UC3 );
                sample_counter++;
                frame_counter++;
                cout << "Finished sampling! :)" << endl;
                
                normalizer.normalize(frameFloats);
                //foo
                // start write file
                ofstream myfile;
                myfile.open ("training.csv", ios::app);
                
                // format of 1d vector
                
                for (int i = 0; i < 401; i++) {
                
                    myfile << frameFloats[i] << " ";
                
                }
                
                myfile << endl;
                    
                myfile.close();
                
               
                
                //TODO: clear gesture vector...
            }
            
            
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
        
        t1.join();
        
        // imshow("Original", imgLines); //show the original image
        // imshow("Original", imgOriginal); //show the original image
        
        int key_press = waitKey(1);
        
        if (key_press == 27) {
            cout << "esc key is pressed by user" << endl;
            break;
        }
        else if (key_press == 32) {
            now_sampling = true;
            frame_counter = 0;
            cout << "capture" << endl;
        }
        
    }
    
    return 0;
}



int doMagic() {
    
    XYVV frameArray[PAGE_BUFFER_SIZE];
    
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
    
    high_resolution_clock::time_point betweenTime;
    
    
    
    while (true) {

        high_resolution_clock::time_point time0 = high_resolution_clock::now();
        
        std::thread t1(capThread, std::ref(imgOriginal));
        
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
            
            high_resolution_clock::time_point beforeT = betweenTime;
            high_resolution_clock::time_point betweenTime = high_resolution_clock::now();
            long mSec = duration_cast<milliseconds>( beforeT - betweenTime ).count();;
            
            // store current frame data
            frameArray[frame_count].x = posX;
            frameArray[frame_count].y = posY;
            frameArray[frame_count].velX = (iLastX - posX)/float(mSec);
            frameArray[frame_count].velY = (iLastY - posY)/float(mSec);
            if (iLastX == -1 && iLastY == -1) {
                frameArray[frame_count].velX = 0;
                frameArray[frame_count].velY = 0;
            }
    
            
            // put it into mutable array
            
           // cout << "x: "<<frame.x<< " y: "<<frame.y<< " velX: "<<frame.velX<< " velY: "<<frame.velY<< endl;
            
            // copy last 100 frames and output to test pipeline
            if (frame_count > 98 && frame_count < PAGE_BUFFER_SIZE) {
                
                // copy last 100 frames to GRT test vector
                
                for (int i=0; i < FRAMES_PER_GESTURE; i++) {
                    inputVector[i*4] = frameArray[frame_count-99+i].x;
                    inputVector[i*4+1] = frameArray[frame_count-99+i].y;
                    inputVector[i*4+2] = frameArray[frame_count-99+i].velX;
                    inputVector[i*4+3] = frameArray[frame_count-99+i].velY;
                }
                
        
                
                /*cout << "Saving data to file\n";
                 if( !gestureData.save("RegressionData.csv") ){
                 cout << "Failed to save training data to CSV file!\n";
                 return EXIT_FAILURE;
                 }
                 break; */
                
                
//                cout << "Frame: " << frame_count << endl;
            }
            else if (frame_count == PAGE_BUFFER_SIZE) {
                
                //copy last FRAMES_PER_GESTURE frames to first part of page buffer and reset...
                
                cout << "Resetting frame buffer: " << endl;
                
                for (int i=0; i < FRAMES_PER_GESTURE; i++) {
                    
                    frameArray[i].x = frameArray[901+i].x;
                    frameArray[i].y = frameArray[901+i].y;
                    frameArray[i].velX = frameArray[901+i].velX ;
                    frameArray[i].velY = frameArray[901+i].velY;
                    
                    inputVector[i*4] = frameArray[i].x;
                    inputVector[i*4+1] = frameArray[i].y;
                    inputVector[i*4+2] = frameArray[i].velX;
                    inputVector[i*4+3] = frameArray[i].velY;
                    
                }
                
                
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
    
    
 
    doCapture();
   
    
    return 0;
}
