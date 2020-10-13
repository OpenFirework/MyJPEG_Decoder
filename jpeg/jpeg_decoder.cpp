#include "jpeg_decoder.h"
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <math.h>
#include <fstream>

struct hufcode {
    int codeLength;   // 0< codeLength<16
    int encodeValue;  //编码值
    byte realVale;    //实际值

};

double cu[8],cv[8];  //离散余弦变换中间计算值暂存
double cos_cache[200];

//保存图像的实际宽和高
struct {
    int height;
    int width;
} image;

unsigned char maxWidth, maxHeight;  //水平和垂直最大采样因子，一般为2：2

//RGB数据
struct RGB {
    unsigned char R, G, B;
};

//颜色分量单元
typedef double BLOCK[8][8];

//交流哈夫曼编码
struct acCode {
    unsigned char len;
    unsigned char zeros;
    int value;
};
//存储量化表，一般2个就够了，每个表8*8
int quantTable[4][128];

int bmpwidth;
int bmpheight;

std::vector<hufcode> YDCHT;
std::vector<hufcode> YACHT;

std::vector<hufcode> UVDCHT;
std::vector<hufcode> UVACHT;

struct {
    unsigned char id;
    unsigned char width;  //水平采样因子
    unsigned char height;  //垂直采样因子
    unsigned char quant;   //量化表ID
} subVector[4];

class MCU{
public:
    BLOCK mcu[4][2][2];
    void show() {
        printf("*************** mcu show ***********************\n");
        for (int id = 1; id <= 3; id++) {
            for (int h = 0; h < subVector[id].height; h++) {
                for (int w = 0; w < subVector[id].width; w++) {
                    printf("mcu id: %d, %d %d\n", id, h, w);
                    for (int i = 0; i < 8; i++) {
                        for (int j = 0; j < 8; j++) {
                            printf("%lf ", mcu[id][h][w][i][j]);
                        }
                        printf("\n");
                    }
                }
            }
        }
    };
   
    //反量化
    void quantify() {
        for (int id = 1; id <= 3; id++) {
            for (int h = 0; h < subVector[id].height; h++) {
                for (int w = 0; w < subVector[id].width; w++) {
                    for (int i = 0; i < 8; i++) {
                        for (int j = 0; j < 8; j++) {
                            mcu[id][h][w][i][j] *= quantTable[subVector[id].quant][i*8 + j];
                        }
                    }
                }
            }
        }
    };
   
    void izigzag() {
        for (int id = 1; id <= 3; id++) {
            for (int h = 0; h < subVector[id].height; h++) {
                for (int w = 0; w < subVector[id].width; w++) {
                    //8*8的数组拉直，暂时存储到input数组中
                    int input[64];
                    for (int i = 0; i < 8; i++) {
                        for (int j = 0; j < 8; j++) {
                            input[i*8+j] = mcu[id][h][w][i][j];
                        }
                    }
                    //逆向zaizag,并存回到mcu
                    int k =0;
                    for(int index = 0; index < 15;index++) {
                        for(int i=0;i<8;i++){
                            for(int j=0;j<8;j++) {
                                if(i+j == index) {
                                    if(index%2 == 0) {
                                        mcu[id][h][w][j][i] = input[k];
                                        k++;
                                    } else {
                                        mcu[id][h][w][i][j] = input[k];
                                        k++;
                                    }
                                }
                            }
                        }
                    }
                    //逆向zaizag
                }
            }
        }
    };
   
