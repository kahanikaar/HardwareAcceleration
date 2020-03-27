//Encoder using VAAPI Core API

#include<stdio.h>
#include<stdlib.h>
#include<stddef.h>

#include <stddef.h>
#include <stdint.h>
#include<va.h>
#include <va/va_version.h>
#include <va/va_dec_hevc.h>
#include <va/va_dec_jpeg.h>
#include <va/va_dec_vp8.h>
#include <va/va_dec_vp9.h>
#include <va/va_enc_hevc.h>
#include <va/va_fei_hevc.h>
#include <va/va_enc_h264.h>
#include <va/va_enc_jpeg.h>
#include <va/va_enc_mpeg2.h>
#include <va/va_enc_vp8.h>
#include <va/va_enc_vp9.h>
#include <va/va_fei.h>
#include <va/va_fei_h264.h>
#include <va/va_vpp.h>

#define NUMSURFACES 3;

int main()
{
    //Initialising VAAPI
	int major_ver, minor_ver;
	Display *x11_display=XOpenDisplay(":0.0");
	VADisplay display=vaGetDisplay(x11_display);
	VAStatus va_status =vaInitialize(display,&major_ver, &minor_ver);

    //Creating encoder configuration
    VAConfigAttrib attrib[2];
    attrib[0].type=VAConfigAttribRTFormat;
    attrib[0].value=VA_RT_FORMAT_YUV420;
    attrib[1].type=VAConfigAttribRateControl;
    attrib[1].value=VA_RC_VBR;
    VAConfigID config;
    va_status=vaCreateConfig(display,VAProfileH264Main,VAEntrypointEncSlice,attrib,2,&config);

    
     //creating source data surfaces
     VASurfaceID surfaces[NUMSURFACES];
     va_status=vaCreateSurfaces(display,frame_width,frame_height,VA_RT_FORMAT_YUV420,NUMSURFACES,surfaces);

     //creating encoding context
     VAContextID context;
     va_status=vaCreateContext(display,config, frame_width,frame_height,0,surfaces,NUMSURFACES,&context);

     //creating coded data buffer
     unsigned int codedbuf_size=(frame_width*frame_height*400)/256;
     va_status=vaCreateBuffer(display,context,VAEncCodedBufferType, codedbuf_size,1,NULL,&coded_buf);

     
     //accessing source surface data buffer
     VAImage image;
     unsigned char *pbuffer= NULL;
     vaDeriveImage(display,surface_id,&image); //get data layout
     vaMapBuffer(display,image.buf,&pbuffer);

     //beginning to encode
     while(1)
     {
     	vaBeginPicture(display, context,src_surface_id);

        //setting video sequence parameters
        if(current_frame==0){
        	VABufferID seq_buf_id;
        	VAEncSequenceParameterBufferH264 seq_h264={0};
        	seq_h264.seq_parameter_set_id=0;
        	seq_h264.level_idc=SH_LEVEL_3;
        	seq_h264.picture_width_in_mbs=frame_width/16;
        	seq_h264.picture_height_in_mbs=frame_height/16;
        	seq_h264.bits_per_second=14000000;
        	seq_h264.frame_rate=frame_rate;
        	seq_h264.initial_qp=24;
        	seq_h264.min_qp=1;
        	seq_h264.basic_unit_size=0;
        	seq_h264.intra_period=intra_count;
        	seq_h264.vui_flag=0;
        	vaCreateBuffer(display,context,VAEncSequenceParameterBufferType,sizeof(seq_h264),1,&seq_h264,&seq_buf_id);
        	vaRenderPicture(display,context,&sex_buf_id,1);
        }


        //setting picture parameters
        VAEncPictureParameteBufferH264 pic_h264;
        VASurfaceID src_surface_id,rec_surface_id,ref_surface_id;
        rec_surface_id=surfaces[NUMSURFACES-1];
        ref_surface_id=surfaces[NUMSURFACES-2];
        pic_h264.reference_picture=ref_surface;
        pic_h264.reconstructed_picture=rec_surface;
        pic_h264.coded_buf=coded_buf;
        pic_h264.picture_width=frame_width;
        pic_h264.picture_height=frame_height;
        pic_h264.last_picture=(current_frame==frame_count-1);
        vaCreateBuffer(va_dpy,context_id, VAEncPictureParameteBufferType,sizeof(pic_h264),1,&pic_h264,&pic_param_buf);
        vaRenderPicture(va_dpy,context_id,&pic_param_buf,1);

       //setting slice parameters
       VAENCSliceParameterBuffer slice_h264;
       slice_h264.start_row_number=0;
       slice_h264.slice_height=frame_height/16;
       slice_h264.slice_flags.bits.is_intra=((i%intra_count)==0);
       slice_h264.slice_flags.bits.disable_deblocking_filter_idc=0;
       vaCreateBuffer(va_dpy,context_id,VAENCSliceParameterBufferType,sizeof(slice_h264),1,&slice_h264,&slice_param_buf);
       vaRenderPicture(va_dpy,context_id,&slice_param_buf,1);

       //encoding process
       vaEndPicture(va_dpy, context_id);

       //on completion of encoding a frame
       vaSyncSurface(display,src_surface_id);
       vaQuerySurfaceStatus(va_dpy,src_surface_id,&surface_status);


       //saving coded buffer
       //accessing coded buffer data
       void *coded_p=NULL;
       vaMapBuffer(va_dpy,coded_buf_id,&coded_p);

       //getting coded buffer size, offset and video data pointer
       coded_size=*((unsigned long*)coded_p);
       coded_offset=*((unsigned long*)(coded_p+4));
       unsigned char* pdata=coded_p+coded_offset;

       //saving encoded data
       write(coded_fd,pdata,coded_size);
       vaUnmapBuffer(va_dpy,coded_buf);
     }
//releasing source data buffer
     vaUnmapBuffer(display,image.buf);
     vaDestroyImage(display,image.image_id);

     //deleting encoding context
     vaDestroySurfaces(display,surfaces,NUMSURFACES);
     vaDestroyConfig(display,config);
     vaDestroyConfig(display,context);

     //cleaning up the api
     vaTerminate(display);
     XCloseDisplay(x11_display);
	
	return 0;

}
