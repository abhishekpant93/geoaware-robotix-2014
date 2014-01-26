#include "opencv2/highgui/highgui.hpp"
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <vector>
#include <stdio.h>

#define PI 3.1415926

using namespace cv;
using namespace std;


float abs( float x , float y){
  if((x-y) < 0)
    return y-x;
  else
    return x-y;

}

vector<Vec6f> getLineSegments( Mat& edgeIm , vector<Vec2f> clines){

  vector<Vec2f> lines;
  bool exists = false;
  
  //Removing lines close to each other
  for(int i = 0; i < clines.size() ; i++){
    exists = false;
    float crho = clines[i][0], ctheta = clines[i][1];
    for( int j = 0 ; j < lines.size() ; j++){
      float rho = lines[j][0], theta = lines[j][1];
      if( abs(rho - crho) <= 10 && abs(theta - ctheta) <= .05){
        crho = (rho + crho) /2;
        ctheta = (theta + ctheta)/2;
        exists = true;
      }
    }
    if(!exists){
      lines.push_back(Vec2f( crho , ctheta));
    }
  }
  cout<<"There are total " <<  lines.size() << " lines"<<endl;
  imshow("Getting line segments on " , edgeIm);
  //remove contours that are close by
  int kernel = 5;
  vector<Vec6f> segments;
  Point p1 , p2;
  Mat lineTest;
  edgeIm.copyTo(lineTest);
  //equation is p = xcostheta + ysintheta
  for(int i = 0 ; i< lines.size() ; i++){

    float rho = lines[i][0], theta = lines[i][1];
    double a = cos(theta), b = sin(theta);
    bool setP1 , setP2;
    setP1 = setP2 = false;

    double y = 0;
    double  x = (rho /a) - y*( b/a);

    if(x >=0 && x< edgeIm.cols){
      if(!setP1){
        p1 = Point( x , y);
        setP1 = true;
      }
      else if(!setP2){
        p2 = Point(x , y);
        setP2 = true;
      }
      else
        cout<<"Error"<<endl;
    }
    y = edgeIm.rows -1;
    x = (rho /a) - y*( b/a);

    if(x >=0 && x< edgeIm.cols){
      if(!setP1){
        p1 = Point( x , y);
        setP1 = true;
      }
      else if(!setP2){
        p2 = Point(x , y);
        setP2 = true;

      }
      else cout<<"Error"<<endl;
    }

    x = edgeIm.cols -1;
    y = (rho /b) - x*( a/b);
    

    if(y >=0 && y< edgeIm.rows){
      if(!setP1){
        p1 = Point( x , y);
        setP1 = true;
      }
      else if(!setP2){
        p2 = Point(x , y);
        setP2 = true;
      }
      else cout<<"Error"<<endl;
    }
    x = 0;
    y = (rho /b) - x*( a/b);

    if(y >=0 && y< edgeIm.rows){
      if(!setP1){
        p1 = Point( x , y);
        setP1 = true;
      }
      else if(!setP2){
        p2 = Point(x , y);
        setP2 = true;
      }
      else cout<<"Error"<<endl;
    }

    //Find the points using the funda that the line will touch two edges
 
    LineIterator lit( edgeIm , p1 , p2 );
    Point start_point , end_point;
    bool isOnLine = false;
    Point curr_pos;
    for(int j = 0; j < lit.count; j++, ++lit)
      {
        curr_pos = lit.pos();
        bool isBlack = false;
        for (int xi = curr_pos.x  - kernel ; xi <= curr_pos.x + kernel ; xi++)
          {
            for (int yi = curr_pos.y  - kernel ; yi <= curr_pos.y + kernel ; yi++)
              {
                if( xi < 0 || xi >= edgeIm.cols || yi < 0 || yi >= edgeIm.rows )
                  continue;
                Scalar intensity = edgeIm.at<uchar>(yi , xi);
                      
                if( intensity.val[0] == 0){
                  isBlack = true;
                }
              }
          }
        if(isBlack){
          if(isOnLine == false){
            start_point = curr_pos;
            isOnLine =true;
          }
        }
        else{
          if(isOnLine){
            end_point = curr_pos;
            isOnLine = false;
            segments.push_back(Vec6f(start_point.x , start_point.y , end_point.x , end_point.y  , rho , theta));
          }
        }
        
      }
    //end checking. just in case
    if(isOnLine){
      end_point = curr_pos;
      isOnLine = false;
      segments.push_back(Vec6f(start_point.x , start_point.y , end_point.x , end_point.y ,rho , theta));
    }
  }

  vector<Vec6f> finalsegments;
  for(int i =0 ; i < segments.size(); i++)
    {
      double length =  cv::norm(cv::Mat(Point(segments[i][0] , segments[i][1])),cv::Mat(Point(segments[i][2] , segments[i][3])));
      if(length > kernel*5)
        {
          finalsegments.push_back(segments[i]);
          cout<<length<<endl;
        }

    }

  
  cout<<"There are total " <<finalsegments.size() << " segments"<<endl;


  return finalsegments;
}