   //不高效，待改进
    void idct() {
        for (int id = 1; id <= 3; id++) {
            for (int h = 0; h < subVector[id].height; h++) {
                for (int w = 0; w < subVector[id].width; w++) {
                    //逆离散余弦变换的结果暂存到out数组
                    double out[8][8] = {0};
		    //逆离散余弦变换计算过程
		   /*
                    for(int i=0; i<8; i++) {
                        for(int j=0; j<8; j++) {
                            float temp = 0.0;
                            for(int u=0; u<8; u++) {
                                for(int v=0; v<8; v++) {
                                    temp += cu[u]*cv[v]*mcu[id][h][w][u][v]*cos_cache[(2*i+1)*u]*cos_cache[(2*j+1)*v];
                                }
                            }       
                            out[i][j] = temp;
                        }
                    }*/
	           //优化
		    double s[8][8] = {};
		    for (int j = 0; j < 8; j++) {
			for (int x = 0; x < 8; x++) {
			    for (int y = 0; y < 8; y++) {
				s[j][x] += cu[y] * mcu[id][h][w][x][y] * cos_cache[(j + j + 1) * y];
			    }
			}
		    }                    
		    for (int i = 0; i < 8; i++) {
			for (int j = 0; j < 8; j++) {
			    for (int x = 0; x < 8; x++) {
				out[i][j] += cu[x] * s[j][x] * cos_cache[(i + i + 1) * x];
			    }
			}
		    }
                    //存回mcu中
                    for (int i = 0; i < 8; i++) {
                        for (int j = 0; j < 8; j++) {
                            mcu[id][h][w][i][j] = out[i][j];
                        }
                    }
                }
            }
        }
    }
   
    void decode() {
        this->quantify();
        this->izigzag();
        this->idct();
    }
 
    RGB **toRGB() {
        RGB **ret = new RGB *[maxHeight*8];
        for (int i = 0; i < maxHeight*8; i++) { 
            ret[i] = new RGB[maxWidth*8];
        }

        for (int i = 0; i < maxHeight * 8; i++) {
            for (int j = 0; j < maxWidth * 8; j++) {
                double Y = trans(1, i, j);
                double Cb = trans(2, i, j);
                double Cr = trans(3, i, j);
                ret[i][j].R = chomp(Y + 1.402*Cr + 128);
                ret[i][j].G = chomp(Y - 0.34414*Cb - 0.71414*Cr + 128);
                ret[i][j].B = chomp(Y + 1.772*Cb + 128);
            }
        }
        return ret;
    }

private:
    unsigned char chomp(double x) {
        if (x > 255.0) {
            return 255;
        } else if (x < 0) {
            return 0;
        } else {
            return (unsigned char) x;
        }
    }
   
   //颜色分量id, 对应在MCU中的位置h,w
    double trans(int id, int h, int w) {
        int vh = h * subVector[id].height / 2;  
        int vw = w * subVector[id].width / 2;
        return mcu[id][vh / 8][vw / 8][vh % 8][vw % 8];
    }

};

void createhuftable(int tree[16], byte *value, int length, std::vector<hufcode> &OUTT);
void readAPP0(byte *&jpegdata);
void readDQT(byte *&jpegdata);
void readSOF0(byte *&jpegdata);
void reaDHT(byte *&jpegdata);
void readSOS(byte *&jpegdata);
void readCOM(byte *&jpegdata);
void decodeMCU(byte *&framedata, byte *&bgrdata);

MyJPEG::jpegDecoder::jpegDecoder() {
    picheight = -1;
    picwidth = -1;
    datapos = 0;
}

int MyJPEG::jpegDecoder::test() {

}

void init_c() {
    for(int i=0;i<8;i++) {
        if(i==0) {
            cu[i] = sqrt(1.0/8.0);
            cv[i] = sqrt(1.0/8.0);
        } else {
            cu[i] = sqrt(2.0/8.0);
            cv[i] = sqrt(2.0/8.0);
        }
    }
}

void init_cos_cache() {
    for (int i = 0; i < 200; i++) {
        cos_cache[i] = cos(i * M_PI / 16.0);
    }
}

