//decoder for mpeg2 using VAAPI Core API

#include<stdio.h>
#include<stdlib.h>
#include<stddef.h>

#include <stddef.h>
#include <stdint.h>
#include "va.h"
#include "va_version.h"
#include "va_dec_hevc.h"
#include "va_dec_jpeg.h"
#include "va_dec_vp8.h"
#include "va_dec_vp9.h"
#include "va_enc_hevc.h"
#include "va_fei_hevc.h"
#include "va_enc_h264.h"
#include "va_enc_jpeg.h"
#include "va_enc_mpeg2.h"
#include "va_enc_vp8.h"
#include "va_enc_vp9.h"
#include "va_fei.h"
#include "va_fei_h264.h"
#include "va_vpp.h"
#include "va_backend.h"
#include "va_backend_vpp.h"
#include "va_compat.h"
#include "va_dec_av1.h"
#include "va_fei.h"
#include "va_fool.h"
#include "va_internal.h"
#include "va_str.h"
#include "va_x11.h"


#define NUMSURFACES 3

int main(int argc, char *argv[])
{
 // MPEG-2 decode for a 720x480 frame 

        int major_ver, minor_ver;
	Display *x11_display= XOpenDisplay(":0.0");
	VADisplay dpy=vaGetDisplay(x11_display);
        vaInitialize(dpy, &major_ver, &minor_ver);

        int max_num_profiles, max_num_entrypoints, max_num_attribs;
        max_num_profiles = vaMaxNumProfiles(dpy);
        max_num_entrypoints = vaMaxNumProfiles(dpy);
        max_num_attribs = vaMaxNumProfiles(dpy);

        //checking whether MPEG2 is supported
        VAProfile *profiles = malloc(sizeof(VAProfile)*max_num_profiles);
        int num_profiles;
        vaQueryConfigProfiles(dpy, profiles,&num_profiles);
       

        //getting the available entrypoints for MPEG2
        VAEntrypoint *entrypoints = malloc(sizeof(VAEntrypoint)*max_num_entrypoints);
        int num_entrypoints;
        VAProfile profile_mpg;

        vaQueryConfigEntrypoints(dpy, profile_mpg, entrypoints, &num_entrypoints);

        

      //finding the render target format
        VAConfigAttrib attrib;
         VAConfigID config_id;
VAEntrypoint entry;
int* num_attribs;
        vaQueryConfigAttributes(dpy, config_id, &profile_mpg,&entry , &attrib, num_attribs);

        
            

       
        vaCreateConfig(dpy, profile_mpg, entry, &attrib, 1,
                       &config_id);

        VASurfaceID surfaces[4];
        VASurfaceAttrib attrib_list;
        vaCreateSurfaces(dpy, VA_RT_FORMAT_YUV420,720, 480,surfaces , 4,&attrib_list,2 );

        VAContextID context;
        vaCreateContext(dpy, config_id, 720, 480, VA_PROGRESSIVE, surfaces,
                        4, &context);

        // Creating a picture parameter buffer for this frame 
        VABufferID picture_buf;
        VAPictureParameterBufferMPEG2 *picture_param;
        FILE *in;
        in=fopen(argv[1],"r");        
        uint8_t* info;
        fread((uint8_t*)info,720*480,1,in);
        vaCreateBuffer(dpy, context,VAPictureParameterBufferType,720*480,2,info,&picture_buf);
     vaBufferData(dpy, picture_buf, sizeof(VAPictureParameterBufferMPEG2), NULL);
     
        vaMapBuffer(dpy, picture_buf, (void**)&picture_param);
        picture_param->horizontal_size = 720;
        picture_param->vertical_size = 480;
        picture_param->picture_coding_type = 1; 
        vaUnmapBuffer(dpy, picture_buf);


        // Creating an IQ matrix buffer for this frame *
        VAIQMatrixBufferMPEG2 *iq_matrix;
        vaCreateBuffer(dpy,context, VAIQMatrixBufferType, 720*480,2,info,&iq_buf);
        vaBufferData(dpy, iq_buf, sizeof(VAIQMatrixBufferMPEG2), NULL);
        vaMapBuffer(dpy, iq_buf,(void**) &iq_matrix);

        vaUnmapBuffer(dpy, iq_buf);

        //sending picture and IQ matrix data to server
        vaBeginPicture(dpy, context, surfaces[0]);

        vaRenderPicture(dpy, context, &picture_buf, 1);
        vaRenderPicture(dpy, context, &iq_buf, 1);

        /* 
         * Send slices in this frame to the server.
         * For MPEG-2, each slice is one row of macroblocks, and
         * we have 30 slices for a 720x480 frame 
         */
        for (int i = 1; i <= 30; i++) {

                VABufferID slice_param_buf;
                VASliceParameterBufferMPEG2 *slice_param;
                vaCreateBuffer(dpy,context, VASliceParameterBufferType,720*480,2,info, &slice_param_buf);
                vaBufferData(dpy, slice_param_buf, sizeof(VASliceParameterBufferMPEG2), NULL);
                vaMapBuffer(dpy, slice_param_buf,(void**) &slice_param);
                slice_param->slice_data_offset = 0;

                slice_param->macroblock_offset = 64; 
                slice_param->slice_vertical_position = i;
                      vaUnmapBuffer(dpy, slice_param_buf);

              
                vaRenderPicture(dpy, context, &slice_param_buf, 1);

                unsigned char *slice_data;
                VABufferID slice_data_buf;
                vaCreateBuffer(dpy,context, VASliceDataBufferType,720*480,2,info, &slice_data_buf);
                vaBufferData(dpy, slice_data_buf, sizeof(VASliceDataBufferType), NULL);
                vaMapBuffer(dpy, slice_data_buf,(void**) &slice_data);
                // decoder fill in slice_data 
                vaUnmapBuffer(dpy, slice_data_buf);

                // send the slice data buffer 
                vaRenderPicture(dpy, context, &slice_data_buf, 1);
        }

        // all slices have been sent, mark the end for this frame 
        vaEndPicture(dpy, context);
return 0;
}
