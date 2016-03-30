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
#include <fstream>

using namespace std;
using namespace urbi;

class UFiles : public UObject {
public:
  UFiles(const string& str);
  void init();

  UVar intVar;
  UVar listVar;

  double add(double rhs) const;

  vector<string> ReadFile(string);
  void WriteFile(string, vector<string>);

private:

};


UFiles::UFiles(const string& s) : urbi::UObject(s) {
  UBindFunction(UFiles, init);
};

void UFiles::init() {
  UBindVar(UFiles, intVar);
  UBindVar(UFiles, listVar);
  UBindFunction(UFiles, add);
  UBindFunction(UFiles, ReadFile);
  UBindFunction(UFiles, WriteFile);

  intVar = 666;
  static const int arr[] = { 16,2,77,29 };
  listVar = vector<int>(arr, arr + sizeof(arr) / sizeof(arr[0]));





}

double UFiles::add(double rhs) const {
  int a = 10;

  return ((double)intVar) + rhs;
}

vector<string> UFiles::ReadFile(string file) {
  vector<string> res;
  ifstream infile(file.c_str());

  string line;
  while (std::getline(infile, line)) {
    res.push_back(line);
  }
  return res;
}

void UFiles::WriteFile(string file, vector<string> lines) {
  ofstream outfile(file.c_str());
  for (int i = 0; i < lines.size(); i++) {
    outfile << lines[i] << endl;
  }
}


UStart(UFiles);

