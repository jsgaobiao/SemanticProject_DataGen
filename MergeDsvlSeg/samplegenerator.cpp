#include "samplegenerator.h"
#include "seglogloader.h"
#include <QDir>
#include <algorithm>
#include <map>
#include <unordered_set>
SampleGenerator::SampleGenerator(RMAP *prm_)
{
    prm = prm_;
    dataimg = cv::Mat::zeros(prm->len * 4.5, prm->wid / 2, CV_8UC3);
}

void SampleGenerator::ExtractSampleById(IDTYPE &rid, cv::Mat &outsample)
{
    LABELLEDPONTS onelabelpt;
    std::vector<LABELLEDPONTS> labelpoints;
    cv::Mat mask_mat = cv::Mat::zeros(prm->len, prm->wid, CV_8UC1);

    int count = 0;
    labelpoints.clear();

    int regionid = rid.id;

    for (int y = 0; y < prm->len; y++) {
        for (int x = 0; x < prm->wid; x++) {
            if (!prm->pts[y*prm->wid + x].i)
                continue;

            if (regionid != prm->regionID[y*prm->wid + x])
                continue;

            int px = x / 2;
            int py = y * 4.5;

            if (px < 0 || px >= dataimg.cols || py < 0 || py >= dataimg.rows)
                continue;

            count++;

            //����ԭʼ��Ϣ��������������
            onelabelpt.img_x = px;
            onelabelpt.img_y = py;
            onelabelpt.intensity = prm->pts[y*prm->wid + x].i;
            onelabelpt.loc = cv::Point3f(prm->pts[y*prm->wid + x].x, prm->pts[y*prm->wid + x].y, prm->pts[y*prm->wid + x].z);
            labelpoints.push_back(onelabelpt);

            mask_mat.at<uchar>(y, x) = 255;

            //���ӻ�
            int val = prm->regionID[y*prm->wid + x] % COLORNUM;
            dataimg.at<cv::Vec3b>(py, px) = cv::Vec3b(LEGENDCOLORS[val]);
        }
    }

    rid.point_num = count;

    if (count == 0)
        return ;

    double mean_x, mean_y, mean_z;
    mean_x = mean_y = mean_z = 0.0;

    int mean_ix, mean_iy;
    mean_ix = mean_iy = 0;

    if (labelpoints.size()) {

        cv::Rect bbox;
        bool ret = FindMaxContourCenter(mask_mat, mean_ix, mean_iy, bbox);

        if (!ret) {
            return;
        }

        ExtractPointsInBbox(bbox, labelpoints, mean_x, mean_y, mean_z);
    }

    if (mean_ix > 0 || mean_iy > 0) {//��ͼ������ϵ�а������ĵ㣬ȡ�̶��ߴ�������

        rid.center.x = mean_ix;
        rid.center.y = mean_iy;

        ExtractSampleByCenter(mean_ix, mean_iy, labelpoints, outsample);
    }
}

