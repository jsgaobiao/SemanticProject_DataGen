#include "samplegenerator.h"
#include "seglogloader.h"
#include <QDir>
#include <algorithm>
#include <unordered_set>
SampleGenerator::SampleGenerator(RMAP *prm_)
{
    prm = prm_;
    dataimg = cv::Mat::zeros(prm->len * 4.5, prm->wid / 2, CV_8UC3);
}

void SampleGenerator::ExtractPointsByCenter(std::vector<LABELLEDPONTS> &points, double center_x, double center_y, double center_z)
{
    //
    cv::Point3f minBoxPt = cv::Point3f(center_x - 2.5, center_y - 2.5, center_z - 1.2);
    cv::Point3f maxBoxPt = cv::Point3f(center_x + 2.5, center_y + 2.5, center_z + 1.2);

    points.clear();
    LABELLEDPONTS onelabelpt;

    for (int y = 0; y < prm->len; y++) {
        for (int x = 0; x < prm->wid; x++) {
            if (!prm->pts[y*prm->wid + x].i)
                continue;

            int px = x / 2;
            int py = y * 4.5;

            onelabelpt.img_x = px;
            onelabelpt.img_y = py;
            onelabelpt.intensity = prm->pts[y*prm->wid + x].i;
            onelabelpt.loc = cv::Point3f(prm->pts[y*prm->wid + x].x, prm->pts[y*prm->wid + x].y, prm->pts[y*prm->wid + x].z);
            if (IsInBbox(onelabelpt, minBoxPt, maxBoxPt)) {
                points.push_back(onelabelpt);
            }
        }
    }
}

void SampleGenerator::ExtractSampleByCenter(cv::Point2i centerInImage, cv::Mat &outsample)
{
    //find the point in local coordinate
    double x,y,z;
    x = prm->pts[centerInImage.y*prm->wid+centerInImage.x].x;
    y = prm->pts[centerInImage.y*prm->wid+centerInImage.x].y;
    z = prm->pts[centerInImage.y*prm->wid+centerInImage.x].z;

    std::vector<LABELLEDPONTS> labelpoints;
    ExtractPointsByCenter(labelpoints, x, y, z);

    int width = 256;
    int height = 256;

    cv::Mat range_mat = cv::Mat::zeros(height, width, CV_8UC1);
    cv::Mat height_mat = cv::Mat::zeros(height, width, CV_8UC1);
    cv::Mat intensity_mat = cv::Mat::zeros(height, width, CV_8UC1);

    int px = centerInImage.x / 2;
    int py = centerInImage.y * 4.5;
    cv::Point left_top = cv::Point(px - width / 2, py - height / 2);
    cv::Point right_bottom = cv::Point(px + width / 2, py + height / 2);

    //
    std::vector<double> range_feat;
    double min_r = 999999;
    double max_r = -min_r;
    double min_z = min_r;
    double max_z = max_r;
    int count = 0;
    for (int i = 0; i < labelpoints.size(); i++) {
        if (labelpoints[i].img_x <= left_top.x || labelpoints[i].img_x >= right_bottom.x
            || labelpoints[i].img_y <= left_top.y || labelpoints[i].img_y >= right_bottom.y) {
            continue;
        }
        count++;
        double onerange = std::sqrt(sqr(labelpoints[i].loc.x) + sqr(labelpoints[i].loc.y) + sqr(labelpoints[i].loc.z));
        range_feat.push_back(onerange);
        min_r = std::min<double>(onerange, min_r);
        max_r = std::max<double>(onerange, max_r);
        min_z = std::min<double>(labelpoints[i].loc.z, min_z);
        max_z = std::max<double>(labelpoints[i].loc.z, max_z);
    }

    if (count > 0) {
        count = 0;
        for (int i = 0; i < labelpoints.size(); i++) {
            if (labelpoints[i].img_x <= left_top.x || labelpoints[i].img_x >= right_bottom.x
                || labelpoints[i].img_y <= left_top.y || labelpoints[i].img_y >= right_bottom.y) {
                continue;
            }
            int ix = labelpoints[i].img_x - px + width / 2;
            int iy = labelpoints[i].img_y - py + height / 2;

            range_mat.at<uchar>(iy, ix) = (range_feat[count] - min_r) / (max_r - min_r) * 255; //����
            int height = (labelpoints[i].loc.z + 3.0) * 60;
            //height_mat.at<uchar>(iy, ix) = (labelpoints[i].loc.z - min_z) / (max_z - min_z) * 255; //�߶�
            height_mat.at<uchar>(iy, ix) = std::min<int>(height, 255); //�߶�
            intensity_mat.at<uchar>(iy, ix) = labelpoints[i].intensity; //intensity

            count++;
        }
        cv::Mat container[3] = { range_mat, height_mat, intensity_mat };

        cv::merge(container, 3, outsample);

        cv::imshow("ColorSample", outsample);
        cv::imshow("range_mat", range_mat);
        cv::imshow("height_mat", height_mat);
        cv::imshow("intensity_mat", intensity_mat);
    }
}