int MyJPEG::jpegDecoder::readinfo(byte *&jpegdata, byte *&bgrdata, int &imgwidth, int &imgheidht) {
    init_c();
    init_cos_cache();
    unsigned char start;
    unsigned short two;
    memcpy(&start, jpegdata++, 1);
    if(start == 0xFF) {
        memcpy(&start, jpegdata++, 1);
        if(start == 0xD8) {          //jpeg文件开头标识符：0XFFD8
            std::cout<<"This is a jpeg file and open success!\n";
        }
        else {
            std::cout<<"This is not a jpeg file, decode failed!\n";
        } 
    }

    memcpy(&start, jpegdata++, 1);
    while(start == 0xFF) {
        memcpy(&start, jpegdata++, 1);
        switch (start) {
            case 0xE0:
            case 0xE1:
            case 0xE2:
            case 0xE3:
            case 0xE4:
            case 0xE5:
            case 0xE6:
            case 0xE7:
            case 0xE8:
            case 0xE9:
            case 0xEA:
            case 0xEB:
            case 0xEC:
            case 0xED:
            case 0xEE:
            case 0xEF:
                readAPP0(jpegdata);
                break;
            case 0xFE:
                readCOM(jpegdata);
                break;
            case 0xDB:
                readDQT(jpegdata);
                break;
            case 0xC0:
                readSOF0(jpegdata);
                imgwidth = image.width;
                imgheidht = image.height;
                break;
            case 0xC4:
                reaDHT(jpegdata);
                break;
            case 0xDA:
                readSOS(jpegdata);
                decodeMCU(jpegdata, bgrdata);
                break;
            default:
            break;
        }
        memcpy(&start, jpegdata++, 1);
    }
    return 0;
}

int MyJPEG::jpegDecoder::imgread(const char *filename, int &width, int &height, byte *&bgrdata) {
    //读文件
    std::ifstream infile;
    infile.open(filename, std::ios::in|std::ios::binary);

    infile.seekg(0, infile.end);
    int jpgdatalength = infile.tellg();
    infile.seekg(0, infile.beg);
    std::cout << "encoded jpeg data length is " << jpgdatalength<<"\n";

    byte *decoded = new byte[jpgdatalength]; //decode会不断变换，拷贝一份到copyptr，然后删除
    byte *copyptr = decoded;
    infile.read((char*)decoded,jpgdatalength);  //读取编码的jpeg文件数据到内存中
    infile.close();

    int ret = readinfo(decoded, bgrdata, width, height);
    width = bmpwidth;
    height = bmpheight;
    if(copyptr != nullptr) {
        delete[] copyptr;
    }
    return ret;
}

int readSectionLength(byte *&jpegdata) {
    byte tmp;
    int length = 0;
    memcpy(&tmp, jpegdata++, 1);
    length = tmp;
    memcpy(&tmp, jpegdata++, 1);
    length = length*256 + tmp;
}

void readAPP0(byte *&jpegdata) {
    std::cout<<"\n************APP0 INFO*******************\n";
    int length = readSectionLength(jpegdata);
    std::cout<<"本数据段的长度为："<<length<<"\n";
    byte *appdata = new byte[length-2];  //length 包括前面的长度信息数据所占的2个字节
    memcpy(appdata, jpegdata, length-2);
    jpegdata = jpegdata + (length-2);
    //开始输出本段的信息
    std::cout<<"标识符：";
    std::cout<<std::setw(5)<<appdata<<";\t";
    std::cout<<"版本号：";
    std::cout<<static_cast<int>(appdata[5])<<"."<<static_cast<int>(appdata[6])<<"\n";
    std::cout<<"X和Y的密度单位: "<<static_cast<int>(appdata[7])<<"; ---注释：0：无单位；1：点数/英寸；2：点数/厘米\n";
    int x = appdata[8]*256 + appdata[9];
    int y = appdata[10]*256 + appdata[11];
    std::cout<<"X方向像素密度:"<<x<<";  Y方向像素密度:"<<y<<"\n";
    std::cout<<"缩略图水平像素数目:"<<static_cast<int>(appdata[12])<<"; 缩略图垂直像素数目:"<<static_cast<int>(appdata[13])<<"\n";
    if(length == 16) {
        std::cout<<"该jpeg文件没有缩略图RGB位图\n";
    } else {
        std::cout<<"该jpeg文件有缩略图RGB位图\n";
    }

    if(appdata != nullptr) {
        delete[] appdata;
    }
}

