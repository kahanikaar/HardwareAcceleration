#include<stdio.h>
#include<stdlib.h>

#include<errno.h>
#include<libavcodec/avcodec.h>
#include<libavutil/hwcontext.h>

static int width, height;
static AVBuffer *hardware_device_context=NULL;  //for referencing to data buffer for hardware

static int setHardwareframeContext(AVCodecContext*, AVBufferRef*);
static int encode_func(AVCodecContext*, AVFrame*, FILE*);

int main(int argc, char *argv[])  // command line arguments for accepting source of width, height, source of input file to be encoded, source of output file after encoding, and encoder name respectively
{
	int size;      // for storing screen size to be found as the product of height and width
	int info; // for storing info regarding data that functions return
	FILE *in=NULL; // for address of input file
	FILE *out= NULL ; //for address of output file
	AVFrame *software_frame= NULL ; //for referencing software frame data
	AVFrame *hardware_frame=NULL; //for referencing hardware frame data
	AVCodecContext *AVCntx = NULL; //for referencing main api structure
	AVCodec *codec = NULL; //for referencing codec info

	if(argc<6){
	printf("\n\n Error! Incomplete set of arguments for encoding process!");
	return -1;
	}

	width=atoi(argv[1]);
	height=atoi(argv[2]);
	size=width*height;
	const char *encoderName=argv[5];

	if(!(in=fopen(argv[3],"r"))){
	printf("\n\nError! Fail to open input file!");
	return -1;
	}

	if(!(out=fopen(argv[4],"w+b"))){
	printf("\n\nError! Fail to open output file!");
	return -1;
	}

	info=av_hwdevice_ctx_create(&hardware_device_context, AV_HWDEVICE_TYPE_VAAPI,NULL,NULL,0);  //for for creating hardware device context data according to the VAAPI

	if(info<0){
	printf("\n\n Failed to create a VAAPI device! error code: %s",av_err2str(info));
	}

	if(!(codec=avcodec_find_encoder_by_name(encoderName)))   //for checking the availability of particular encoder
	{
	printf("Error! Could not find encoder!");
	info=-1;
	}

	
	if(!(AVCntx=avcodec_alloc_context3(codec)))  //for loading reference data of codec to AVContext
	{ 
	info=AVERROR(ENOMEM);
	}

//setting the data for the AVContext
	AVCntx->width=width;
	AVCntx->height=height;
	AVCntx->time_base=(AVRational){1,25};
	AVCntx->framerate=(AVRational){25,1};
	AVCntx->sample_aspect_ratio=(AVRational){1,1};
	AVCntx->pix_fmt=AV_PIX_FMT_VAAPI;


if((info=setHardwareframeContext(AVCntx,hardware_device_context))<0)
{
	printf("Failed to set hardware frame context./n/n");
}

if((info=avcodec_open2(AVCntx,codec,NULL))<0){
	printf("/n/nCannot open video encoder codec. Error code : %s\n",av_err2str(info));
}

while(1){

	if(!(software_frame=av_frame_alloc())){ //for allocating frame reference to software frame
	info=AVERROR(ENOMEM);
	}

//reading data from software framee, and transferring them into hw frame

software_frame->width=width;
software_frame->height=height;
software_frame->format=AV_PIX_FMT_NV12;

if((info=av_frame_get_buffer(software_frame,32))<0)
 if((info=fread((uint8_t*)(software_frame
->data[0]),size,1,in))<=0)
   break;
 if((info=fread((uint8_t*)(software_frame
->data[1]),size/2,1,in))<=0)
     break;

 if(!(hardware_frame=av_frame_alloc())){
 info=AVERROR(ENOMEM);
 }

 if((info=av_hwframe_get_buffer(AVCntx->hw_frames_ctx,hardware_frame,0))<0){
 printf("Error! Code: %s \n",av_err2str(info));
 }

 if(!hardware_frame->hw_frames_ctx){
 info=AVERROR(ENOMEM);
 }

 if((info=(encode_func(AVCntx,hardware_frame,out)))<0){
 printf("Failed to encode!\n");
 }

 av_frame_free(&hardware_frame);
 av_frame_free(&software_frame);


}

info=encode_func(AVCntx,NULL,out);
if(info==AVERROR_EOF)
info=0;

return info;





}

static int setHardwareframeContext(AVCodecContext *ctx, AVBufferRef *hw_device_ctx){
	 AVBufferRef *hw_frames_ref;
    AVHWFramesContext *frames_ctx = NULL;
    int err = 0;

    if (!(hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx))) {
        fprintf(stderr, "Failed to create VAAPI frame context.\n");
        return -1;
    }
    frames_ctx = (AVHWFramesContext *)(hw_frames_ref->data);
    frames_ctx->format    = AV_PIX_FMT_VAAPI;
    frames_ctx->sw_format = AV_PIX_FMT_NV12;
    frames_ctx->width     = width;
    frames_ctx->height    = height;
    frames_ctx->initial_pool_size = 20;
    if ((err = av_hwframe_ctx_init(hw_frames_ref)) < 0) {
        fprintf(stderr, "Failed to initialize VAAPI frame context."
                "Error code: %s\n",av_err2str(err));
        av_buffer_unref(&hw_frames_ref);
        return err;
    }
    ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
    if (!ctx->hw_frames_ctx)
        err = AVERROR(ENOMEM);

    av_buffer_unref(&hw_frames_ref);
    return err;
}

static int encode_func(AVCodecContext *avctx, AVFrame *frame, FILE *fout)
{
    int ret = 0;
    AVPacket enc_pkt;

    av_init_packet(&enc_pkt);
    enc_pkt.data = NULL;
    enc_pkt.size = 0;

    if ((ret = avcodec_send_frame(avctx, frame)) < 0) {
        fprintf(stderr, "Error code: %s\n", av_err2str(ret));
        goto end;
    }
    while (1) {
        ret = avcodec_receive_packet(avctx, &enc_pkt);
        if (ret)
            break;

        enc_pkt.stream_index = 0;
        ret = fwrite(enc_pkt.data, enc_pkt.size, 1, fout);
        av_packet_unref(&enc_pkt);
    }

end:
    ret = ((ret == AVERROR(EAGAIN)) ? 0 : -1);
    return ret;
}

