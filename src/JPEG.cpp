#include "JPEG.h"
#include "NxNDCT.h"
#include <math.h>

#include "JPEGBitStreamWriter.h"


#define DEBUG(x) do{ qDebug() << #x << " = " << x;}while(0)



// quantization tables from JPEG Standard, Annex K
uint8_t QuantLuminance[8*8] =
    { 16, 11, 10, 16, 24, 40, 51, 61,
      12, 12, 14, 19, 26, 58, 60, 55,
      14, 13, 16, 24, 40, 57, 69, 56,
      14, 17, 22, 29, 51, 87, 80, 62,
      18, 22, 37, 56, 68,109,103, 77,
      24, 35, 55, 64, 81,104,113, 92,
      49, 64, 78, 87,103,121,120,101,
      72, 92, 95, 98,112,100,103, 99 };
uint8_t QuantChrominance[8*8] =
    { 17, 18, 24, 47, 99, 99, 99, 99,
      18, 21, 26, 66, 99, 99, 99, 99,
      24, 26, 56, 99, 99, 99, 99, 99,
      47, 66, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99 };

static char quantizationMatrix[64] =
{
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77,
    24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99
};

struct imageProperties{
    int width;
    int height;
    int16_t* coeffs;
};


void DCTUandV(const char input[], int16_t output[], int N, double* DCTKernel)
{
    double* temp = new double[N*N];
    double* DCTCoefficients = new double[N*N];

    double sum;
    for (int i = 0; i <= N - 1; i++)
    {
        for (int j = 0; j <= N - 1; j++)
        {
            sum = 0;
            for (int k = 0; k <= N - 1; k++)
            {
                sum = sum + DCTKernel[i*N+k] * (input[k*N+j]);
            }
            temp[i*N + j] = sum;
        }
    }

    for (int i = 0; i <= N - 1; i++)
    {
        for (int j = 0; j <= N - 1; j++)
        {
            sum = 0;
            for (int k = 0; k <= N - 1; k++)
            {
                sum = sum + temp[i*N+k] * DCTKernel[j*N+k];
            }
            DCTCoefficients[i*N+j] = sum;
        }
    }

    for(int i = 0; i < N*N; i++)
    {
        output[i] = floor(DCTCoefficients[i]+0.5);
    }

    delete[] temp;
    delete[] DCTCoefficients;

    return;
}

uint8_t quantQuality(uint8_t quant, uint8_t quality) {
    // Convert to an internal JPEG quality factor, formula taken from libjpeg
    int16_t q = quality < 50 ? 5000 / quality : 200 - quality * 2;
    return clamp((quant * q + 50) / 100, 1, 255);
}

static void doZigZag(int16_t block[], uint8_t quantizationBlock[], int N, int DCTorQuantization)
{
    /* TO DO */
}

/* perform DCT */
imageProperties performDCT(char input[], int xSize, int ySize, int N, uint8_t quality, bool quantType)
{
    struct imageProperties image;
    image.width = xSize;
    image.height = ySize;
    image.coeffs = new int16_t[3*xSize*ySize/2];
    JPEGBitStreamWriter streamer("example.jpg");


    // Create NxN buffer for one input block
    char* inBlock = new char[N*N];

    // Generate DCT kernel
    double* DCTKernel = new double[N*N];
    GenerateDCTmatrix(DCTKernel, N);

    // Create buffer for DCT coefficients
    short* dctCoeffs = new short[N*N];

    // Extend image borders
    int x_size2, y_size2;
    char* input2;
    extendBorders(input, xSize , ySize, N, &input2, &x_size2, &y_size2);

    for(int y = 0; y < y_size2/N; y++){
        for(int x = 0; x < x_size2/N; x++){

            // Fill input block buffer
            for(int j=0; j<N; j++){
                for(int i=0; i<N; i++){
                    inBlock[j*N+i] = input2[(N*y+j)*(x_size2)+N*x+i];
                }
            }

            // Invoke DCT
            DCT(inBlock, dctCoeffs, N, DCTKernel);

            // Prepere DCT coefficients for drawing and put them in input
            for(int i = 0; i < N*N; i++){
                dctCoeffs[i] = abs(dctCoeffs[i]);
                if(dctCoeffs[i] < 0){
                    dctCoeffs[i] = 0;
                }else if(dctCoeffs[i] > 255){
                    dctCoeffs[i] = 255;
                }

                inBlock[i] = dctCoeffs[i];
            }

            // Write output values to output image
            for(int j=0; j<N; j++){
                for(int i=0; i<N; i++){
                    input2[(N*y+j)*(x_size2)+N*x+i] = inBlock[j*N+i];
                }
            }
        }
    }

    cropImage(input2, x_size2, y_size2, input, xSize, ySize);

    // Delete used memory buffers coefficients
    delete[] dctCoeffs;
    delete[] inBlock;
    delete[] DCTKernel;
    delete[] input2;
    return image;
}

void performJPEGEncoding(uchar Y_buff[], char U_buff[], char V_buff[], int xSize, int ySize, int quality)
{
	DEBUG(quality);
	
    auto s = new JPEGBitStreamWriter("example.jpg");
    char *Y_buff_corrected = new char[xSize * ySize];
    struct imageProperties image;
    double* DCTKernel = new double[8*8];
    GenerateDCTmatrix(DCTKernel, 8);


    int16_t *buff = new int16_t[xSize*ySize/4];

    for(int x = 0; x < xSize; x++)
    {
        for(int y = 0; y < ySize; y++)
        {
            Y_buff_corrected[y*xSize + x] = (char)Y_buff[y*xSize + x] - 128;
        }
    }

    image = performDCT(Y_buff_corrected, xSize, ySize, 8, quality, true);
    DCTU_
    DCTU_V

    YUV420_to_RGB(Y_buff_corrected, U_buff, V_buff);

    delete[] Y_buff_corrected;
    delete[] buff;
}
















