#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <jpeglib.h>
#include "bytes.h"

/* ARGB颜色结构体 */
typedef struct argbcolor{
	int Alpha;
	int Red;
	int Green;
	int Blue;
} ARGB;

/* YUV颜色结构体 */
typedef struct yuvcolor{
	int Y;
	int U;
	int V;
} YUV;

/* 读取bmp文件头 */
void readbh(BYTE *BMPFILEHEADER,BYTE *BMPINFOHEADER,FILE *BMPFILE){
	fseek(BMPFILE,0,SEEK_SET);
	fread(BMPFILEHEADER,sizeof(BYTE)*14,1,BMPFILE);
	fread(BMPINFOHEADER,sizeof(BYTE)*40,1,BMPFILE);
	// return 0;
}

/* 解析得到所需数据 */
void parsebh(int *filesize,int *bfoffset,int *width,int *height,const BYTE *BMPFILEHEADER,const BYTE *BMPINFOHEADER){
	BYTE bsize[4],boffset[4],bwidth[4],bheight[4];
	bytescut(bsize,BMPFILEHEADER,14,2,4);
	bytescut(boffset,BMPFILEHEADER,14,10,4);
	bytescut(bwidth,BMPINFOHEADER,40,4,4);
	bytescut(bheight,BMPINFOHEADER,40,8,4);
	// printf("0x%02X 0x%02X 0x%02X 0x%02X\n",bheight[0],bheight[1],bheight[2],bheight[3]);
	*filesize = bytes2int(bsize,4,0);
	*bfoffset = bytes2int(boffset,4,0);
	*width = bytes2int(bwidth,4,0);
	*height = bytes2int(bheight,4,0);
}

/* 读取调色板 */
void readcp(ARGB *COLORPALLETTE,const int colornumber,FILE *BMPFILE){
	BYTE rawcolors[colornumber * 4];
	int idx;
	fseek(BMPFILE,54,SEEK_SET);
	fread(rawcolors,sizeof(BYTE) * 4 * colornumber,1,BMPFILE);
	// printf("0x%02X 0x%02X 0x%02X 0x%02X\n",rawcolors[4],rawcolors[5],rawcolors[6],rawcolors[7]);
	for(idx=0;idx<colornumber;idx++){
		COLORPALLETTE[idx].Blue = rawcolors[4*idx+0];
		COLORPALLETTE[idx].Green = rawcolors[4*idx+1];
		COLORPALLETTE[idx].Red = rawcolors[4*idx+2];
		COLORPALLETTE[idx].Alpha = rawcolors[4*idx+3];
	}
}

/* 读取像素点RGB数据 - 24位位图,直接读取BGR数据 */
void readpixels_24bits(ARGB *pixels,const int bfoffset,const long width,const long height,FILE *BMPFILE){
	BYTE *rawcolors;
	ARGB tmp1;
	int bpl,len;
	long iw,ih;
	if((width *3)%4){
		bpl = (width * 3 / 4) * 4 + 4;
	}else{
		bpl = (width * 3 / 4) * 4;
	}
	len = height * bpl;
	rawcolors = (BYTE *)malloc(sizeof(BYTE) * len);
	fseek(BMPFILE,bfoffset,SEEK_SET);
	fread(rawcolors,sizeof(BYTE) * len,1,BMPFILE);
	for(ih=height-1;ih>=0;ih--){
		for(iw=0;iw<width;iw++){
			tmp1.Blue = rawcolors[ih*bpl+3*iw+0];
			tmp1.Green = rawcolors[ih*bpl+3*iw+1];
			tmp1.Red = rawcolors[ih*bpl+3*iw+2];
			tmp1.Alpha = 0;
			pixels[(height-1-ih)*width+iw] = tmp1;
		}
	}
	free(rawcolors);
}

