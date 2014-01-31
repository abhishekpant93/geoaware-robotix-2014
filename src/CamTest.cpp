#include "CamController.h"

using namespace cv;
int main(int argc , char **argv){

  CamController cp;

  char * argp = argv[1];
  VideoCapture capture = VideoCapture(argp);

  Mat frame;
  // if 2 successive frames detect same symbol, return it

  while(1){
    bool left ,right;
    capture >> frame;
    cp.processVideo( frame , "TUNNEL" , left ,right);
    cp.processVideo(frame , "LANE" , left , right);
    cv::waitKey(1000);

  }
}
