#include "bmp_code.h"
#include <fstream>
#include <iostream>
#include <string.h>
#include <math.h>

MyBMP::BMP::BMP() {

}

void MyBMP::BMP::decode(const char *filename, int &width, int &height, byte *&bgrdata) {
    std::ifstream infile;
    infile.open(filename, std::ios::in|std::ios::binary);
    infile.seekg(0, infile.end);
    int bmpdatalength = infile.tellg();
    infile.seekg(0, infile.beg);
    std::cout << "bmp data length is " << bmpdatalength<<"\n";
    //all data
    byte *bmpdata = new byte[bmpdatalength];
    byte *copyptr = bmpdata;  //保存首地址，便于以后释放
    infile.read((char*)bmpdata, bmpdatalength);  //读取编码的jpeg文件数据到内存中
    infile.close();

    //read file header
    memcpy(&fileHeader.bfType, bmpdata, sizeof(fileHeader.bfType));
    bmpdata = bmpdata + sizeof(fileHeader.bfType);
    memcpy(&fileHeader.bfSize, bmpdata, sizeof(fileHeader.bfSize));
    bmpdata = bmpdata + sizeof(fileHeader.bfSize);
    memcpy(&fileHeader.bfReserved1, bmpdata, sizeof(fileHeader.bfReserved1));
    bmpdata = bmpdata + sizeof(fileHeader.bfReserved1);
    memcpy(&fileHeader.bfReserved2, bmpdata, sizeof(fileHeader.bfReserved2));
    bmpdata = bmpdata + sizeof(fileHeader.bfReserved2);
    memcpy(&fileHeader.bfOffBits, bmpdata, sizeof(fileHeader.bfOffBits));
    bmpdata = bmpdata + sizeof(fileHeader.bfOffBits);
    showBmpHead(fileHeader);

    //read info header
    memcpy(&infoHeader, bmpdata, sizeof(BITMAPINFOHEADER));
    bmpdata = bmpdata + sizeof(BITMAPINFOHEADER);
    showBmpInfoHead(infoHeader);

    height = infoHeader.biHeight;
    width = infoHeader.biWidth;
   
    int bytes_per_row = 0;  
    int tmp = floor((24*width + 31)*1.0/32);
    bytes_per_row = tmp*4;    
    int datasize = bytes_per_row*height;
    if((bmpdatalength - fileHeader.bfOffBits) != (infoHeader.biSizeImage)) {
        if((bmpdatalength - fileHeader.bfOffBits) != datasize) {
            std::cout<<"******BMP DATA is WRONG******\n";
        }
    }
    //给外部使用的，不能删除
    bgrdata = new byte[height*width*3];

    //bmp的数据存储顺序是从左下到右上，而一般的习惯是从左上到右下，因此做一个转置
    for(int h=0;h<height;h++) {
        for(int w=0;w<width;w++) {
            int offset = h*width + w;
            int inflect = (height - 1 -h)*bytes_per_row + w*3;
            bgrdata[offset*3] = bmpdata[inflect];
            bgrdata[offset*3+1] = bmpdata[inflect+1];
            bgrdata[offset*3+2] = bmpdata[inflect+2];

        }
    }
    
    if(copyptr != nullptr) {
        delete[] copyptr;
    }

}

void MyBMP::BMP::showBmpHead(BITMAPFILEHEADER pBmpHead) {
    printf("BMP文件大小：%dkb\n", pBmpHead.bfSize/1024);
    printf("保留字必须为0：%d\n",  pBmpHead.bfReserved1);
    printf("保留字必须为0：%d\n",  pBmpHead.bfReserved2);
    printf("实际位图数据的偏移字节数: %d\n",  pBmpHead.bfOffBits);
}

void MyBMP::BMP::showBmpInfoHead(BITMAPINFOHEADER pBmpinfoHead)
{
    //定义显示信息的函数，传入的是信息头结构体
    printf("位图信息头:\n" );
    printf("信息头的大小:%d\n" ,pBmpinfoHead.biSize);
    printf("位图宽度:%d\n" ,pBmpinfoHead.biWidth);
    printf("位图高度:%d\n" ,pBmpinfoHead.biHeight);
    printf("图像的位面数(位面数是调色板的数量,默认为1个调色板):%d\n" ,pBmpinfoHead.biPlanes);
    printf("每个像素的位数:%d\n" ,pBmpinfoHead.biBitCount);
    printf("压缩方式:%d\n" ,pBmpinfoHead.biCompression);
    printf("图像的大小:%d\n" ,pBmpinfoHead.biSizeImage);
    printf("水平方向分辨率:%d\n" ,pBmpinfoHead.biXPelsPerMeter);
    printf("垂直方向分辨率:%d\n" ,pBmpinfoHead.biYPelsPerMeter);
    printf("使用的颜色数:%d\n" ,pBmpinfoHead.biClrUsed);
    printf("重要颜色数:%d\n" ,pBmpinfoHead.biClrImportant);
}