void readDQT(byte *&jpegdata) {
    std::cout<<"\n************DQT INFO*******************\n";
    int length = readSectionLength(jpegdata);
    std::cout<<"本数据段的长度为："<<length<<"\n";
    byte *dqtdata = new byte[length-2];  //length 包括前面的长度信息数据所占的2个字节
    memcpy(dqtdata, jpegdata, length-2);
    jpegdata = jpegdata + (length-2);
    int dataindex = 0;
    int Precision = ((dqtdata[dataindex]&0xF0)>>4);
    int tableid = (dqtdata[dataindex]&0x0F);
    dataindex++;

    if(Precision == 0) {
        std::cout<<"精度:8位,量化表ID:"<<tableid<<"\n";
        int count = 0;
        for(int i=dataindex;i<dataindex+64;i++) {
            //printf(" %2d",dqtdata[i]);
            std::cout<<" "<<std::setw(2)<<static_cast<int>(dqtdata[i]);
            quantTable[tableid][count] = dqtdata[i];
            count++;
            if(count%8==0) {
                std::cout<<"\n";
            }          
        }
    } else {
        std::cout<<"精度:16位,量化表ID:"<<tableid<<"\n";
        //待补充16位时候的情况
    }

    if(dqtdata != nullptr) {
        delete[] dqtdata;
    }
}

void readSOF0(byte *&jpegdata) {
    std::cout<<"\n************SOF0 INFO*******************\n";
    int length = readSectionLength(jpegdata);
    std::cout<<"本数据段的长度为："<<length<<"\n";
    byte *sofdata = new byte[length-2];  //length 包括前面的长度信息数据所占的2个字节
    memcpy(sofdata, jpegdata, length-2);
    jpegdata = jpegdata + (length-2);

    int Precision = sofdata[0];
    std::cout<<"图片数据精度："<<Precision<<"位精度数据\n";

    int heigh = sofdata[1]*256 + sofdata[2];
    int width = sofdata[3]*256 + sofdata[4];
    std::cout<<"图片高度:"<<heigh<<";图片宽度:"<<width<<"\n";
    image.width = width;
    image.height = heigh;
    int colors = sofdata[5];
    std::cout<<"图片的颜色格式为：";
    if(colors == 1) {
        std::cout<<"灰度图\n";
    } else if(colors == 3) {
        std::cout<<"YCrCb格式彩色图\n";
    } else if(colors == 4) {
        std::cout<<"CMYK格式彩色图\n";
    }  else {
        std::cout<<"解析图像格式错误\n";
    }

    //将每个颜色分量使用的量化表，水平和垂直采样因子存储起来
    int index = 6;
    for(int i=0;i<colors;i++) {
        byte colorid = sofdata[index++];
        subVector[colorid].id = colorid;
        byte tmp = sofdata[index++];
        subVector[colorid].width =  tmp>>4;
        subVector[colorid].height = tmp & 0x0F;
        subVector[colorid].quant = sofdata[index++];

        maxHeight = (maxHeight > subVector[colorid].height ? maxHeight : subVector[colorid].height);
        maxWidth = (maxWidth > subVector[colorid].width ? maxWidth : subVector[colorid].width);
    }

    for(int i=1;i<=colors;i++) {
        std::cout<<"颜色分量ID："<<static_cast<int>(subVector[i].id)<<"\n";
        std::cout<<"水平采样因子："<<static_cast<int>(subVector[i].width)<<"\n";
        std::cout<<"垂直采样因子："<<static_cast<int>(subVector[i].height)<<"\n";
        std::cout<<"量化表ID："<<static_cast<int>(subVector[i].quant)<<"\n";
    }

    if(sofdata != nullptr) {
        delete[] sofdata;
    }
}

void readCOM(byte *&jpegdata) {
    std::cout<<"\n************COM INFO*******************\n";
    int length = readSectionLength(jpegdata);
    std::cout<<"本数据段的长度为："<<length<<"\n";
    byte *comdata = new byte[length-2];  //length 包括前面的长度信息数据所占的2个字节
    memcpy(comdata, jpegdata, length-2);
    jpegdata = jpegdata + (length-2);

    for(int i=0; i<length-2; i++) {
        std::cout<<comdata[i];
    }
    std::cout<<"\n";
    if(comdata != nullptr) {
        delete[] comdata;
    }
}