void SampleGenerator::GenerateAllSamplesInRangeImage(RMAP *prm_, SegLogLoader *seglog)
{
    prm = prm_;
    int count = 0;

    //determine all region id, prm->regnum is not enough
    std::unordered_set<int> idx_set;
    for (int y = 0; y < prm->len; y++) {
        for (int x = 0; x < prm->wid; x++) {
//            if (!prm->pts[y*prm->wid + x].i)
//                continue;
            if (prm->regionID[y*prm->wid+x] > 0){
                idx_set.insert(prm->regionID[y*prm->wid+x]);
            }
        }
    }

    std::unordered_set<int>::iterator iter;
    for (iter=idx_set.begin(); iter!=idx_set.end(); iter++){
        int regionid = *iter;
        int count = 0;
        int max_sample_num = 0;

        for (int i=0; i<seglog->seednum; i++){
            int label = seglog->seeds[i].lab;
            if ((seglog->seeds[i].prid==regionid)){
                cv::Mat outsample;

                cv::Point2i center_in_image;
                center_in_image.x = seglog->seeds[i].ip.x;
                center_in_image.y = seglog->seeds[i].ip.y;
                ExtractSampleByCenter(center_in_image, outsample);

                if (outsample.data){
                    std::string imgname = "/home/pku-m/SemanticMap/campus_test12/";
                    QDir dir(QString(imgname.c_str()));
                    if (!dir.exists()){
                        if (!dir.mkpath(dir.absolutePath()))
                            return ;
                    }

                    //name folder by region ID
                    int seedid = seglog->seeds[i].rno;

                    QString foldername =QString("%1%2_%3").arg(imgname.c_str()).arg(regionid).arg(label);
                    dir.mkdir(foldername);

                    imgname = foldername.toStdString() + "/" + std::to_string(seglog->seeds[i].milli) + "_" + std::to_string(seedid);
                    imgname += ".png";
                    cv::imwrite(imgname, outsample);
                }

                count++;
//                if (count >= max_sample_num)
//                    break;
            }
        }
    }

    return ;
}

bool SampleGenerator::IsInBbox(LABELLEDPONTS pt, cv::Point3f minBoxPt, cv::Point3f maxBoxPt)
{
    if (pt.loc.x>minBoxPt.x && pt.loc.x<maxBoxPt.x)
        if (pt.loc.y>minBoxPt.y && pt.loc.y<maxBoxPt.y)
            if (pt.loc.z > minBoxPt.z && pt.loc.z < maxBoxPt.z) {
                return 1;
            }
    return 0;
}

void SampleGenerator::setRangeMapPointer(RMAP *value)
{
    prm = value;
}

void SampleGenerator::OnMouse(int event, int x, int y)
{
    if (event == CV_EVENT_LBUTTONDOWN)
    {
        cv::Point p = cv::Point(x,y);
        p.x = x * 2;
        p.y = y / 4.5;

        if (!prm)
            return;

        int regionid = prm->regionID[p.y*prm->wid + p.x];
        if (regionid == GROUND)
            return;

        IDTYPE rid(regionid, 0);

//        cv::Mat sample_mat;
//        ExtractSampleById(rid, sample_mat);

        printf("time: %d ID: %d point_num: %d\n", prm->millsec, regionid, rid.point_num);
       // cv::imshow("sample", sample_mat);
    }
}