//外部传入bgr数据，保存为bmp文件，文件名为filename
void MyBMP::BMP::encode(const char *filename, int width, int height, byte *&bgrdata) {
    BITMAPFILEHEADER header;
    BITMAPINFOHEADER info;
    int bytes_per_row = 0;  
    int tmp = floor((24*width + 31)*1.0/32);
    bytes_per_row = tmp*4;    
    int datasize = bytes_per_row*height;
    header.bfType = 0x4d42;
    header.bfSize = datasize + 54;
    header.bfReserved1 = 0;
    header.bfReserved2 = 0;
    header.bfOffBits = 54;

    info.biSize = 40;
    info.biWidth = width;
    info.biHeight = height;
    info.biPlanes = 1;
    info.biBitCount = 24;
    info.biCompression = 0;
    info.biSizeImage = 0;
    info.biXPelsPerMeter = 0;
    info.biYPelsPerMeter = 0;
    info.biClrUsed = 0;
    info.biClrImportant = 0;

    byte *bmpdata = new byte[width*height*3];
    byte *copyptr = bmpdata;
   // bmp的数据存储顺序是从左下到右上，而一般的习惯是从左上到右下，因此做一个转置
    for(int h=0;h<height;h++) {
        for(int w=0;w<width;w++) {           
            int offset = h*width + w;
            int inflect = (height - 1 -h)*width + w;
            bmpdata[offset*3] = bgrdata[inflect*3];
            bmpdata[offset*3+1] = bgrdata[inflect*3+1];
            bmpdata[offset*3+2] = bgrdata[inflect*3+2];
        }
    }
    int zeros = bytes_per_row-width*3;
    //保存到文件
    std::ofstream outfile;
    outfile.open(filename, std::ios::out|std::ios::binary);
    outfile.write((char*)&header.bfType, sizeof(header.bfType));
    outfile.write((char*)&header.bfSize, sizeof(header.bfSize));
    outfile.write((char*)&header.bfReserved1, sizeof(header.bfReserved1));
    outfile.write((char*)&header.bfReserved2, sizeof(header.bfReserved2));
    outfile.write((char*)&header.bfOffBits, sizeof(header.bfOffBits));

    outfile.write((char*)&info, sizeof(info));
    byte zero = 0;
    for(int h=0;h<height;h++) { 
        outfile.write((char*)bmpdata, width*3);
        bmpdata = bmpdata + width*3;
        for(int i=0;i<zeros; i++) {
            outfile.write((char*)&zero, 1);
        }
    }

    outfile.close();
    if(copyptr != nullptr) {
        delete[] copyptr;
    }
}


int MyBMP::BMP::resize(byte *&inputdata, int inwidth, int inheight, byte *&outdata, int &outwidth, int &outheight, float scale) {

    float xscale = 0.0;
    float yscale = 0.0;
    if((outheight <= 0) || (outheight <= 0)) {
        std::cout<<"resize use scale and scale is "<<scale<<"\n";
        if(scale > 0) {
            outwidth = inwidth*scale;
            outheight = inheight*scale;
            xscale = inwidth*1.0/outwidth;
            yscale = inheight*1.0/outheight;
        } else {
            std::cout<<"scale value is wrong(less than 0)\n";
        }
       
    } else {
        std::cout<<"resize use outwidth and outheight, resized width is "<<outwidth<<" heigt is "<<outheight<<"\n";
        xscale = inwidth*1.0/outwidth;
        yscale = inheight*1.0/outheight;

    }
    std::cout<<"resized width is "<<outwidth<<" height is "<<outheight<<"\n";

    outdata = new byte[3*outwidth*outheight];

    for(int h=0; h<outheight; h++) {
        for(int w=0; w<outwidth; w++) {
            float srcx = w*xscale;
            float srcy = h*yscale;
            int topx = srcx;
            int topy = srcy;
            int reflect = topy*inwidth+topx;
            int reflect2 = 0;
            int topy2 = 0;
            int topx2 = 0;
            if(topy+1 > (inheight-1)){
                reflect2 = topy*inwidth+topx;
                topy2 = topy;
            } else {
                reflect2 = (topy+1)*inwidth+topx;
                topy2 = topy+1;
            }
            if(topx +1 > (inwidth-1)) {
                topx = topx - 1;
            }
            float fr = (topy2-srcy)*(topx+1-srcx)*inputdata[reflect*3] +
                       (topy2-srcy)*(srcx - topx)*inputdata[reflect*3+3] +
                        (srcy-topy)*(topx+1-srcx)*inputdata[reflect2*3] +
                        (srcy-topy)*(srcx-topx)*inputdata[reflect2*3+3];
            
            float fg = (topy2-srcy)*(topx+1-srcx)*inputdata[reflect*3+1] +
                       (topy2-srcy)*(srcx - topx)*inputdata[reflect*3+3+1] +
                        (srcy-topy)*(topx+1-srcx)*inputdata[reflect2*3+1] +
                        (srcy-topy)*(srcx-topx)*inputdata[reflect2*3+3+1];


            float fb = (topy2-srcy)*(topx+1-srcx)*inputdata[reflect*3+2] +
                       (topy2-srcy)*(srcx - topx)*inputdata[reflect*3+3+2] +
                        (srcy-topy)*(topx+1-srcx)*inputdata[reflect2*3+2] +
                        (srcy-topy)*(srcx-topx)*inputdata[reflect2*3+3+2];

            int offset = h*outwidth + w;
            outdata[offset*3] = fr;
            outdata[offset*3+1] = fg;
            outdata[offset*3+2] = fb;
        }
    }

}