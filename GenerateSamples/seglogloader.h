#ifndef SEGLOGLOADER_H
#define SEGLOGLOADER_H
#include "types.h"

int seed_compare_fno(const void *t1, const void *t2);

class SegLogLoader
{
public:
    ~SegLogLoader();
    SegLogLoader();
    void loadSegLog(char *szFile);
    void loadColorTabel(char *filename);

    unsigned char colorTable[MAXLABNUM][3];

    std::map<int, int> prid_label_map;

    ONESEED		*seeds;
    int			seednum ;
    ONEPRID		*prids;
    int			pridnum;
    int			pridbufnum;
    int			seedidx;
};

#endif // SEGLOGLOADER_H