void drawLineSegments(Mat& ime , vector<Vec6f> segments ,  cv::Scalar color=cv::Scalar(255)){

  // Draw the lines
  std::vector<cv::Vec6f>::const_iterator it2= segments.begin();
  while (it2!=segments.end()) {
    cv::Point pt1((*it2)[0],(*it2)[1]);        
    cv::Point pt2((*it2)[2],(*it2)[3]);
    cv::line( ime, pt1, pt2, color, 10 );
    ++it2;	
  }
}


int houghVote = 60;
int cannyLower = 50;
int cannyHigher = 250;
Mat image;
string argp;
VideoCapture capture;




void removeSymbols(Mat& img){
  
  Mat hsv;
  cvtColor(img ,hsv ,CV_RGB2HSV);

  Mat edges;
  std::vector<std::vector<cv::Point> > contours;
  cv::Canny(img, edges, 50, 200, 3 );

  cv::findContours(edges, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

  for( int i=0;i<contours.size();++i)
    {

      int colored = 0;
      int black =1;
      int white = 1;
      Scalar background;
      Rect bRect =  boundingRect( contours[i]);
      for( int j = bRect.x ; j< bRect.x + bRect.width ; j++)
        for(int k = bRect.y ; k < bRect.y + bRect.height; k++){
          Vec3b intensity = hsv.at<Vec3b>(k , j);
          int hue = (int)intensity.val[0];
          int sat = (int)intensity.val[1];
          int val = (int)intensity.val[2];
          if(sat > 50 && val > 50)
            colored++;
          else if( val < 10)
            black++;
          else{
            white++;
          }
        }

      if(colored > black + white){
        if(black > white)
          rectangle(img,  bRect, Scalar(0 ,0 ,0) ,-1 );
        else
          rectangle(img, bRect , Scalar(135 , 135 ,135) , -1 );
      }
      
    }
  
  imshow("nosybold", img);
}


void detectTunnel(vector<Vec6f> segments){

  int tunnelGap= 50;

  for ( int i =0 ; i< segments.size(); i++)
    for( int j = i+1 ; j <segments.size(); j++){
      float distance = 99999999;
      if( abs(segments[i][4]  - segments[j][4] < .02) &&   abs(segments[i][5]  - segments[j][5]) < .02)
        {

          float ndist =  norm(Mat(Point(segments[i][0] , segments[i][1])),Mat(Point(segments[j][0] , segments[j][1])));
          distance = ndist < distance ? ndist : distance;
          ndist =  norm(Mat(Point(segments[i][0] , segments[i][1])),Mat(Point(segments[j][2] , segments[j][3])));
          distance = ndist < distance ? ndist : distance;
          ndist =  norm(Mat(Point(segments[i][2] , segments[i][3])),Mat(Point(segments[j][0] , segments[j][1])));
          distance = ndist < distance ? ndist : distance;
          ndist =  norm(Mat(Point(segments[i][2] , segments[i][3])),Mat(Point(segments[j][2] , segments[j][3])));
          distance = ndist < distance ? ndist : distance;
          if(distance > tunnelGap){
            cout<<"Tunnel detected" <<endl;
          }
        }
    }
}


float slope( Point p1 , Point p2){
  return   atan((p2.y - p1.y)/(p2.x - p1.x)) * 180 / PI;
}

void LaneDetect(){

  if (image.empty())
    return;
  Mat gray;
  //remove colors
  removeSymbols(image);
  cvtColor(image,gray,CV_RGB2GRAY);
  Rect roi(0,image.rows/3,image.cols-1,image.rows - image.rows/3);// set the ROI for the image
  //set ROI dynamically
  Mat imgROI = image(roi);
  //Mat imgROI = image;
  // Display the image
  imshow("Original Image",imgROI );
  moveWindow("Original Image" , 400 , 0);
  // Canny algorithm
  Mat contours;
  Canny(imgROI,contours,cannyLower,cannyHigher);
  Mat contoursInv;
  threshold(contours,contoursInv,128,255,THRESH_BINARY_INV);
  imshow("Canny",contoursInv);
      
  std::vector<Vec2f> lines;
  HoughLines(contours,lines,1,PI/180, houghVote);
  Mat result(imgROI.size(),CV_8U,Scalar(255));
  imgROI.copyTo(result);
  // Draw the limes
  std::vector<Vec2f>::const_iterator it= lines.begin();
  Mat hough(imgROI.size(),CV_8U,Scalar(0));
  while (it!=lines.end()) {
          
    float rho= (*it)[0];   // first element is distance rho
    float theta= (*it)[1]; // second element is angle theta

    Point pt1(rho/cos(theta),0);        
    // point of intersection of the line with last row
    Point pt2((rho-result.rows*sin(theta))/cos(theta),result.rows);
    // draw a white line
    line( result, pt1, pt2, Scalar(255), 1); 
    line( hough, pt1, pt2, Scalar(255), 1);
    ++it;
  }
  // Display the detected line image
  imshow("Detected Lines with Hough",result);
  vector<Vec6f> segments   =getLineSegments( contoursInv, lines) ;
  drawLineSegments(contoursInv ,segments, Scalar(0));
  detectTunnel(segments);
  
  int seg1, seg2;
  bool foundLane  = false;
  for ( int i =0 ; i< segments.size(); i++)
    for( int j = i+1 ; j <segments.size(); j++){
      cout<< segments[i][5] << " " << segments[j][5] << " sum is " << (segments[i][5] + segments[j][5]) * 180 /PI<<endl;
      if( abs ((segments[i][5] + segments[j][5])* 180 /PI - 180 ) < 10)
        {
          seg1 = i;
          seg2 = j;
          foundLane = true;
        }
    }

  if(foundLane){
    if( segments[seg1][5] > segments[seg2][5])
      {
        int temp;
        temp = seg1;
        seg1 = seg2;
        seg2 = temp;
      }

    if( (180- segments[seg1][5] * 180/PI) < segments[seg2][5] * 180/PI)
      cout<<"LEFT"<<endl;
    else
      cout<< "RIGHT"<<endl;

  }
  else{
    cout<<"Cant decide"<<endl;
  }
  
  //find seg
  imshow("Personal algo", contoursInv);
  lines.clear();
  waitKey(1000);
}

void on_trackbar( int, void* )
{
  LaneDetect();
}
int main(int argc, char* argv[])
{

  argp = argv[1];
  capture = VideoCapture(argp);

  if (!capture.isOpened()) //if this fails, try to open as a video camera, through the use of an integer param
    {capture.open(atoi(argp.c_str()));}
  double dWidth = capture.get(CV_CAP_PROP_FRAME_WIDTH); //get the width of frames of the video
  double dHeight = capture.get(CV_CAP_PROP_FRAME_HEIGHT); //get the height of frames of the video
  Size frameSize(static_cast<int>(dWidth), static_cast<int>(dHeight));
  image = imread(argv[1]);	
  namedWindow("Trackbars" , 1);
  createTrackbar( "Hough Vote" , "Trackbars", &houghVote, 400, on_trackbar );
  createTrackbar( "cannyLower" , "Trackbars", &cannyLower, 400, on_trackbar );
  createTrackbar( "cannyHigher" , "Trackbars", &cannyHigher, 400, on_trackbar );


  while (1)
    {
      capture >> image;
      LaneDetect();
      char key = (char) waitKey(10000);
    }
}




