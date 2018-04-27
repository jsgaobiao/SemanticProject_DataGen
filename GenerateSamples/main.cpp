#include <iostream>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include <QString>
#include <QStringList>
#include "dsvlprocessor.h"
#include "seglogloader.h"

using namespace std;

int main()
{
    std::string dsvlfilename = "/home/gaobiao/Documents/2-1/2-2-bg.dsvl";
    std::string seglogfilename = "/home/gaobiao/Documents/2-1/2-2-bgalign.log";
    std::string colortablefilename = "/home/gaobiao/Documents/2-1/colortable.txt";

    SegLogLoader segloader;
    segloader.loadSegLog(const_cast<char*>(seglogfilename.c_str()));
    segloader.loadColorTabel(const_cast<char*>(colortablefilename.c_str()));

    DsvlProcessor dsvl(dsvlfilename);
    dsvl.setSeglog(&segloader);
    dsvl.Processing();

    cout << "Hello World!" << endl;
    return 0;
}