/* 读取像素点RGB数据 - 256色位图,需要从调色板读取数据 */
void readpixels_256colors(ARGB *pixels,const int bfoffset,const long width,const long height,ARGB *COLORPALLETTE,FILE *BMPFILE){
	BYTE *rawcolorindexs;
	int bpl,len,iw,ih;
	if(width%4){
		bpl = width / 4 * 4 + 4;
	}else{
		bpl = width;
	}
	len = height * bpl;
	rawcolorindexs = (BYTE *)malloc(sizeof(BYTE) * len);
	fseek(BMPFILE,bfoffset,SEEK_SET);
	fread(rawcolorindexs,sizeof(BYTE) * len,1,BMPFILE);
	for(ih=height-1;ih>=0;ih--){
		for(iw=0;iw<width;iw++){
			pixels[(height-1-ih)*width+iw] = COLORPALLETTE[(int)rawcolorindexs[ih*bpl+iw]];
		}
	}
	free(rawcolorindexs);
}

/* 读取像素点RGB数据 - 16色位图,需要从调色板读取数据 */
void readpixels_16colors(ARGB *pixels,const int bfoffset,const long width,const long height,ARGB *COLORPALLETTE,FILE *BMPFILE){
	BYTE *rawcolorindexs;
	BYTE tmp1;
	int bpl,len,iw,ih,idx;
	if(width%8){
		bpl = width / 8 * 4 + 4;
	}else{
		bpl = width / 2;
	}
	len = height * bpl;
	rawcolorindexs = (BYTE *)malloc(sizeof(BYTE) * len);
	fseek(BMPFILE,bfoffset,SEEK_SET);
	fread(rawcolorindexs,sizeof(BYTE) * len,1,BMPFILE);
	for(ih=height-1;ih>=0;ih--){
		for(iw=0;iw<width;iw++){
			tmp1 = rawcolorindexs[ih*bpl+iw/2];
			idx = (int)((iw%2)*(tmp1&0xF0>>4))+(int)((!(iw%2))*(tmp1&0x0F));
			// printf("--debug 0x%02X-%d-0x%02X-0x%02X-%d\n",tmp1,iw%2,(tmp1&0xF0>>4),(tmp1&0x0F),idx);
			pixels[(height-1-ih)*width+iw] = COLORPALLETTE[idx];
		}
	}
	free(rawcolorindexs);
}

