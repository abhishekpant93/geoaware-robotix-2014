
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>

#define EPSILON 10
#define PRIMARY_THRESHOLD 69
#define BLACK_THRESHOLD 65
#define WHITE_THRESHOLD 175
#define MAXVAL 255
#define THRESH 100

using namespace cv;
using namespace std;

static double angle(cv::Point pt1, cv::Point pt2, cv::Point pt0)
{
  double dx1 = pt1.x - pt0.x;
  double dy1 = pt1.y - pt0.y;
  double dx2 = pt2.x - pt0.x;
  double dy2 = pt2.y - pt0.y;
  return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

string vtxToShape(int vtc)
{
  if (vtc >= 8)
    return "CIRCLE";
  if(vtc == 3)
    return "TRIANGLE";
  if(vtc == 4)
    return "SQUARE";
  if(vtc == 5)
    return "PENTAGON";
  if(vtc == 6)
    return "HEXAGON";
  return "None";
}

bool isPrimaryColor(int blue, int green, int red)
{
  return ((blue-green>PRIMARY_THRESHOLD && blue-red>PRIMARY_THRESHOLD) || (green-blue>PRIMARY_THRESHOLD && green-red>PRIMARY_THRESHOLD) || (red-blue>PRIMARY_THRESHOLD && red-green>PRIMARY_THRESHOLD));
}

string getColor(int blue, int green, int red)
{
  string color;
  if(isPrimaryColor(blue,green,red))
    {
      if(blue>green && blue>red)
	color = "BLUE";
      else if(green>blue && green>red)
	color = "GREEN";
      else
	color = "RED";
    }
  else
    {
      if(red>200 && green>200)
	color = "YELLOW";
      else
	color = "BROWN";
    }
  return color;
}

void removeDuplicateContours(std::vector<std::vector<cv::Point> >& contours_dupl, std::vector<std::vector<cv::Point> >& contours)
{
  int i,j;
  int X,Y,x,y;
  cv::Rect R,r;
  for(i=0;i<contours_dupl.size();++i)
    {
      R = cv::boundingRect(contours_dupl[i]);
      X = R.x;
      Y = R.y;
      for(j=0;j<contours.size();++j)
	{
	  r = cv::boundingRect(contours[j]);
	  x = r.x;
	  y = r.y;
	  if(abs(X-x)<EPSILON && abs(Y-y)<EPSILON)
	    break;
	}
      if(j==contours.size())
	  contours.push_back(contours_dupl[i]);
    }
}

void cleanEdges(std::vector<cv::Point>& approx, std::vector<cv::Point>& actual)
{
  // Number of vertices of polygonal curve (small edges yet to be removed)
  int vtc = approx.size();
  
  if(vtc>=3 && vtc<=6)
    {
      int j = 0;
      int s = approx.size();
      while(j<s)
	{
	  Point2f a(approx[j].x,approx[j].y);
	  Point2f b(approx[(j+1) % s].x,approx[(j+1) % s].y);
	  double res = cv::norm(a-b);
	  if(res>=EPSILON) //keep only large edges
	    actual.push_back(approx[j]);
	  ++j;
	}
    }
  else
    actual = approx;
}

string getColorHSV( int h , int s , int v){

  string color = "default";
  if( s > 100 && v >50){
    if( h < 20 || h > 340){
      color = "RED";
    }
    else if( h > 160 && h < 260){
      color = "BLUE";
    }
    else if ( h>90 && h < 140){
      color = "GREEN";
    }
  }
  else if ( s<  50 && v > 50){
    color = "WHITE";
  }
  else if ( v < 50){
    color = "BLACK";
  }
  cout << "h = " << h << ", s = " << s << ", v = " << v << endl;
  return color;   

}



void addShape(cv::Mat src_thresh, std::vector<cv::Point2f>& centers, std::vector<float>& radii, std::vector<double>& areas, std::vector<string>& shapes, std::vector<string>& colors, std::vector<cv::Point>& actual)
{
  cv::Point2f center;
  float radius;
  double area;
  
  cv::minEnclosingCircle( (Mat)actual, center, radius );
  area = cv::contourArea(actual);

  Mat hsv;
  cvtColor(src_thresh ,hsv ,CV_BGR2HSV);
  
  int blue = 0, red = 0, green = 0, brown = 0, yellow = 0;
  string color;
  for(int i=center.x-5; i<center.x+5;++i)
    {
      for(int j =center.y-5; j < center.y+5; ++j)
	{

	  Vec3b intensity = hsv.at<Vec3b>(j,i);
	  int hue = (int)intensity.val[0];
	  int sat = (int)intensity.val[1];
	  int val = (int)intensity.val[2];
	  color = getColorHSV(hue *2 , val , sat);

	    // Vec3b intensity = src_thresh.at<Vec3b>((int)center.y,(int)center.x);
	    // int b = (int)intensity.val[0];
	    // int g = (int)intensity.val[1];
	    // int r = (int)intensity.val[2];
	    // color = getColor(b,g,r);
	    if(color == "BLUE")
	      ++blue;
	    if(color == "RED")
	      ++red;
	    if(color == "GREEN")
	      ++green;
	    if(color == "BROWN")
	      ++brown;
	    if(color == "YELLOW")
	      ++yellow;
	}
    }

  if(blue > 50)
    color = "BLUE";
  else if(green > 50)
    color = "GREEN";
  else if(red> 50)
    color = "RED";
  else if(brown > 50)
    color = "BROWN";
  else if(yellow > 50)
    color = "YELLOW";
  else
    color = "NONE";

  
  centers.push_back(center);
  radii.push_back(radius);
  areas.push_back(area);
  shapes.push_back(vtxToShape(actual.size()));
  colors.push_back(color);
}
void detectSymbol(cv::Mat src, cv::Mat src_thresh, cv::Mat edges)
{
  std::vector<std::vector<cv::Point> > contours_dupl;
  std::vector<std::vector<cv::Point> > contours;
  cv::findContours(edges.clone(), contours_dupl, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
  removeDuplicateContours(contours_dupl,contours);
  
  std::vector<cv::Point> approx;
  std::vector<cv::Point> actual;
  
  int vtc;
  cv::Scalar clr;
  int i,j;
  
  // used to store all detected shapes in 'edges'
  std::vector<cv::Point2f> centers;
  std::vector<float> radii;
  std::vector<double> areas;
  std::vector<string> shapes;
  std::vector<string> colors;
  
  // cout << "number of contours : " << contours.size() << endl;
  // for(i=0;i<contours.size();++i)
  //   {
  //     cout << contours[i] << endl;
  //     for(j=0;j<contours[i].size();++j)
  // 	{
  // 	  clr = CV_RGB(rand()%255,rand()%255,rand()%255);
  // 	  cv::circle(src,contours[i][j] , 5, clr, 2, 8, 0 );	
  // 	}
  //   }

  for(i=0;i<contours.size();++i)
    {
      actual.clear();
      cv::approxPolyDP(cv::Mat(contours[i]), approx, cv::arcLength(cv::Mat(contours[i]), true)*0.02, true);
      cleanEdges(approx,actual);
      vtc = actual.size();
      //cout << "approx : " << approx.size() << ", actual : " << actual.size() << endl;
      
      for(j=0;j<actual.size();++j)
	{
	  clr = CV_RGB(rand()%255,rand()%255,rand()%255);
	  cv::circle(src,actual[j] , 5, clr, 2, 8, 0 );	
	}

      if (std::fabs(cv::contourArea(contours[i])) < 100 || !cv::isContourConvex(approx) || vtc<3)
      	continue;
      
      if(vtc==3)
	addShape(src_thresh, centers, radii, areas, shapes, colors, actual);
      else if(vtc>=4 && vtc<=6)
	{
	  std::vector<double> cos;
	  for (int j = 2; j < vtc+1; j++)
	    cos.push_back(angle(approx[j%vtc], approx[j-2], approx[j-1]));

	  // Sort ascending the corner degree values
	  std::sort(cos.begin(), cos.end());

	  // Get the lowest and the highest degree
	  double mincos = cos.front();
	  double maxcos = cos.back();
	  //cout << "mincos : " << mincos << ", maxcos : " << maxcos << endl;
	  // Use the degrees obtained above and the number of vertices
	  // to determine the shape of the contour
	  if (vtc == 4 && mincos >= -0.1 && maxcos <= 0.3)
	    addShape(src_thresh, centers, radii, areas, shapes, colors, actual);
	  else if (vtc == 6 && mincos >= -0.65 && maxcos <= -0.45)
	    addShape(src_thresh, centers, radii, areas, shapes, colors, actual);
	}
      else if(vtc>=8)
	{
	  // Detect circles ( area ~= pi*r^2 )
	  double area = cv::contourArea(contours[i]);
	  cv::Rect r = cv::boundingRect(contours[i]);
	  int radius = r.width / 2;

	  if (std::abs(1 - ((double)r.width / r.height)) <= 0.2 && std::abs(1 - (area / (CV_PI * std::pow(radius, 2)))) <= 0.2)
	    addShape(src_thresh, centers, radii, areas, shapes, colors, actual);
	}
    }
  
  if(centers.size() == 0)
    {
      cout << "Could not detect symbol!" << endl;
      return;
    }
  
  // get index of largest area shape
  int s = 0;
  for(i = 0;i < areas.size(); ++i)
    {
      if(areas[i] > areas[s])
	s = i;
      cout << "added : center = " << centers[i] <<", radius = " << radii[i] << ", area = " << areas[i] << ", shape = " << shapes[i] << endl;
    }
  
  // print details of biggest shape
  clr = CV_RGB(rand()%255,rand()%255,rand()%255);
  cv::circle(src, centers[s], radii[s], clr, 2, 8, 0 );
  cout << "detected : shape = " << shapes[s] << ", color = " << colors[s] << endl;
  cout << "-------------------" << endl;
}

// custom threshold function for pre-processing
void customThreshold(cv::Mat& src_color, cv::Mat& dst, int thresh, int maxval)
{  
  dst = src_color.clone();
  for(int i=0;i<src_color.rows;++i)
    {
      for(int j=0;j<src_color.cols;++j)
	{
	  Vec3b intensity = src_color.at<Vec3b>(i,j);
	  int blue = (int)intensity.val[0];
	  int green = (int)intensity.val[1];
	  int red = (int)intensity.val[2];

	  if((blue>=WHITE_THRESHOLD && green>=WHITE_THRESHOLD && red>=WHITE_THRESHOLD) || (blue<=BLACK_THRESHOLD && green<=BLACK_THRESHOLD && red<=BLACK_THRESHOLD)) //white or black
	    {
	      dst.at<Vec3b>(i,j)[0] = 0;
	      dst.at<Vec3b>(i, j)[1] = 0;
	      dst.at<Vec3b>(i, j)[2] = 0;
	    }
	  else
	    {
	      dst.at<Vec3b>(i, j)[0] = blue;
	      dst.at<Vec3b>(i, j)[1] = green;
	      dst.at<Vec3b>(i, j)[2] = red;
	    }
	}
    }
}

int main()
{
  srand(time(NULL));
  
  VideoCapture cap(0); // open the default camera
  
  // if(!cap.isOpened()) // check if we succeeded
  //   {
  //     cout << "No video capture device detected!" << endl;
  //     return -1;
  //   }

  cv::Mat src, src_gray, src_thresh;
  cv::Mat edges_normal, edges_blur;
  cv::Mat eroded, dilated, dilated_blur;
  cv::Mat kernel = Mat::ones(Size(7, 7), CV_8U);

  while(true)
    {
      cap >> src;
      
      //src = cv::imread("../assets/samples/symbols/shape_3.jpg");
      Rect roi(0,src.rows/6,src.cols-1,src.rows - src.rows/6);// set the ROI for the image
      src = src(roi); 
      cv::cvtColor( src, src_gray, COLOR_RGB2GRAY );
      //cv::blur( src_gray, src_gray_smooth, Size( 5, 5 ), Point(-1,-1) );

      customThreshold(src, src_thresh, THRESH, MAXVAL);
      //cv::threshold(src_gray, src_binary, THRESH, MAXVAL, THRESH_BINARY);
      
      eroded = src_thresh.clone();
      cv::erode(src_thresh,eroded,kernel);
      dilated = eroded.clone();
      cv::dilate(eroded,dilated,kernel);
      
      //cv::medianBlur(dilated, dilated_blur, 7);
      //cv::Canny(dilated_blur, edges_blur, 50, 200, 3);
      cv::Canny(dilated, edges_normal, 50, 200, 3 );

      //detectSymbol(src, src_thresh, edges_blur);
      detectSymbol(src, src_thresh, edges_normal);
      
      cv::imshow("src",src);
      cv::imshow("thresh", src_thresh);
      cv::imshow("eroded",eroded);
      cv::imshow("dilated",dilated);
      cv::imshow("edges_normal",edges_normal);
      if(cv::waitKey(33) == 'q')
	break;
    }
  return 0;
}
