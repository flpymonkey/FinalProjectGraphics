#include "jpegio.h"
#include <vector>
#include <stdio.h>
#ifdef __linux__
  #include "jpeglib.h"
#endif

//FIXME: Currently this only works on linux build, need to find alternative for Windows

bool SaveJPEG(const std::string& filename,
              int image_width,
              int image_height,
              const unsigned char* pixels)
{
  #ifdef __linux__
  	struct jpeg_compress_struct cinfo;
  	struct jpeg_error_mgr jerr;
  	FILE* outfile;
  	JSAMPROW row_pointer[1];
  	int row_stride;

  	cinfo.err = jpeg_std_error(&jerr);
  	jpeg_create_compress(&cinfo);

  	outfile = fopen(filename.c_str(), "wb");
  	if (outfile == NULL)
  		return false;

  	jpeg_stdio_dest(&cinfo, outfile);

  	cinfo.image_width = image_width;
  	cinfo.image_height = image_height;
  	cinfo.input_components = 3;
  	cinfo.in_color_space = JCS_RGB;
  	jpeg_set_defaults(&cinfo);
  	jpeg_set_quality(&cinfo, 100, true);
  	jpeg_start_compress(&cinfo, true);

  	row_stride = image_width * 3;

  	while (cinfo.next_scanline < cinfo.image_height) {
  		row_pointer[0] = const_cast<unsigned char*>(
  				&pixels[(cinfo.image_height - 1 - cinfo.next_scanline) * row_stride]);
  		jpeg_write_scanlines(&cinfo, row_pointer, 1);
  	}

  	jpeg_finish_compress(&cinfo);
  	fclose(outfile);

  	jpeg_destroy_compress(&cinfo);
  	return true;
  #else
    return false;
  #endif
}