void reaDHT(byte *&jpegdata) {
    std::cout<<"\n************DHT INFO*******************\n";
    int length = readSectionLength(jpegdata);
    std::cout<<"本数据段的长度为："<<length<<"\n";
    byte *dhtdata = new byte[length-2];  //length 包括前面的长度信息数据所占的2个字节
    memcpy(dhtdata, jpegdata, length-2);
    jpegdata = jpegdata + (length-2);

    int type = dhtdata[0]>>4;
    int tableid = dhtdata[0]&0x0F;
    if(type == 0) { 
        std::cout<<"DC哈夫曼表; "<<"Table ID:"<<tableid<<"\n";
    } else if (type ==1){
        std::cout<<"AC哈夫曼表; "<<"Table ID:"<<tableid<<"\n";
    } else {
        std::cout<<"读取哈夫曼表失败，数据不正确\n";
    }

    int huf[16] = {0};
    for(int i=0;i<16;i++) {
        std::cout<<static_cast<int>(dhtdata[i+1])<<" ";
        huf[i] = dhtdata[i+1];
    }
    std::cout<<"\n";

    if(type == 0 && tableid ==0) {   //Y分量的DC（直流）哈夫曼码表
        createhuftable(huf, dhtdata+17, length-2-17, YDCHT);   
    } else if(type == 0 && tableid ==1) {   //UV分量的DC（直流）哈夫曼码表,这两个分量公用相同的DC，AC哈夫曼表
        createhuftable(huf, dhtdata+17, length-2-17, UVDCHT);   
    } else if(type == 1 && tableid == 0) {   //Y分量的AC（交流）哈夫曼码表,
        createhuftable(huf, dhtdata+17, length-2-17, YACHT);   
    } else if(type == 1 && tableid == 1) {   //UV分量的AC（交流）哈夫曼码表,这两个分量公用相同的DC，AC哈夫曼表
        createhuftable(huf, dhtdata+17, length-2-17, UVACHT); 
    }

    if(dhtdata != nullptr) {
        delete[] dhtdata;
    }
}

void readSOS(byte *&jpegdata) {
    std::cout<<"\n************SOS INFO*******************\n";
    int length = readSectionLength(jpegdata);
    std::cout<<"本数据段的长度为："<<length<<"\n";
    byte *sosdata = new byte[length-2];  //length 包括前面的长度信息数据所占的2个字节
    memcpy(sosdata, jpegdata, length-2);
    jpegdata = jpegdata + (length-2);

    int colors = sosdata[0];
    std::cout<<"图片的颜色格式为：";
    if(colors == 1) {
        std::cout<<"灰度图\n";
    } else if(colors == 3) {
        std::cout<<"YCrCb格式彩色图\n";
    } else if(colors == 4) {
        std::cout<<"CMYK格式彩色图\n";
    }  else {
        std::cout<<"解析图像格式错误\n";
    }
    for(int i=0;i<colors;i++) {
        std::cout<<"颜色分量ID:"<<static_cast<int>(sosdata[i*2 +1])<<"\n";
        byte tmp = sosdata[i*2 +2];
        int DCnum = tmp>>4;
        int ACnum = tmp&0x0F;
        std::cout<<"直流分量使用的哈夫曼表ID:"<<DCnum<<"\n";
        std::cout<<"交流分量使用的哈夫曼表ID:"<<DCnum<<"\n";
    }
    printf("谱选择开始:0x%02X\n",sosdata[7]);
    printf("谱选择结束:0x%02X\n",sosdata[8]);
    printf("谱选择:0x%02X\n",sosdata[9]);
    if(sosdata != nullptr) {
        delete[] sosdata;
    }
}

