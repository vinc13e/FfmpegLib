#ifndef FFMPEGLIB_H
#define FFMPEGLIB_H

#include "ffmpeglib_global.h"
#include <iostream>


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}


#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include "opencv2/highgui/highgui.hpp"



class FFMPEGLIBSHARED_EXPORT FfmpegLib
{
protected:
    AVFormatContext *pFormatCtx = NULL;
    int videoStream = -1;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame;
    AVFrame *pFrameRGB;
    int numBytes;
    uint8_t *buffer;
    AVPacket packet;
    AVDictionary *format_opts = NULL;
    int frameFinished;
    uint8_t *rawBuffer;
    int frameToMem(AVFrame * pFrame, int width, int height, unsigned char *buf_mem);
        cv::Mat internalGetNextFrame(int inType, int& outType);
    void init();

public:
    FfmpegLib();
    int imgConvert(AVPicture * dst, AVPixelFormat dst_pix_fmt, const AVPicture * src, AVPixelFormat src_pix_fmt, int src_width, int src_height, int dest_width, int dest_height);

    int setup(std::string&);
    cv::Mat getNextFrame();
    cv::Mat getNextFrame(int& type);
    cv::Mat getNextIFrame();
    cv::Mat getNextPFrame();
    int getGopSize();
    int getHeight();
    int getWidth();
    void clean();
    AVFrame *getNextAVFrameI();
    std::vector<short> getDCTvalues(AVFrame *frame);

};

#endif // FFMPEGLIB_H
