#ifndef LOCOMOTOR_H
#define LOCOMOTOR_H

#include <fstream>
#include<iostream>
#include<curses.h>
#include <deque>
#include<vector>
//#define LOCO_ARDUINO "/dev/ttyUSB0"
//#define DIST_ARDUINO "/dev/ttyACM0"

using namespace std;

class Locomotor
{  
 public:
  static std::ofstream loco_arduino_out;
  static std::ofstream dist_arduino_out;
  static std::ifstream dist_arduino_in;
  deque<string> path_history;
  void goLeft(int amount = 1);
  void goRight(int amount =1 );
  void goForward(int amount = 1);
  void goBackward(int amount = 1);
  void goUTurn( int amount=1);
  void gradualLeft(int amount = 1);
  void gradualRight(int amount = 1);
  void addToPathHistory (string dir);
  void switchToKeyboard();
  int getDistanceFront();
  int getDistanceLeft();
  int getDistanceRight();
  string currentPos;
  //void facePassage(string passageDir);
  //static Locomotor* getInstance();
  static Locomotor *getInstance(string ACM = "0" , string USB ="0");
  Locomotor(string ACM , string USB);
  Locomotor(const Locomotor&);
  ~Locomotor();
  vector<string> windBack(int amt);

  
 private:
  void writeToLoco(char c);
  void writeToDist( char c);
  int getDistance();
  void servoLeft();
  void servoRight();
  void servoFront();
  static bool instanceFlag;
  static Locomotor *single;
  static bool streamFlag;
};

#endif 