//建立哈夫曼表
//tree[16] 是哈夫曼树每个高度的叶子节点数量，value是权值的指针，length是值的长度
void createhuftable(int tree[16], byte *value, int length, std::vector<hufcode> &OUTT) {
    int comlength = 0;
    for (int i = 0; i<16; i++) {
        comlength += tree[i];
    }
    if (comlength != length) {
        std::cout << "data error, length not match\n";
    }
    int left = 0;
    int right = 0;
    int index = 0;
    int last_depth = 0;

    for (int i = 0; i<16; i++) {
        int nums = tree[i];
        if (index != 0) {
            int mul = pow(2, i + 1 - last_depth);
            left = (right + 1)*mul;
        }
        else {
            left = 0;
        }
        if (nums != 0) {
            last_depth = i + 1;
            int cur_value = left;
            for (int j = 0; j<nums; j++) {
                hufcode unit;
                unit.codeLength = i+1;
                unit.encodeValue = cur_value++;
                unit.realVale = value[index++];
                OUTT.push_back(unit);
            }
            right = cur_value - 1;
        }
    }
}

bool getBit(byte *&framedata) {
    static unsigned char buf;
    static unsigned char count = 0;
    if (count == 0) {
        memcpy(&buf, framedata++, 1);
        if (buf == 0xFF) {
            unsigned char check;
            memcpy(&check, framedata++, 1);
            if (check != 0x00) {
                fprintf(stderr, "data 段有不是 0xFF00 的数据\n");
            }
        }
    }
    bool ret = buf & (1 << (7 - count));
    count = (count == 7 ? 0 : count + 1);
    return ret;
}

int readDC(byte *&framedata, unsigned char number) {
    unsigned int len = 0;
    unsigned char codeLen;
    for (int count = 1; ; count++) {
        len = len << 1;
        len += (unsigned int)getBit(framedata);
        if(number == 0) {
            for(int v=0;v<YDCHT.size();v++) {
                if((YDCHT[v].codeLength==count) && (YDCHT[v].encodeValue==len)) {
                    codeLen = YDCHT[v].realVale;
                    if (codeLen == 0) { return 0; }
                    unsigned char first = getBit(framedata);
                    int ret = 1;
                    for (int i = 1; i < codeLen; i++) {
                        unsigned char b = getBit(framedata);
                        ret = ret << 1;
                        ret += first ? b : !b;
                    }
                    ret = first ? ret : -ret;
                    return ret;
                }
            }
            if (count > 16) {fprintf(stderr, "key not found\n"); count = 1; len = 0; }
        } else {
            for(int v=0;v<UVDCHT.size();v++) {
                if((UVDCHT[v].codeLength==count) && (UVDCHT[v].encodeValue==len)) {
                    codeLen = UVDCHT[v].realVale;
                    if (codeLen == 0) { return 0; }
                    unsigned char first = getBit(framedata);
                    int ret = 1;
                    for (int i = 1; i < codeLen; i++) {
                        unsigned char b = getBit(framedata);
                        ret = ret << 1;
                        ret += first ? b : !b;
                    }
                    ret = first ? ret : -ret;
                    return ret;
                }
            }
            if (count > 16) {fprintf(stderr, "key not found\n"); count = 1; len = 0; }
        }
        
    }
}

acCode readAC(byte *&framedata, unsigned char number) {
    unsigned char x;
    unsigned int len = 0;
    for (int count = 1; ; count++) {
        len = len << 1;
        len += (unsigned int)getBit(framedata);
        if(number == 0) {
            for(int v=0;v<YACHT.size();v++) {
                if((YACHT[v].codeLength==count) && (YACHT[v].encodeValue==len)) {
                    x = YACHT[v].realVale;
                    unsigned char zeros = x >> 4;
                    unsigned char codeLen = x & 0x0F;
                    if (x == 0) {
                        return acCode{0,0,0};
                    } else if (x == 0xF0) {
                        return acCode{0, 16, 0};
                    }
                    unsigned  char first = getBit(framedata);
                    int code = 1;
                    for (int i = 1; i < codeLen; i++) {
                        unsigned char b = getBit(framedata);
                        code = code << 1;
                        code += first ? b : !b;
                    }
                    code = first ? code : -code;
                    return acCode{codeLen, zeros, code};
                }
            }
            if (count > 16) {fprintf(stderr, "key not found\n"); count = 1; len = 0;}
        } else {
            for(int v=0;v<UVACHT.size();v++) {
                if((UVACHT[v].codeLength==count) && (UVACHT[v].encodeValue==len)) {
                    x = UVACHT[v].realVale;
                    unsigned char zeros = x >> 4;
                    unsigned char codeLen = x & 0x0F;
                    if (x == 0) {
                        return acCode{0,0,0};
                    } else if (x == 0xF0) {
                        return acCode{0, 16, 0};
                    }
                    unsigned  char first = getBit(framedata);
                    int code = 1;
                    
                    for (int i = 1; i < codeLen; i++) {
                        unsigned char b = getBit(framedata);
                        code = code << 1;
                        code += first ? b : !b;
                    }
                    code = first ? code : -code;
                    return acCode{codeLen, zeros, code};
                }
            }
            if (count > 16) {fprintf(stderr, "key not found\n"); count = 1; len = 0; }
        }
        
    }
}

