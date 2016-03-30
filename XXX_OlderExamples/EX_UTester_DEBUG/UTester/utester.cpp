/**********************************************************
 **********************************************************
 **
 **				..:: UTester ::..
 **
 **		Very simple module for communication with
 **		evaluation board with MC9S12A64.
 **
 **    ver.: 1.0
 **    date: 22.06.2012
 **  author: Jan Kedzierski, Adam Oleksy, Wroclaw University of Technology
 *********************************************************
 ********************************************************/

#include <urbi/uobject.hh>
#include <boost/unordered_map.hpp>
#include <sstream>
#include <cstdlib>

using namespace std;
using namespace urbi;

class UTester : public UObject {
public:
  UTester(const string& str);
  void init();

  UVar intVar;
  UVar listVar;

  double add(double rhs) const;



private:

};


UTester::UTester(const string& s) : urbi::UObject(s) {
  UBindFunction(UTester, init);
};

void UTester::init() {
  UBindVar(UTester, intVar);
  UBindVar(UTester, listVar);
  UBindFunction(UTester, add);

  intVar = 666;
  static const int arr[] = { 16,2,77,29 };
  listVar = vector<int>(arr, arr + sizeof(arr) / sizeof(arr[0]));



  

}

double UTester::add(double rhs) const {
  int a = 10;

  return ((double)intVar) + rhs;
}

UStart(UTester);
