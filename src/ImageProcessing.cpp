
#include "ImageProcessing.h"
#include "ColorSpaces.h"
#include "JPEG.h"

#include <cmath>

#include <QDebug>
#include <QString>
#include <QImage>

#define DEBUG(x) do{ qDebug() << #x << " = " << x;}while(0)

void imageProcessingFun(const QString& progName, QImage& outImgs, const QImage& inImgs, const QVector<double>& params)
{
	// TO DO

	/* Create buffers for YUV image */
    int X_SIZE = inImgs.width(),
        Y_SIZE = inImgs.height();
    int imgSize = X_SIZE*Y_SIZE;
    uchar* Y_buff = new uchar[imgSize];
    char* U_buff = new char[imgSize / 4];
    char* V_buff = new char[imgSize / 4];

	/* Create empty output image */
	outImgs = QImage(inImgs.width(), inImgs.height(), inImgs.format());

	/* Convert input image to YUV420 image */
    RGBtoYUV420(inImgs.bits(), X_SIZE, Y_SIZE, Y_buff, U_buff, V_buff);

    if(progName == QString("JPEG Encoder"))
	{	
		/* Perform NxN DCT */
        performJPEGEncoding(Y_buff, U_buff, V_buff, X_SIZE, Y_SIZE, params[0]);
	}


    /* Convert YUV image back to RGB */
    YUV420toRGB(Y_buff, U_buff, V_buff, inImgs.width(), inImgs.height(), outImgs.bits());

    outImgs = QImage("example.jpg");

	/* Delete used memory buffers */
    delete[] Y_buff;
    delete[] U_buff;
    delete[] V_buff;

}