MCU readMCU(byte *&framedata) {
    static int dc[4] = {0, 0, 0, 0};
    auto mcu = MCU();
    for (int i = 1; i <= 3; i++) {
        for (int h = 0; h < subVector[i].height; h++) {
            for (int w = 0; w < subVector[i].width; w++) {              
                dc[i] = readDC(framedata, i/2) + dc[i];
                mcu.mcu[i][h][w][0][0] = dc[i];
                unsigned int count = 1;
                while (count < 64) {   //8*8个数据
                    acCode ac = readAC(framedata, i/2);
                    if (ac.len == 0 && ac.zeros == 16) {
                        for (int j = 0; j < ac.zeros; j++) {
                            mcu.mcu[i][h][w][count/8][count%8] = 0;
                            count++;
                        }
                    } else if (ac.len == 0) { //如果读到0，提前结束
                        break;
                    } else {                //补足对应的0的个数
                        for (int j = 0; j < ac.zeros; j++) {
                            mcu.mcu[i][h][w][count/8][count%8] = 0;
                            count++;
                        }
                        mcu.mcu[i][h][w][count/8][count%8] = ac.value;
                        count++;
                    }
                }
                while (count < 64) {  //补足对应的0的个数，补足8*8个位置
                    mcu.mcu[i][h][w][count/8][count%8] = 0;
                    count++;
                }
            }
        }
    } //end for
    return mcu;
}

void decodeMCU(byte *&framedata, byte *&bgrdata) {
    int w = (image.width - 1) / (8*maxWidth) + 1; //水平mcu的个数
    int h = (image.height - 1) / (8*maxHeight) + 1; //垂直mcu的个数

    bmpwidth = image.width;
    bmpheight = image.height;
    bgrdata = new byte[bmpwidth*bmpheight*3];
    int bytes_per_pixel = 3;
    int bytes_per_row = bmpwidth*3;
    
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            MCU mcu = readMCU(framedata);
            mcu.decode();
            RGB **b = mcu.toRGB();
            for (int y = i*8*maxHeight; y < (i+1)*8*maxHeight; y++) {  //整个图片上的垂直坐标
                for (int x = j*8*maxWidth; x < (j+1)*8*maxWidth; x++) {  //整个图片上的水平坐标
                    int by = y - i*8*maxHeight;
                    int bx = x - j*8*maxWidth;
                    if(x < 0 || x >= bmpwidth || y < 0 || y >= bmpheight) {
                        continue;
                    } else {
                        byte *pixel;
                        pixel = bgrdata + ( y * bytes_per_row + x * bytes_per_pixel );
                        *( pixel + 2 ) = b[by][bx].R;
                        *( pixel + 1 ) = b[by][bx].G;
                        *( pixel + 0 ) = b[by][bx].B;
                    }
                }
            } // end for 

            //释放每个MCU转换RBG数据的暂存内存
            for (int i = 0; i < maxHeight*8; i++) { 
                if(b[i] != nullptr) {
                     delete[] b[i];
                }
            } 
            if(b != nullptr) {
                delete[] b;
            }
            
        }
    }
}
