
#include "VideoSource.h"

#include "Persistence/instances.h"

#include <iostream>
#include <sstream>

using namespace std;

// For the time being, I am implementing webcam live capture.... All being well, more will follow...
constexpr double distortionParameter[5] = {0.2312, -0.7849, -0.0033, -0.0001,
                                           0.9172};       // k1 k2 d1 d2 k3
constexpr double oldK[4] = {520.9, 325.1, 521.0, 249.7};  // fx cx fy cy
constexpr double newK[4] = {530, 320, 530, 240};          // fx cx fy, cy

VideoSource::VideoSource(int camera_index) {

    std::cout << "  Initiating capture device (whatever it is)..." << std::endl;

    camera_index_ = camera_index;
    pcap = new cv::VideoCapture(camera_index_);  // by device number

    if (!pcap->isOpened()) {
        cerr << "Cannot open default capture device. Exiting... " << endl;
        exit(-1);
    }

    std::cout << "  Now capturing...." << std::endl;
    // obtaining the capture size
    int width = (int)pcap->get(cv::CAP_PROP_FRAME_WIDTH);
    int height = (int)pcap->get(cv::CAP_PROP_FRAME_HEIGHT);
    mirSize = cv::Size2i(width, height);
    cout << " Screen size (width , height) : " << width << " , " << height
         << endl;
};

cv::Size2i VideoSource::getSize() {
    return mirSize;
};

void VideoSource::GetAndFillFrameBWandRGB(cv::Mat_<uchar>& imBW,
                                          cv::Mat& imRGB) {
    if (!pcap->grab()) {
        cout << " Could not even grab the first frame! exiting..." << endl;
        exit(-1);
    }

    cv::Mat capFrame;
    pcap->retrieve(capFrame);
    /*cv::namedWindow("framed");
  cv::imshow("framed", capFrame);
  cv::waitKey(-1);*/
    capFrame.copyTo(imRGB);
    //imRGB = frame.clone(); // deep copy (in BGR!!!! We may need to do the conversion in the following line instead)
    //cv::cvtColor(frame, imRGB, CV_BGR2RGB); // We 'll see about this...
    imBW.create(imRGB.rows, imRGB.cols);

    cv::cvtColor(
        imRGB, imBW,
        cv::COLOR_BGR2GRAY);  // conversion from BGR (OpenCV default) to grayscale
}

ImageDataSet::ImageDataSet(const std::string& strDatasetDir,
                           const std::string& strAssociationFilePath)
    : mStrDatasetDir(strDatasetDir),
      mStrAssociationFilePath(strAssociationFilePath) {
    mirSize = cv::Size2i(640, 480);
}

void ImageDataSet::ReadImagesAssociationFile() {
    std::ifstream fAssociation;
    fAssociation.open(mStrAssociationFilePath.c_str());
    if (!fAssociation.is_open()) {
        cerr << "read {" << mStrAssociationFilePath << "} image data failed!"
             << endl;
        return;
    }

    while (!fAssociation.eof()) {
        std::string s;
        getline(fAssociation, s);
        if (!s.empty()) {
            std::stringstream ss;
            ss << s;
            double t;
            std::string sRGB, sD;
            ss >> t;
            mvTimestamps.push_back(t);
            ss >> sRGB;
            mvstrImageFilenamesRGB.push_back(mStrDatasetDir + "/" + sRGB);
            ss >> t;
            ss >> sD;
            mvstrImageFilenamesD.push_back(mStrDatasetDir + "/" + sD);
        }
    }
    cout << "get image nmu: " << mvstrImageFilenamesD.size() << endl;
}

void ImageDataSet::GetAndFillFrameBWandRGB(cv::Mat& imgRGB, cv::Mat& imgBW) {
    static bool isInited = false;
    if (!isInited) {
        ReadImagesAssociationFile();
        isInited = true;
    }

    static unsigned int index = 0;

    cv::Mat imgBGR =
        cv::imread(mvstrImageFilenamesRGB[index], cv::IMREAD_UNCHANGED);
    if (imgBGR.empty()) {
        cout << "read image{" << mvstrImageFilenamesRGB[index] << "} failed"
             << endl;
        return;
    }

    const cv::Mat oldK = (cv::Mat_<float>(3, 3) << 520.9, 0.0, 325.1, 0.0,
                          521.0, 249.7, 0.0, 0.0, 1.0);
    const cv::Mat newK = (cv::Mat_<float>(3, 3) << 530.0, 0.0, 320.0, 0.0,
                          530.0, 240.0, 0.0, 0.0, 1.0);
    const cv::Mat dis =
        (cv::Mat_<float>(5, 1) << 0.2312, -0.7849, -0.0033, -0.0001, 0.9172);
    //cout << "oldK:\n" << oldK << endl;
    //cout << "newK:\n" << newK << endl;
    //cout << "dis: " << dis << endl;
    cv::Mat temp;
    cv::undistort(imgBGR, temp, oldK, dis, newK);
    imgBGR = temp;

    cv::cvtColor(imgBGR, imgRGB, cv::COLOR_BGR2RGB);
    cv::cvtColor(imgBGR, imgBW, cv::COLOR_BGR2GRAY);

    //cv::imshow("imgBGR", imgBGR);
    //cv::imshow("imgBw", imgBW);
    //cv::waitKey(0);

    index++;

    if (index == mvstrImageFilenamesRGB.size()) {
        index = 0;
    }
}
