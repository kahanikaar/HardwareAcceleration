# MPEG2Decoder
Main file containing the code for the decoder : decode.c
## Compilation Process
   ### Creation of Object files
       gcc -c va.c
       gcc -c va_compat.c
       gcc -c va_fool.c
       gcc -c va_str.c
       gcc -c va_trace.c
       gcc -c decode.c
  This thus creates the necessary object files to be used to linked.
  
  ### Linking the created object files
       gcc -o a.out decode.o va.o va_compat.o va_fool.o va_str.o va_trace.o -lm -lavcodec -lexpat -lz -lswresample -lswscale -lavutil -ldl
  This creates the output file a.out by linking the necessary libraries and object files.     
      

## For running the program we have different modes
First argument after the program's name is the path of the video file and then follow the various modes to be used for the decoder to work

#### 1)  to decode a bitstream with double precision IDCT, such as to create a reference reconstruction set of frames:
        ./a.out test.m2v -r -f -o0 rec%d  
  
#### 2)  to decode a bitstream with fast integer IDCT, such as to create a test reconstruction set of frames:
         ./a.out test.m2v -f -o0 rec%d 
  
#### 3)  To substitute reference pictures with external reference pictures (ref%d):
          ./a.out test.m2v -f -x ref%d -o0 rec%d
          
#### 4)  Same as 3. only using a single large concatenated file for the sequence of reconstruction frames.
     ./a.out test.m2v -f -g -x ref%d  -o0 rec%d
     
#### 5)  Decode an SNR enhancement bitstream at the same time as base layer stream:
    ./a.out test.m2v -o0 rec%d -e snr_bitstream.m2v
   
#### 6)  Decode a Spatially scalable bitstream

   Step 1: create lower layer reconstruction

     ./a.out test.a2v -f -o0 llrec%d 

   Step 2: decode enhancement bitstream, combined reconstruction.

     mpeg2decode enhancement.a2v -f -l llrec%d  -o0 rec%d
  
    
  ------------
     where:
      -o0             specifies .Y, .U, .V input
      -f              specifies field interleaved format
      -g              specifies substitute file is a large concatendated one.
      -e              substitution flag
      test.m2v        bitstream file (main source file to be decoded)
      test%d          output file pattern
      ref.pic         filename of substitution sequence

          
### Running without specifying any modes or argument stating the file to be decoded will give the following output
      MPEG2 Decoder
    Usage:  ./a.out video_path {options}
    Options: -cn file  conformance report (n: level)
         -e  file  enhancement layer bitstream (SNR or Data Partitioning)
         -f        store/display interlaced video in frame format
         -g        concatenated file format for substitution method (-x)
         -in file  information & statistics report  (n: level)
         -l  file  file name pattern for lower layer sequence
                   (for spatial scalability)
         -on file  output format (0:YUV 1:SIF 2:TGA 3:PPM 4:X11 5:X11HiQ)
         -q        disable warnings to stderr
         -r        use double precision reference IDCT
         -t        enable low level tracing to stdout
         -u  file  print user_data to stdio or file
         -vn       verbose output (n: level)
         -x  file  filename pattern of picture substitution sequence

    File patterns:  for sequential filenames, "printf" style, e.g. rec%d
                 or rec%d%c for fieldwise storage
    Levels:        0:none 1:sequence 2:picture 3:slice 4:macroblock 5:block


         
