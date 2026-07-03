#include "Helper.h"
#include <Eigen/Core>
#include <sstream>

using namespace Eigen;

void UHelper::ConvertCoordinateSystem(FTransform& Transform, const EAxis SrcXInDstAxis, const EAxis SrcYInDstAxis, const EAxis SrcZInDstAxis)
{
}

FTransform UHelper::ConvertUnrealToOpenCV(FTransform Transform)
{
    return Transform;
}

FTransform UHelper::ConvertOpenCVToUnreal(FTransform Transform)
{
    return Transform;
}

Matrix3d UHelper::rodrigues(const Vector3d& rvec) {
    return Matrix3d::Identity();
}

double UHelper::variance(const VectorXd& vec)
{
    return 0.0;
}

FVector UHelper::eigenVectorToUnreal(const Vector3d& vec)
{
    return FVector::Zero();
}

FMatrix UHelper::eigenMatrixToUnreal(const Matrix4d& mat)
{
    return FMatrix::Identity;
}

Vector3d UHelper::unrealVectorToEigen(const FVector& vec)
{
    return Vector3d::Zero();
}

Matrix4d UHelper::extrinsicFromRt(const Matrix3d& R, const Vector3d& tvec)
{
    return Matrix4d::Identity();
}