/* 读取yuv像素点数据 */
void readyuvpixels(YUV *pixels,const BYTE *yuvrawdata,const int iwidth,const int iheight,const int encodetype,const int savetype){
	int idx,ih,iw;
	switch(encodetype){
		case 444:
			switch(savetype){
				case 1:
					/* yvu 格式存储 */
					for(idx=0;idx<iwidth*iheight;idx++){
						pixels[idx].Y = yuvrawdata[3*idx+0];
						pixels[idx].U = yuvrawdata[3*idx+2];
						pixels[idx].V = yuvrawdata[3*idx+1];
					}
					break;
				case 2:
					/* yuv 格式Plane存储 */
					for(idx=0;idx<iwidth*iheight;idx++){
						pixels[idx].Y = yuvrawdata[0*iwidth*iheight+idx];
						pixels[idx].U = yuvrawdata[1*iwidth*iheight+idx];
						pixels[idx].V = yuvrawdata[2*iwidth*iheight+idx];
					}
					break;
				case 0:
				default:
					/* yuv 格式存储 */
					for(idx=0;idx<iwidth*iheight;idx++){
						pixels[idx].Y = yuvrawdata[3*idx+0];
						pixels[idx].U = yuvrawdata[3*idx+1];
						pixels[idx].V = yuvrawdata[3*idx+2];
						// printf("%d %d %d\n",pixels[idx].Y,pixels[idx].U,pixels[idx].V);
					}
					break;
			}
			break;
		case 422:
			switch(savetype){
				case 1:
					/* uyvy 格式存储 */
					for(idx=0;idx<iwidth*iheight;idx+=2){
						pixels[idx].Y = yuvrawdata[2*idx+1];
						pixels[idx].U = yuvrawdata[2*idx+0];
						pixels[idx].V = yuvrawdata[2*idx+2];
						pixels[idx+1].Y = yuvrawdata[2*idx+3];
						pixels[idx+1].U = yuvrawdata[2*idx+0];
						pixels[idx+1].V = yuvrawdata[2*idx+2];
					}
					break;
				case 2:
					/* yvyu 格式存储 */
					for(idx=0;idx<iwidth*iheight;idx+=2){
						pixels[idx].Y = yuvrawdata[2*idx+0];
						pixels[idx].U = yuvrawdata[2*idx+3];
						pixels[idx].V = yuvrawdata[2*idx+1];
						pixels[idx+1].Y = yuvrawdata[2*idx+2];
						pixels[idx+1].U = yuvrawdata[2*idx+3];
						pixels[idx+1].V = yuvrawdata[2*idx+1];
					}
					break;
				case 3:
					/* vyuy 格式存储 */
					for(idx=0;idx<iwidth*iheight;idx+=2){
						pixels[idx].Y = yuvrawdata[2*idx+1];
						pixels[idx].U = yuvrawdata[2*idx+2];
						pixels[idx].V = yuvrawdata[2*idx+0];
						pixels[idx+1].Y = yuvrawdata[2*idx+3];
						pixels[idx+1].U = yuvrawdata[2*idx+2];
						pixels[idx+1].V = yuvrawdata[2*idx+0];
					}
					break;
				case 0:
				default:
					/* yuyv 格式存储 */
					for(idx=0;idx<iwidth*iheight;idx+=2){
						pixels[idx].Y = yuvrawdata[2*idx+0];
						pixels[idx].U = yuvrawdata[2*idx+1];
						pixels[idx].V = yuvrawdata[2*idx+3];
						pixels[idx+1].Y = yuvrawdata[2*idx+2];
						pixels[idx+1].U = yuvrawdata[2*idx+1];
						pixels[idx+1].V = yuvrawdata[2*idx+3];
					}
					break;
			}
			break;
		case 420:
			switch(savetype){
				case 1:
					/* YV12,YU12 格式存储 */
					for(ih=0;ih<iheight;ih++){
						for(iw=0;iw<iwidth;iw++){
							pixels[ih*iwidth+iw].Y = yuvrawdata[ih*iwidth+iw];
							if(!(ih%2) && !(iw%2)){
								pixels[ih*iwidth+iw].U = yuvrawdata[5*iwidth*iheight/4+(ih/2*iwidth+iw)/2];
								pixels[ih*iwidth+iw].V = yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)/2];
								
								pixels[ih*iwidth+iw+1].U = yuvrawdata[5*iwidth*iheight/4+(ih/2*iwidth+iw)/2];
								pixels[ih*iwidth+iw+1].V = yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)/2];
								
								pixels[(ih+1)*iwidth+iw].U = yuvrawdata[5*iwidth*iheight/4+(ih/2*iwidth+iw)/2];
								pixels[(ih+1)*iwidth+iw].V = yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)/2];
								
								pixels[(ih+1)*iwidth+iw+1].U = yuvrawdata[5*iwidth*iheight/4+(ih/2*iwidth+iw)/2];
								pixels[(ih+1)*iwidth+iw+1].V = yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)/2];
							}
						}
					}
					break;
				case 2:
					/* NV12,NV21 格式存储 */
					for(ih=0;ih<iheight;ih++){
						for(iw=0;iw<iwidth;iw++){
							pixels[ih*iwidth+iw].Y = yuvrawdata[ih*iwidth+iw];
							if(!(ih%2) && !(iw%2)){
								pixels[ih*iwidth+iw].U = yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)];
								pixels[ih*iwidth+iw].V = yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)+1];
								
								pixels[ih*iwidth+iw+1].U = yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)];
								pixels[ih*iwidth+iw+1].V = yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)+1];
								
								pixels[(ih+1)*iwidth+iw].U = yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)];
								pixels[(ih+1)*iwidth+iw].V = yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)+1];
								
								pixels[(ih+1)*iwidth+iw+1].U = yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)];
								pixels[(ih+1)*iwidth+iw+1].V = yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)+1];

							}
						}
					}
					break;
				case 0:
				default:
					/* I420格式存储 */
					for(ih=0;ih<iheight;ih++){
						for(iw=0;iw<iwidth;iw++){
							pixels[ih*iwidth+iw].Y = yuvrawdata[ih*iwidth+iw];
							if(!(ih%2) && !(iw%2)){
								pixels[ih*iwidth+iw].U = yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)/2];
								pixels[ih*iwidth+iw].V = yuvrawdata[5*iwidth*iheight/4+(ih/2*iwidth+iw)/2];
								
								pixels[ih*iwidth+iw+1].U = yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)/2];
								pixels[ih*iwidth+iw+1].V = yuvrawdata[5*iwidth*iheight/4+(ih/2*iwidth+iw)/2];
								
								pixels[(ih+1)*iwidth+iw].U = yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)/2];
								pixels[(ih+1)*iwidth+iw].V = yuvrawdata[5*iwidth*iheight/4+(ih/2*iwidth+iw)/2];
								
								pixels[(ih+1)*iwidth+iw+1].U = yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)/2];
								pixels[(ih+1)*iwidth+iw+1].V = yuvrawdata[5*iwidth*iheight/4+(ih/2*iwidth+iw)/2];
							}
						}
					}
					break;
			}
			break;
	}
}
		
