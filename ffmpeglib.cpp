#include "ffmpeglib.h"


FfmpegLib::FfmpegLib()
{
}


void FfmpegLib::init()
{
    static bool ran = false;
    if(!ran){
        av_register_all();
        avformat_network_init();
        ran = true;
    }
}

void FfmpegLib::clean()
{
    avformat_close_input(&pFormatCtx);
    avcodec_free_frame(&pFrame); // av_frame_free(&pFrame);
    avcodec_free_frame(&pFrameRGB); //av_frame_free(&pFrameRGB);
    av_free(buffer);
    av_free(rawBuffer);
}


int FfmpegLib::imgConvert(AVPicture * dst, AVPixelFormat dst_pix_fmt, const AVPicture * src,
                        AVPixelFormat src_pix_fmt, int src_width, int src_height, int dest_width, int dest_height)
{
    int w, h;
    struct SwsContext *pSwsCtx;
    w = src_width;
    h = src_height;
    pSwsCtx = sws_getContext(w, h, src_pix_fmt, dest_width, dest_height, dst_pix_fmt, SWS_AREA, NULL, NULL, NULL);	// SWS_BICUBIC
    sws_scale(pSwsCtx, src->data, src->linesize, 0, h, dst->data, dst->linesize);
    sws_freeContext(pSwsCtx);
    return 0;
}

int FfmpegLib::frameToMem(AVFrame * pFrame, int width, int height, unsigned char *buf_mem)
{
    int y;
    int count = 0;
    for (y = 0; y < height; y++) {
        memcpy((char *) &buf_mem[count], pFrame->data[0] + y * pFrame->linesize[0], width * 3);
        count += width * 3;
    }
    return count;
}

int FfmpegLib::setup(std::string& videofile)
{
    init();
    pFormatCtx = avformat_alloc_context();
    if(!pFormatCtx) return 0;

    if(avformat_open_input(&pFormatCtx, (const char*) videofile.c_str(), NULL, NULL) != 0){
        return 0;
    }

    videoStream = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);
    if (videoStream == -1 && pFormatCtx) {
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        return 0;
    }

    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (!pCodec && pFormatCtx) {
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        return 0;
    }

 if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0){
        if(pFormatCtx){
            avformat_close_input(&pFormatCtx);
            avformat_free_context(pFormatCtx);
            return 0;
        }
    }

    if (pCodecCtx->codec_id == CODEC_ID_H264)
        pCodecCtx->flags2 =   CODEC_FLAG2_DROP_FRAME_TIMECODE |  CODEC_FLAG2_CHUNKS;

    pFrame = avcodec_alloc_frame(); // pFrame = av_frame_alloc();
    pFrameRGB = avcodec_alloc_frame(); // pFrameRGB = av_frame_alloc();

    if(!(pFrame && pFrameRGB)){
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        avcodec_free_frame(&pFrame); // av_frame_free(&pFrame);
        avcodec_free_frame(&pFrameRGB); //av_frame_free(&pFrameRGB);
        return 0;
    }


    numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);

    buffer = (uint8_t *) av_malloc(numBytes * sizeof (uint8_t));
    rawBuffer = (unsigned char *) av_malloc(pCodecCtx->width*pCodecCtx->height*3);
    //TODO num components;

    if(!(buffer && rawBuffer)){

        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        avcodec_free_frame(&pFrame); // av_frame_free(&pFrame);
        avcodec_free_frame(&pFrameRGB); //av_frame_free(&pFrameRGB);
        av_free(buffer);
        av_free(rawBuffer);
        return 0;
    }

    avpicture_fill((AVPicture *) pFrameRGB, buffer, PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
    return 1;

}


int FfmpegLib::getWidth()
{
    return pCodecCtx->width;
}

int FfmpegLib::getHeight()
{
    return pCodecCtx->height;
}

cv::Mat FfmpegLib::getNextFrame()
{
    int type;
    return internalGetNextFrame(0, type);
}


cv::Mat FfmpegLib::getNextFrame(int& outType){
    return internalGetNextFrame(0, outType);

}

cv::Mat FfmpegLib::getNextIFrame()
{
    int type;
    return internalGetNextFrame(1, type);
}


#define IS_SKIP(a)       ((a)&MB_TYPE_SKIP)

cv::Mat FfmpegLib::internalGetNextFrame(int inType, int& outType) ///C
{

   // pCodecCtx->debug_mv = FF_DEBUG_MV;

    cv::Mat ret;
    while(1){
        if(av_read_frame(pFormatCtx, &packet) < 0){
            //std::cout << "no frame . . . " << std::endl;
            return ret;
        }

        if (packet.stream_index == videoStream){
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

            if(inType && pFrame->pict_type != inType) continue;
            AVPixelFormat pix_fmt = (pCodecCtx->pix_fmt == 12) ? AV_PIX_FMT_YUV420P : pCodecCtx->pix_fmt;
            //printf("key frame=%d \+++ type:%d\n", pFrame->key_frame,  pFrame->pict_type);
            if (frameFinished) {
                imgConvert((AVPicture *) pFrameRGB, AV_PIX_FMT_BGR24, (AVPicture *) pFrame, pix_fmt,
                             pCodecCtx->width, pCodecCtx->height, pCodecCtx->width, pCodecCtx->height);

                frameToMem(pFrameRGB, pCodecCtx->width, pCodecCtx->height, rawBuffer);
                ret = cv::Mat(pCodecCtx->height, pCodecCtx->width, CV_8UC3, rawBuffer);
                outType = pFrame->pict_type;
                return ret;
            }
        }
    }
}

AVFrame *FfmpegLib::getNextAVFrameI(){
    pCodecCtx->debug  |= FF_DEBUG_DCT_COEFF;

    while(1){
        if(av_read_frame(pFormatCtx, &packet) < 0){
            return NULL;
        }
        if (packet.stream_index == videoStream){
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
            if(pFrame->pict_type != AV_PICTURE_TYPE_I) continue;
            if(frameFinished){
                return pFrame;
            }
        }
    }
}


std::vector<short> FfmpegLib::getDCTvalues(AVFrame *frame){
    std::vector<short> ret;
    int mbsize = 0;
    while(frame->mb_type[mbsize++]) ;
    int len = 64 * mbsize * sizeof(short) * 6;
    //std::cout << "mbsize: " << mbsize << " len:" << len << std::endl;
    for(int i = 0; i < len; i++)
        ret.push_back(frame->dct_coeff[i]);
    return ret;
}

//TODO review when gop = N or N/2
int FfmpegLib::getGopSize(){
    int gop1=1, gop2=1;
    int iFramesCount = 0;
    bool exit = false;

    while(!exit){
        if(av_read_frame(pFormatCtx, &packet) < 0){
            av_seek_frame( 	pFormatCtx, videoStream, 0, AVSEEK_FLAG_ANY);
            return 0;
        }
        if (packet.stream_index == videoStream){
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
            if (!frameFinished) continue;
            if(pFrame->key_frame) {
                iFramesCount++;
                continue;
            }
            if(iFramesCount == 0) continue;
            else{
                (iFramesCount == 1) ? gop1++ : ((iFramesCount == 2) ? gop2++ : exit = true);
            }
        }
    }
    //TODO seek to first frame
    av_seek_frame( 	pFormatCtx, videoStream, 0, AVSEEK_FLAG_ANY);
    return (gop1 == gop2) ? gop1 : 0;
}








