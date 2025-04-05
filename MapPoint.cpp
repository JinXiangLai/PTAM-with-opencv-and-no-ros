// George Terzakis 2016
//
// University of Portsmouth
//
// Code based on PTAM by Klein and Murray (Copyright 2008 Isis Innovation Limited)

#include "MapPoint.h"
#include "KeyFrame.h"

using namespace std;

void MapPoint::RefreshPixelVectors() {
    // Get the KF in which the patch was originally found
    // (i.e., a "home" KF )
    KeyFrame::Ptr k = pPatchSourceKF;

    // And now compute the 3D coordinates of the mappoint
    // in the coordinate frame of the home/source KF
    // 地图点变换到首次观测到它的源关键帧得到Pc
    cv::Vec3f v3PlanePoint_C = k->se3CfromW * v3WorldPos;

    // Now, we compute the DEPTH of the 3D location above its own projection in the Euclidean plane Z = 1.
    // NOTE!!! NOTE!!! This computation is valid ONLY if we assume that the normal is the same direction as the optical axis (Z)
    //              *** NOTE AGAIN!! I just HAD TO rename this variable from dCamHeight to dCamDepth ... ********
    //      because IT SIMPLY IS DEPTH and NOT height!!!
    // 获得在相机光轴向量上的投影，诶，那不就是Z吗？
    float dCamDepth = fabs(v3PlanePoint_C.dot(v3Normal_NEC));
    // 经过测试就是Z，除非是光轴存在偏差，也就是内参矩阵的 K[0][1]!=0
    //cout << "dCamDepth, Z: " << dCamDepth << ", " << v3PlanePoint_C.row(2)(0) << endl; 

    // NOTE!!! Again, the following variable names are misleading unfortunately... But they actually, in some way, tell the truth!!!!
    //          So I kept them...
    //
    //         The following dot products yield the angles (cosines in particular) from the Z axis (recall normal is the -Z direction):
    //
    //         a) "dPixelRate" is effectively the angle of the feature's projection ray from the normal (also optical axis if n = (0, 0,-1) ).
    //
    //         n) "dOneRightRate" is the angle cosine of the right pixel's Euclidean projection ray from the normal (also optical axis if n = (0, 0,-1) ).
    //
    //         c) "dOneDownRate" is the angle cosine of the down pixel's Euclidean projection ray from the normal (also optical axis if n = (0, 0,-1) ).
    // 这里是计算cosθ 即：
    // 计算中心点、向右一个像素点和向下一个像素点的归一化向量与光轴方向的夹角余弦

    // patch中心像素与光轴的夹角余弦，只需到归一化平面即可计算
    float dPixelRate = fabs(v3Center_NEC.dot(v3Normal_NEC)); 
    // 同理，以下是向右、向下一个像素偏差的归一化平面射线与光轴的夹角余弦
    float dOneRightRate = fabs(v3OneRightFromCenter_NEC.dot(v3Normal_NEC));
    float dOneDownRate = fabs(v3OneDownFromCenter_NEC.dot(v3Normal_NEC));
    // 均不为0，为0.9几接近1的量级
    //printf("dPixelRate=%f, dOneRightRate=%f, dOneDownRate=%f\n", dPixelRate, dOneRightRate, dOneDownRate);

    // 现在，我们实际上可以通过简单的三角学运算，计算出这些位置（右、下、中）在现实世界中的反投影射线长度。
    // Now we can actually work out backprojection ray-lengths into the real world that these locations (right, down, center)
    // by simply using a bit of trigonometry
    //
    // 通过三角学计算在3D空间中对应图像像素点的位置
    // 这些向量表示在3D空间中，图像平面上的像素移动对应的实际位置变化

    // scaled vector：表示正确尺度深度
    // i. "v3CenterOnPlane_C" is the scaled vector of the real-world location from its normalized Euclidean projection along the y-axis
    // 深度会放大这个误差，因此需要将深度作用于余弦夹角引起的偏差
    // d = a*cos0, 这里计算的就是实际射线了，a = d/cos0，而射线就是3D空间的坐标了
    cv::Vec3f v3CenterOnPlane_C = (dCamDepth / dPixelRate) * v3Center_NEC;

    // ii. "v3OneRightOnPlane_C" is the scaled vector of the real-world location from the projection ray of the "right pixel" along the x-axis.
    cv::Vec3f v3OneRightOnPlane_C =
        (dCamDepth / dOneRightRate) * v3OneRightFromCenter_NEC;

    // iii. "v3OneDownOnPlane_C" is the scaled vector of the real-world location from the projection ray of the "down pixel" along the y-axis.
    cv::Vec3f v3OneDownOnPlane_C =
        (dCamDepth / dOneDownRate) * v3OneDownFromCenter_NEC;

    //************* INTUITION: v3OneDownOnPlane_C and v3OneRightOnPlane_C are in fact, the BAT SIGNAL of the patch in the 3D sky!!!! *********

    // Express the difference vectors of these SCALED projection vectors wrt World frame orientation.
    // 计算一个像素偏差在实际相机坐标系下平面引起的方向向量偏差，
    // 并通过Rt把偏差转到w系下，我们关心的是扰动变化，而不是偏差像素的实际空间位置
    SO3<> Rt = k->se3CfromW.get_rotation().inverse();
    v3PixelGoRight_W = Rt * (v3OneRightOnPlane_C - v3CenterOnPlane_C);
    v3PixelGoDown_W = Rt * (v3OneDownOnPlane_C - v3CenterOnPlane_C);
}