/* RGBA结构体数组中拆分出单独的颜色数组 */
void rgbsplit(BYTE *red,BYTE *green,BYTE *blue,const ARGB *argb,size_t size){
	int idx;
	for(idx=0;idx<size;idx++){
		red[idx] = argb[idx].Red;
		green[idx] = argb[idx].Green;
		blue[idx] = argb[idx].Blue;
	}
}

/* YUV结构体数组中拆分出单独的颜色数组 */
void yuvsplit(BYTE *y,BYTE *u,BYTE *v,const YUV *yuv,size_t size){
	int idx;
	for(idx=0;idx<size;idx++){
		y[idx] = yuv[idx].Y;
		u[idx] = yuv[idx].U;
		v[idx] = yuv[idx].V;
	}
}

/* RGB颜色转为YUV颜色 */
void rgb2yuv(YUV *yuvpixel,ARGB *argbpixel,size_t size){
	int idx;
	for(idx=0;idx<size;idx++){
		/*
		yuvpixel[idx].Y = (int)(299*argbpixel[idx].Red +587*argbpixel[idx].Green + 114*argbpixel[idx].Blue)/1000;
		yuvpixel[idx].U = (int)(-168.7*argbpixel[idx].Red - 331.3*argbpixel[idx].Green + 500*argbpixel[idx].Blue)/1000 + 128;
		yuvpixel[idx].V = (int)(500*argbpixel[idx].Red - 418.7*argbpixel[idx].Green - 81.3*argbpixel[idx].Blue)/1000 + 128;
		*/
		yuvpixel[idx].Y = (int)(299*argbpixel[idx].Red +587*argbpixel[idx].Green + 114*argbpixel[idx].Blue)/1000;
		yuvpixel[idx].U = (int)(-147*argbpixel[idx].Red - 289*argbpixel[idx].Green + 436*argbpixel[idx].Blue)/1000 + 128;
		yuvpixel[idx].V = (int)(615*argbpixel[idx].Red - 515*argbpixel[idx].Green - 100*argbpixel[idx].Blue)/1000 + 128;
	}
}

/* YUV颜色转为RGB颜色 */
void yuv2rgb(ARGB *argbpixel,YUV *yuvpixel,size_t size){
	int idx;
	for(idx=0;idx<size;idx++){
		/*
		argbpixel[idx].Red = (int)(1000*yuvpixel[idx].Y + 1402*(yuvpixel[idx].V-128))/1000;
		argbpixel[idx].Green = (int)(1000*yuvpixel[idx].Y - 344.14*(yuvpixel[idx].U-128) - 714.14*(yuvpixel[idx].V-128))/1000;
		argbpixel[idx].Blue = (int)(1000*yuvpixel[idx].Y + 1772*(yuvpixel[idx].U-128))/1000;
		*/
		argbpixel[idx].Red = (int)(1000*yuvpixel[idx].Y + 1140*(yuvpixel[idx].V-128))/1000;
		argbpixel[idx].Green = (int)(1000*yuvpixel[idx].Y - 390*(yuvpixel[idx].U-128) - 580*(yuvpixel[idx].V-128))/1000;
		argbpixel[idx].Blue = (int)(1000*yuvpixel[idx].Y + 2030*(yuvpixel[idx].U-128))/1000;
	}
}

