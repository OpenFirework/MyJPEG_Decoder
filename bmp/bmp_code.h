#ifndef _BMP_CODE_H_
#define _BMP_CODE_H_

typedef unsigned char byte;

typedef struct tagBITMAPFILEHEADER
{
    unsigned short bfType;        // 19778，必须是BM字符串，对应的十六进制为0x4d42,十进制为19778，否则不是bmp格式文件
    unsigned int   bfSize;        // 文件大小 以字节为单位(2-5字节)
    unsigned short bfReserved1;   // 保留，必须设置为0 (6-7字节)
    unsigned short bfReserved2;   // 保留，必须设置为0 (8-9字节)
    unsigned int   bfOffBits;     // 从文件头到像素数据的偏移  (10-13字节)
} BITMAPFILEHEADER;


typedef struct tagBITMAPINFOHEADER
{
    unsigned int    biSize;          // 此结构体的大小 (14-17字节)
    unsigned int    biWidth;         // 图像的宽  (18-21字节)
    unsigned int    biHeight;        // 图像的高  (22-25字节)
    unsigned short  biPlanes;        // 表示bmp图片的平面属，显然显示器只有一个平面，所以恒等于1 (26-27字节)
    unsigned short  biBitCount;      // 一像素所占的位数，一般为24   (28-29字节)
    unsigned int    biCompression;   // 说明图象数据压缩的类型，0为不压缩。 (30-33字节)
    unsigned int    biSizeImage;     // 像素数据所占大小, 这个值应该等于上面文件头结构中bfSize-bfOffBits (34-37字节)
    unsigned int    biXPelsPerMeter; // 说明水平分辨率，用象素/米表示。一般为0 (38-41字节)
    unsigned int    biYPelsPerMeter; // 说明垂直分辨率，用象素/米表示。一般为0 (42-45字节)
    unsigned int    biClrUsed;       // 说明位图实际使用的彩色表中的颜色索引数（设为0的话，则说明使用所有调色板项）。 (46-49字节)
    unsigned int    biClrImportant;  // 说明对图象显示有重要影响的颜色索引的数目，如果是0，表示都重要。(50-53字节)
} BITMAPINFOHEADER;


namespace MyBMP { 
    class BMP{
     public:
        BMP();
        //filename:传入数据，width，height，bgrdata：传出数据，
        void decode(const char *filename, int &width, int &height, byte *&bgrdata);

        //filename,width,height,bgrdata都是传入的数据
        void encode(const char *filename, int width, int height, byte *&bgrdata);

        int resize(byte *&inputdata, int inwidth, int inheight, byte *&outdata, int &outwidth, int &outheight, float scale);

        void showBmpHead(BITMAPFILEHEADER pBmpHead);
        void showBmpInfoHead(BITMAPINFOHEADER pBmpinfoHead);
     private:
        BITMAPFILEHEADER fileHeader;
        BITMAPINFOHEADER infoHeader;

    };
}


#endif //