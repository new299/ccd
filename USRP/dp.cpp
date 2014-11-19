#include <iostream>
#include <fstream>

using namespace std;

int main() {

  ifstream f("d");

  for(;!f.eof();) {
    if(!f.eof()) {
      double d;
      f >> d;
      d = d -700;
      if(d > 1000) d = 1000;
      cout << d << " " << endl;
    } else break;
  }

}