/* ------------------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------------------- */

/* RGB数据保存为JPEG文件 */
void save2jpeg(BYTE *rdata,BYTE *gdata,BYTE *bdata,long iwidth,long iheight,BYTE* filename){
	int depth = 3,ix;
	JSAMPROW row_pointer[1];
	struct jpeg_compress_struct jcinfo;
	struct jpeg_error_mgr jerror;
	FILE *jpgfp;
	BYTE irgb[iwidth * 3];
	jcinfo.err = jpeg_std_error(&jerror);
	jpeg_create_compress(&jcinfo);
	
	jpgfp = fopen(filename,"wb");
	if(NULL == jpgfp){
		printf("打开文件失败\n");
	}else{
		jpeg_stdio_dest(&jcinfo,jpgfp);
		jcinfo.image_width = iwidth;
		jcinfo.image_height = iheight;
		jcinfo.input_components = depth;
		jcinfo.in_color_space = JCS_RGB;
		
		jpeg_set_defaults(&jcinfo);
		jpeg_set_quality(&jcinfo,100,TRUE);
		jpeg_start_compress(&jcinfo, TRUE);
		int row_stride = iwidth * 3;
		while(jcinfo.next_scanline < jcinfo.image_height){
			for(ix=0;ix<iwidth;ix++){
				int idx = jcinfo.next_scanline * iwidth + ix;
				irgb[3*ix] = rdata[idx];
				irgb[3*ix+1] = gdata[idx];
				irgb[3*ix+2] = bdata[idx];
			}
			
			row_pointer[0] = &irgb[0];
			jpeg_write_scanlines(&jcinfo,row_pointer,1);
		}
		jpeg_finish_compress(&jcinfo);
		jpeg_destroy_compress(&jcinfo);//这几个函数都是固定流程
		fclose(jpgfp);
	}
}

/* YUV数据保存为RAW文件 */
void save2yuvraw(BYTE *ydata,BYTE *udata,BYTE *vdata,long iwidth,long iheight,BYTE* yfilename,BYTE* ufilename,BYTE* vfilename){
	FILE *yfp = fopen(yfilename,"wb");
	fwrite(ydata,iwidth * iheight,1,yfp);
	fclose(yfp);
	FILE *ufp = fopen(ufilename,"wb");
	fwrite(udata,iwidth * iheight,1,ufp);
	fclose(ufp);
	FILE *vfp = fopen(vfilename,"wb");
	fwrite(vdata,iwidth * iheight,1,vfp);
	fclose(vfp);
}

