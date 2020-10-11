#ifndef _JPEG_DECODER_H_
#define _JPEG_DECODER_H_

#define PI 3.14154926
typedef unsigned char byte;

namespace MyJPEG {

    class jpegDecoder{
      public:
        jpegDecoder();
        int test();
        //jpegdata:输入数据， rgbdata：输出rgb数据。 imgwidth：输出图片的宽，imgheidht：输出图片的高
        //return 0:sucess; return -1:failed
        int readinfo(byte *&jpegdata, byte *&bgrdata, int &imgwidth, int &imgheidht);

         //filename:输入数据， rgbdata：输出rgb数据。 imgwidth：输出图片的宽，imgheidht：输出图片的高
         //return 0:sucess; return -1:failed
        int imgread(const char *filename, int &width, int &height, byte *&bgrdata);
        
      private:
        int picheight;
        int picwidth;
        int datapos;
    };

}



#endif  //_JPEG_DECODER_H_