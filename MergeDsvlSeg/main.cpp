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

void DoProcessing(DsvlProcessor &dsvl_fg, DsvlProcessor &dsvl_bg)
{
    dsvl_fg.dfp.seekg(0, std::ios_base::end);
    dsvl_fg.dFrmNum = dsvl_fg.dfp.tellg() / 180 / dsvl_fg.dsvlbytesiz;
    dsvl_fg.dfp.seekg(0, std::ios_base::beg);

    dsvl_bg.dfp.seekg(0, std::ios_base::end);
    dsvl_bg.dFrmNum = dsvl_bg.dfp.tellg() / 180 / dsvl_bg.dsvlbytesiz;
    dsvl_bg.dfp.seekg(0, std::ios_base::beg);

    dsvl_fg.InitRmap(&(dsvl_fg.rm));
    dsvl_bg.InitRmap(&(dsvl_bg.rm));
    dsvl_fg.onefrm= new ONEDSVFRAME[1];
    dsvl_bg.onefrm= new ONEDSVFRAME[1];

    IplImage * col = cvCreateImage (cvSize (dsvl_fg.rm.wid/2, dsvl_fg.rm.len*4.5),IPL_DEPTH_8U,3);

    int	num = 0;

    while (true)
    {
        int flag = 1;
        flag &= dsvl_fg.ReadOneDsvlFrame();
        flag &= dsvl_bg.ReadOneDsvlFrame();
        if (flag == 0) break;

        if (num%100==0)
            printf("%d (%d)\n", num, dsvl_fg.dFrmNum);
        num++;

        if (dsvl_fg.onefrm->dsv[0].millisec < 0) continue;
        std::string imgname = "/home/gaobiao/Documents/2-1/ladybug/";
        std::string tim = imgname + std::to_string(dsvl_fg.onefrm->dsv[0].millisec) + std::string("_seg.txt");
        FILE* fp = std::fopen(tim.c_str(), "w");

        point3fi *p;
        for (int i=0; i<BKNUM_PER_FRM; i++) {
            for (int j=0; j<LINES_PER_BLK; j++) {
                for (int k=0; k<PNTS_PER_LINE; k++) {
                    p = &dsvl_bg.onefrm->dsv[i].points[j*PNTS_PER_LINE+k];
                    if (!p->i) continue;
                    int x = i*LINES_PER_BLK+j;
                    int y = k;
                    int id = dsvl_bg.onefrm->dsv[i].lab[j*PNTS_PER_LINE+k];
                    if (id >= 0) {
                        dsvl_fg.onefrm->dsv[i].lab[j*PNTS_PER_LINE+k] = id;
                    }
                    std::fprintf(fp, "%d ", dsvl_fg.onefrm->dsv[i].lab[j*PNTS_PER_LINE+k]);
                }
            }
        }
        std::fclose(fp);
        dsvl_fg.ProcessOneFrame();

        cvResize(dsvl_fg.rm.lMap, col);
        //InteractiveSelect(col, &rm);
        cvShowImage("segmentation",col);
        cv::waitKey(1);

//        SampleGenerator sampler(&rm);
//        cv::setMouseCallback("segmentation", DsvlProcessor::MouseCallback, &sampler);
//        sampler.GenerateAllSamplesInRangeImage(&rm, seglog, vout);
//        std::cout<<"Generate Samples: "<<rm.millsec<<std::endl;

    }
    dsvl_fg.ReleaseRmap (&dsvl_fg.rm);
    dsvl_bg.ReleaseRmap (&dsvl_bg.rm);
    cvReleaseImage(&col);
    delete []dsvl_fg.onefrm;
    delete []dsvl_bg.onefrm;
}

int main()
{
    std::string dsvlfilename_fg = "/home/gaobiao/Documents/2-1/2-2-mv.dsvl";
    std::string dsvlfilename_bg = "/home/gaobiao/Documents/2-1/2-2-bg.dsvl";
    std::string colortablefilename = "/home/gaobiao/Documents/2-1/colortable.txt";
    //    std::string seglogfilename = "/home/gaobiao/Documents/2-1/2-2-bgalign.log";

    SegLogLoader segloader;
//    segloader.loadSegLog(const_cast<char*>(seglogfilename.c_str()));
    segloader.loadColorTabel(const_cast<char*>(colortablefilename.c_str()));

    DsvlProcessor dsvl_fg(dsvlfilename_fg);
    DsvlProcessor dsvl_bg(dsvlfilename_bg);
    dsvl_fg.setSeglog(&segloader);
    dsvl_bg.setSeglog(&segloader);
    DoProcessing(dsvl_fg, dsvl_bg);

    cout << "Hello World!" << endl;
    return 0;
}