bool SampleGenerator::FindMaxContourCenter(const cv::Mat &mask_mat, int &center_ix, int &center_iy, cv::Rect &bbox)
{
    //��������ͨ����,ȷ������λ��
    std::vector<std::vector<cv::Point> > contours;
    //��ȡ���������������ڵ�����
    cv::findContours(mask_mat, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

    if (!contours.size())
        return 0;

    /// Draw contours
    cv::Mat result = cv::Mat::zeros(mask_mat.size(), CV_8UC3);
    int max_contour_id = 0;
    int contour_size = -9;
    for (int i = 0; i< contours.size(); i++)
    {
        int size = contours[i].size();
        if (size > contour_size) {
            contour_size = size;
            max_contour_id = i;
        }
    }

    double tx = 0, ty = 0, tz = 0;
    for (int i = 0; i < contours[max_contour_id].size(); i++) {
        tx += contours[max_contour_id][i].x;
        ty += contours[max_contour_id][i].y;
    }
    //����ͼ������ϵ�����ĵ���λ��
    int ix = tx / double(contours[max_contour_id].size());
    int iy = ty / double(contours[max_contour_id].size());

    center_ix = ix / 2;
    center_iy = iy * 4.5;
    bbox = cv::boundingRect(cv::Mat(contours[max_contour_id]));
    return 1;

    //���ӻ�
    /*
    cv::Mat drawing;
    result = cv::Mat::zeros(mask_mat.size(), CV_8UC3);
    drawContours(result, contours, max_contour_id, cv::Scalar(255, 0, 0));
    cv::resize(result, drawing, cv::Size(result.cols / 2, result.rows*4.5));
    cv::circle(drawing, cv::Point(center_ix, center_iy), 3, cv::Scalar(0, 0, 255), 2);
    cv::imshow("resultImage", drawing);
    */

}

void SampleGenerator::ExtractPointsInBbox(const cv::Rect bbox, std::vector<LABELLEDPONTS> &points, double &center_x, double &center_y, double &center_z)
{
    double tx, ty, tz;

    cv::Point left_top = bbox.tl();
    cv::Point right_bottom = bbox.br();

    left_top.x = left_top.x / 2;
    left_top.y = left_top.y*4.5;
    right_bottom.x = right_bottom.x / 2;
    right_bottom.y = right_bottom.y*4.5;

    tx = ty = tz = 0.0;

    //��������contourȷ����bbox�У�������������ϵ�µ�����λ��
    long count = 0;
    for (int i = 0; i < points.size(); i++) {
        if (points[i].img_x <= left_top.x || points[i].img_x >= right_bottom.x
            || points[i].img_y <= left_top.y || points[i].img_y >= right_bottom.y) {
            continue;
        }
        tx += points[i].loc.x;
        ty += points[i].loc.y;
        tz += points[i].loc.z;
        count++;
    }

    if (count) {
        center_x = tx / double(count);
        center_y = ty / double(count);
        center_z = tz / double(count);

        //����������ϵ��ȡ3ά��BBOX���ߴ��̶�
        cv::Point3f minBoxPt = cv::Point3f(center_x - 2.5, center_y - 2.5, center_z - 1.2);
        cv::Point3f maxBoxPt = cv::Point3f(center_x + 2.5, center_y + 2.5, center_z + 1.2);

        points.clear();
        LABELLEDPONTS onelabelpt;

        //����������ϵ�£����»�ȡ������
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
}

void SampleGenerator::ExtractSampleByCenter(int center_x, int center_y, std::vector<LABELLEDPONTS> &labelpoints, cv::Mat &outsample)
{
    cv::circle(dataimg, cv::Point(center_x, center_y), 2, cv::Scalar(255, 0, 0), 2);
    int width = 256;
    int height = 256;

    cv::Mat range_mat = cv::Mat::zeros(height, width, CV_8UC1);
    cv::Mat height_mat = cv::Mat::zeros(height, width, CV_8UC1);
    cv::Mat intensity_mat = cv::Mat::zeros(height, width, CV_8UC1);

    cv::Point left_top = cv::Point(center_x - width / 2, center_y - height / 2);
    cv::Point right_bottom = cv::Point(center_x + width / 2, center_y + height / 2);
    //��ȡ����
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
            int ix = labelpoints[i].img_x - center_x + width / 2;
            int iy = labelpoints[i].img_y - center_y + height / 2;

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

void SampleGenerator::GenerateAllSamplesInRangeImage(RMAP *prm_, SegLogLoader *seglog, cv::VideoWriter &out)
{
    prm = prm_;

    //determine all region id, prm->regnum is not enough
    std::unordered_set<int> idx_set;
    for (int y = 0; y < prm->len; y++) {
        for (int x = 0; x < prm->wid; x++) {
            if (prm->regionID[y*prm->wid+x] > 0){
                idx_set.insert(prm->regionID[y*prm->wid+x]);
            }
        }
    }
    std::map<int, int> regionIdMapLabel;
    cv::Mat outsample(cv::Size(1080, 144), CV_8UC1, cv::Scalar(0));
    std::unordered_set<int>::iterator iter;
    std::multimap<int,int> seedsMap;
    for (int i = 0; i < seglog->seednum; i ++) {
        seedsMap.insert(std::pair<int,int>(seglog->seeds[i].milli, i));
    }
    for (iter=idx_set.begin(); iter!=idx_set.end(); iter++){
        int regionid = *iter;
        int count = 0;
        int label = 0;
        typedef std::multimap<int, int>::iterator MITER;
        std::pair<MITER, MITER> range;
        range = seedsMap.equal_range(prm->millsec);

        for (MITER i = range.first; i != range.second; i ++) {
            if (seglog->seeds[i->second].prid == regionid) {
                label = seglog->seeds[i->second].lab;
                regionIdMapLabel.insert(std::pair<int, int>(regionid, label));
                count++;
            }
        }

    }

    cv::Mat rangeImg, mergeImg;
    cv::resize(cv::cvarrToMat(prm->rMap), rangeImg, cv::Size(1080, 144), cv::INTER_NEAREST);
    cv::cvtColor(rangeImg, rangeImg, CV_GRAY2BGR);
    mergeImg = rangeImg.clone();

    double yRatio = double(prm->len) / 144.0;
    double xRatio = double(prm->wid) / 1080.0;
    int maxid = 0;
    int minz = 100000;
    int maxz = -minz;

    // Build ground truth image
    for (int y = 0; y < 144; y++) {
        for (int x = 0; x < 1080; x++) {
            int yy = y * yRatio;
            int xx = x * xRatio;
            int tmpRegId = prm->regionID[yy*prm->wid+xx];
            int height = (prm->pts[yy*prm->wid+xx].z + 3.0) * 60;
            rangeImg.at<cv::Vec3b>(y, x)[1] = prm->pts[yy*prm->wid+xx].i;
            rangeImg.at<cv::Vec3b>(y, x)[2] = std::min(height, 255);

            if (tmpRegId > 0) {
                if (tmpRegId > maxid)
                    maxid = tmpRegId;
                if (regionIdMapLabel.find(tmpRegId) != regionIdMapLabel.end()) {
                    int r = seglog->colorTable[regionIdMapLabel[tmpRegId]][0];
                    int g = seglog->colorTable[regionIdMapLabel[tmpRegId]][1];
                    int b = seglog->colorTable[regionIdMapLabel[tmpRegId]][2];
                    int l = seglog->colorTable[regionIdMapLabel[tmpRegId]][3];
                    outsample.at<uchar>(y, x) = l;
                    mergeImg.at<cv::Vec3b>(y, x)[0] = b;
                    mergeImg.at<cv::Vec3b>(y, x)[1] = g;
                    mergeImg.at<cv::Vec3b>(y, x)[2] = r;
                }
                else if (tmpRegId < 22){
                    int r = seglog->colorTable[tmpRegId%22][0];
                    int g = seglog->colorTable[tmpRegId%22][1];
                    int b = seglog->colorTable[tmpRegId%22][2];
                    int l = seglog->colorTable[tmpRegId%22][3];
                    outsample.at<uchar>(y, x) = l;
                    mergeImg.at<cv::Vec3b>(y, x)[0] = b;
                    mergeImg.at<cv::Vec3b>(y, x)[1] = g;
                    mergeImg.at<cv::Vec3b>(y, x)[2] = r;
                }
            }
            else if (tmpRegId == GROUND) {
                outsample.at<uchar>(y, x) = 8;
                mergeImg.at<cv::Vec3b>(y, x)[0] = 208;
                mergeImg.at<cv::Vec3b>(y, x)[1] = 149;
                mergeImg.at<cv::Vec3b>(y, x)[2] = 117;
            }
            else if (tmpRegId <= 0 && prm->pts[yy*prm->wid+xx].i == 0) {
                outsample.at<uchar>(y, x) = 0;
            }
        }
    }
    // Write to image file
//    std::string imgname = "/home/gaobiao/Documents/2-1/ladybug/";
//    QDir dir(QString(imgname.c_str()));
//    if (!dir.exists()){
//        if (!dir.mkpath(dir.absolutePath()))
//            return ;
//    }
//    imgname += std::to_string(prm->millsec);
//    cv::imwrite(imgname + "_gt.png", outsample);
//    cv::imwrite(imgname + "_img.png", rangeImg);
//    cv::imwrite(imgname + "_merge.png", mergeImg);

    // Write to video
    cv::putText(mergeImg, std::to_string(prm->millsec), cv::Point(20,10), CV_FONT_BLACK, 0.6, cv::Scalar(0, 0, 255), 2);
    cv::imshow("test", mergeImg);
    cv::imshow("input", rangeImg);
    cv::waitKey(1);
    out << mergeImg;
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
