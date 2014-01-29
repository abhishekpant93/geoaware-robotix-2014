#ifndef LOCOMOTOR_H
#define LOCOMOTOR_H

#include <fstream>
#include<iostream>
#include<curses.h>
using namespace std;

class Locomotor
{  
 public:
  static std::ofstream loco_arduino;
  static std::ofstream dist_arduino;
  void goLeft(int amount = 1);
  void goRight(int amount =1 );
  void goForward(int amount = 1);
  void goBackward(int amount = 1);
  void switchToKeyboard();
  int getDistanceFront();
  static Locomotor* getInstance();
  Locomotor();
  Locomotor(const Locomotor&);
  ~Locomotor();
  void writeToDevice(char c);
 private:
  static bool instanceFlag;
  static Locomotor *single;
  static bool streamFlag;
};

#endif 