/* 保存为YUV文件 */
int save2yuv(BYTE *yuvrawdata,BYTE *ydata,BYTE *udata,BYTE *vdata,long iwidth,long iheight,const BYTE* filename,const int encodetype,const int savetype){
	int idx,iw,ih;
	long filelen;
	switch(encodetype){
		case 444:
			filelen = 3*iwidth*iheight;
			switch(savetype){
				case 1:
					/* yvu 格式存储 */
					for(idx=0;idx<iwidth*iheight;idx++){
						yuvrawdata[3*idx+0]=ydata[idx];
						yuvrawdata[3*idx+2]=udata[idx];
						yuvrawdata[3*idx+1]=vdata[idx];
					}
					break;
				case 2:
					/* yuv 格式Plane存储 */
					for(idx=0;idx<iwidth*iheight;idx++){
						yuvrawdata[0*iwidth*iheight+idx]=ydata[idx];
						yuvrawdata[1*iwidth*iheight+idx]=udata[idx];
						yuvrawdata[2*iwidth*iheight+idx]=vdata[idx];
					}
					break;
				case 0:
				default:
					/* yuv 格式存储 */
					for(idx=0;idx<iwidth*iheight;idx++){
						yuvrawdata[3*idx+0]=ydata[idx];
						yuvrawdata[3*idx+1]=udata[idx];
						yuvrawdata[3*idx+2]=vdata[idx];
					}
					break;
			}
			break;
		case 422:
			if(iwidth%2){
				printf("行像素为奇数,无法转码!\n");
				return -1;
			}
			filelen = 2*iwidth*iheight;
			switch(savetype){
				case 1:
					/* uyvy 格式存储 */
					for(idx=0;idx<iwidth*iheight;idx+=2){
						yuvrawdata[2*idx+0]=udata[idx];
						yuvrawdata[2*idx+1]=ydata[idx];
						yuvrawdata[2*idx+2]=vdata[idx+1];
						yuvrawdata[2*idx+3]=ydata[idx+1];
					}
					break;
				case 2:
					/* yvyu 格式存储 */
					for(idx=0;idx<iwidth*iheight;idx+=2){
						yuvrawdata[2*idx+0]=ydata[idx];
						yuvrawdata[2*idx+1]=vdata[idx];
						yuvrawdata[2*idx+2]=ydata[idx+1];
						yuvrawdata[2*idx+3]=udata[idx+1];
					}
					break;
				case 3:
					/* vyuy 格式存储 */
					for(idx=0;idx<iwidth*iheight;idx+=2){
						yuvrawdata[2*idx+0]=vdata[idx];
						yuvrawdata[2*idx+1]=ydata[idx];
						yuvrawdata[2*idx+2]=udata[idx+1];
						yuvrawdata[2*idx+3]=ydata[idx+1];
					}
					break;
				case 0:
				default:
					/* yuyv 格式存储 */
					for(idx=0;idx<iwidth*iheight;idx+=2){
						yuvrawdata[2*idx+0]=ydata[idx];
						yuvrawdata[2*idx+1]=udata[idx];
						yuvrawdata[2*idx+2]=ydata[idx+1];
						yuvrawdata[2*idx+3]=vdata[idx+1];
					}
					break;
			}
			break;
		case 420:
			if(iwidth%2){
				printf("行像素数为奇数,无法转码!\n");
				return -1;
			}else if(iheight%2){
				printf("列像素数为奇数,无法转码!\n");
				return -2;
			}
			filelen = 3*iwidth*iheight/2;
			switch(savetype){
				case 1:
					/* YV12,YU12 格式存储 */
					for(ih=0;ih<iheight;ih++){
						for(iw=0;iw<iwidth;iw++){
							yuvrawdata[ih*iwidth+iw]=ydata[ih*iwidth+iw];
							if(!(ih%2) && !(iw%2)){
								yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)/2]=vdata[ih*iwidth+iw];
								yuvrawdata[5*iwidth*iheight/4+(ih/2*iwidth+iw)/2]=udata[ih*iwidth+iw];
							}
						}
					}
					break;
				case 2:
					/* NV12,NV21 格式存储 */
					for(ih=0;ih<iheight;ih++){
						for(iw=0;iw<iwidth;iw++){
							yuvrawdata[ih*iwidth+iw]=ydata[ih*iwidth+iw];
							if(!(ih%2) && !(iw%2)){
								yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)]=udata[ih*iwidth+iw];
								yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)+1]=vdata[ih*iwidth+iw];
							}
						}
					}
					break;
				case 0:
				default:
					/* I420格式存储 */
					for(ih=0;ih<iheight;ih++){
						for(iw=0;iw<iwidth;iw++){
							yuvrawdata[ih*iwidth+iw]=ydata[ih*iwidth+iw];
							if(!(ih%2) && !(iw%2)){
								yuvrawdata[1*iwidth*iheight+(ih/2*iwidth+iw)/2]=udata[ih*iwidth+iw];
								yuvrawdata[5*iwidth*iheight/4+(ih/2*iwidth+iw)/2]=vdata[ih*iwidth+iw];
							}
						}
					}
					break;
			}
			break;
		default:
			printf("未知存储格式,请检查参数!\n");
			return -3;
	}
	FILE *yuvfp = fopen(filename,"wb");
	fwrite(yuvrawdata,filelen,1,yuvfp);
	fclose(yuvfp);
}

#endif
