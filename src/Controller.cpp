#include "Controller.h"

Controller::instanceFlag = false;

Controller* Controller::single = NULL;

Controller* Controller::getInstance(string path)
{
  if(!instanceFlag)
    {
      single = new Controller(path);
      cout<<"Created new controller instance"<<endl;
      instanceFlag = true;
    }
  return single;
}

Controller::Controller(string path):
  lastIndex(0), tunnelMode(false), orientation("?"), pathFound(false), m(path), mp(m), symbolDetector()
{
  m.displayMap();
  m.printLandmarks();
  locomotor = Locomotor::getInstance();
  cam = CamController::getInstance();
}

void Controller::start()
{
  mainLoop();
}

void Controller::processPassage(string passageDir)
{
  int flag;
  string orientation_next;
  std::vector<Landmark> newpath;
  
  // tunnel dilemma sends newpath consistent with lastIndex
  // check params (pass by ref tunnelExitDir)
  flag = mp.tunnelDilemma(passageDir,path,lastIndex,newpath,tunnelExitDir);
  
  if(flag == FOUND_TUNNEL_AND_TAKE && pathFound)
    {
      // traverse tunnel
      tunnelMode = true;
      // facePassage will align and turn to face the passage (and enter slightly so that lane detection can easily pick up ?)
      locomotor->facePassage(passageDir);
      path = newpath;
    }
  else if(flag == FOUND_TUNNEL_DONT_TAKE && pathFound)
    {

    }
  else if(flag == NOT_FOUND_TUNNEL)
    {
      // found a T-Junction
      if(pathFound == false)
	{
	  // try to select the path we are on and update orientation
	  for(int i=0;i<mp.paths.size();++i)
	    {
	      if(mp.paths[i].size() > 1 && mp.paths[i][1].shape == "TJ") // also check for the reqd direction here, update that
		{
		  path = mp.paths[i];
		  orientation = MapProcessor::getOrient(path[lastIndex],path[lastIndex+1]);
		  break;
		}
	    }
	  pathFound = true;
	}
	      
      orientation_next = MapProcessor::getOrient(path[lastIndex+1],path[lastIndex+2]);
      
      // if orientations match, take left at T-Junction
      if(orientation_next == passageDir)
	locomotor->facePassage(passageDir);
      ++lastIndex;
    }
}
  
void Controller::mainLoop()
{
  int flag;
  bool tunnelMode = false;
  string tunnelExitDir;
  string passageDir;
  string orientation_next, direction;
  string shape, color;
  bool pLeft = false;
  bool pRight = false;
  int distance_front; //cm
  
  while(true)
    {
      distance_front = locomotor->getDistanceFront();
      
      if(distance_front > LANE_FOLLOW_MIN)
	followLane();
      else
	move("STRAIGHT",AMT_LANE);
      
      CamController::isPassage(pLeft,pRight);

      // may cause infinite loop?
      if(pLeft)
	processPassage("LEFT");
      if(pRight)
	processPassage("RIGHT");

      if(distance_front < LANE_FOLLOW_MIN)
	{
	  // reached end of corridor
	  // Cases :
	  // 1. end of tunnel
	  // 2. corner
	  // in case of a T-Junction we would have already turned to face it above
	  
	  if(tunnelMode)
	    {
	      // exiting the tunnel
	      move(tunnelExitDir,AMT_TURN);
	      tunnelExitDir = "?";
	      tunnelMode = false;
	    }
	  else
	    {
	      // reached a corner
	      
	      // if the next symbol is end
	      if(pathFound && lastIndex == path.size() - 2)
		{
		  cout << "DONE!" << endl;
		  exit(0);
		}

	      flag = detectSymbol(shape, color);
	      if(flag == FOUND_SYMBOL)
		{
		  if(!pathFound)
		    {
		      // try to select the path we are on and update orientation
		      for(int i=0;i<mp.paths.size();++i)
			{
			  if(mp.paths[i].size() > 1 && mp.paths[i][1].shape == shape && mp.paths[i][1].color == color)
			    {
			      path = mp.paths[i];
			      orientation = MapProcessor::getOrient(path[lastIndex],path[lastIndex+1]);
			      break;
			    }
			}
		      pathFound = true;
		      orientation = MapProcessor::getOrient(path[lastIndex],path[lastIndex+1]);			      
		    }
		  else
		    {
		      if(shape == path[lastIndex+1].shape && color == path[lastIndex+1].color)
			{
			  // turn in reqd dir and upd orientation
			  orientation_next = MapProcessor::getOrient(path[lastIndex+1],path[lastIndex+2]);
			  direction = MapProcessor::getDir(orientation,orientation_next);
			  move(direction,AMT_TURN);
			  orientation = orientation_next;
			  lastIndex++;
			}
		      else
			{
			  // mario panic :D
			}
		    }

		}
	      else
		{
		  // reached end of corridor, but didn't detect any symbol!
		}
	    }
	}
    }
}

void Controller::move(string dir, int amt)
{
  if(dir == "RIGHT")
    locomotor->goRight(amt);
  else if(dir == "LEFT")
    locomotor->goLeft(amt);
  else if(dir == "FORWARD")
    locomotor->goForward(amt);
  else if(dir == "BACKWARD")
    locomotor->goBackward(amt);
  else if(dir == "UTURN")
    locomotor->goUTurn(amt);

  if(amt == AMT_TURN)
    sleep(2);
}

int Controller::detectSymbol(string& shape, string& color)
{
  cv::Mat frame;
  VideoCapture cap(0);
  int i = 0;
  string prevShape,prevColor;
  string currShape,currColor;

  // if 2 successive frames detect same symbol, return it
  cap >> frame;
  symbolDetector.getSymbol(frame,prevShape,prevColor);
  while(i<MAX_ATTEMPTS - 1)
    {
      cap >> frame;
      symbolDetector.getSymbol(frame,currShape,currColor);
      if(currShape == prevShape && currColor == prevColor)
	{
	  shape = currShape;
	  color = currColor;
	  return FOUND_SYMBOL;
	}
      prevShape = currShape;
      prevColor = currColor;
      ++i;
      cv::waitKey(33);
    }
  return NOT_FOUND_SYMBOL;
}


void Controller::followLane()
{ 
  std::vector<cv::Mat> frames(MAX_ATTEMPTS);
  string dir;
  VideoCapture cap(0);
  int i = 0;
  while(i<MAX_ATTEMPTS)
   {
      cap >> frames[i++];
      cv::waitKey(33);
    }
  
  dir = CamController::laneFollowDir(frames);
  
  move(dir, AMT_LANE);
}



int main(int argc, char *argv[])
{
  if(argc!=2)
    {
      cout << "Usage : ./GeoAware <path/to/map>" << endl;
      exit(1);
    }
  
  Controller *controller = Controller::getInstance(argv[1]);
  controller->start();
  return 0;
}
