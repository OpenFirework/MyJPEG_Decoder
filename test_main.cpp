#include "jpeg_decoder.h"
#include "bmp_code.h"
#include <iostream>
#include <fstream>

int main(int argc, char *argv[]) {
    if(argc !=3 ) {
        std::cout<<"please use as test.bin <jpegpath> <bmppath>\n";
        return  -1;
    }

    MyJPEG::jpegDecoder decoder;
    byte *bgrdata = nullptr;
    int imgwidth = 0;
    int imgheidht = 0;

    /*************jpeg文件解码***********/
    decoder.imgread(argv[1], imgwidth, imgheidht, bgrdata);

    /*************将解码后的bgr数据存储，数据内容为[bgr,bgr,bgr,...,bgr]*************/
    std::ofstream outfile;
    outfile.open("jpeg_decoded_bgr.bin",std::ios::out|std::ios::binary);
    outfile.write((char*)bgrdata, imgwidth*imgheidht*3);
    outfile.close();


    std::cout<<"\n*****jpeg图像解码结束，已将bgr数据存储为二进制数据：jpeg_decoded_bgr.bin******\n\n";
    MyBMP::BMP bmpdecoder;

    /*************BMP文件解码***********/
    byte *bgrdata1 = nullptr;
    int imgwidth1 = 0;
    int imgheidht1 = 0;
    bmpdecoder.decode(argv[2], imgwidth1, imgheidht1, bgrdata1);

    /*************存储BMP文件中的图像数据存储位为二进制bgr数据***********/
    std::ofstream outfile1;
    outfile1.open("bmp_decoded_bgr.bin",std::ios::out|std::ios::binary);
    outfile1.write((char*)bgrdata1, imgwidth1*imgheidht1*3);
    outfile1.close();
    std::cout<<"\n*****bmp图像解码结束，已将bgr数据存储为二进制数据：bmp_decoded_bgr.bin******\n\n";

    /*****将前面解码的jpeg数据转换为bmp数据***********/
    bmpdecoder.encode("jpeg_to_bmp_test.bmp",imgwidth, imgheidht, bgrdata);
    std::cout<<"\n*****测试：将jpeg解码的数据存储位bmp图像，验证正确性，存储文件为：jpeg_to_bmp_test.bmp******\n\n";


    /******resize 测试，将图片缩放位任意尺寸********/
    byte *resizedata = nullptr;
    int resizewidth = 0;
    int resizeheight = 0;
    float scale = 1.5;
    bmpdecoder.resize(bgrdata, imgwidth, imgheidht, resizedata, resizewidth, resizeheight, scale);    
    bmpdecoder.encode("jpeg_resized.bmp",resizewidth, resizeheight, resizedata);
    std::cout<<"图像resize测试（双线型插值法）,图像原尺寸（"<<imgwidth<<","<<imgheidht<<"),resize 后的尺寸为（"<<resizewidth<<","<< \
    resizeheight<<"),resize后的图像为：jpeg_resized.bmp\n";

    if(bgrdata != nullptr) {
        delete[] bgrdata;
    }

    if(bgrdata1 != nullptr) {
        delete[] bgrdata1;
    }

    if(resizedata != nullptr) {
        delete[] resizedata;
    }

    return 0;
}
