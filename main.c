#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include "image.h"

BYTE BMPFILENAME[128]={0};
BYTE JPGFILENAME[128]={0};
BYTE YUVFILENAME[128]={0};
BYTE YUV_YFILENAME[128]={0};
BYTE YUV_UFILENAME[128]={0};
BYTE YUV_VFILENAME[128]={0};
BYTE GRAYFILENAME[128]={0};

int main(int argc, char *argv[]){
	FILE *bmp=NULL;
	BYTE bfh[14],bih[40];
	int filesize,bfoffset,width,height;
	int idx;
	if(argc == 1){
		printf("请输入BMP文件名:\n");
		scanf("%s",BMPFILENAME);
	}else{
		bytescpy(BMPFILENAME,argv[1]);
	}
	bmp=fopen(BMPFILENAME,"rb");
	if(NULL == bmp){
		printf("文件 %s 打开失败!\n",BMPFILENAME);
		return -1;
	}
	readbh(bfh,bih,bmp);

	parsebh(&filesize,&bfoffset,&width,&height,bfh,bih);
	printf("size = %d bytes\noffset = 0x%02X bytes\nwidth = %d pixels\nheight = %d pixels\n",filesize,bfoffset,width,height);
	ARGB *rgbpixels_before  = (ARGB *)malloc(sizeof(ARGB) * height * width);
	YUV *yuvpixels_before  = (YUV *)malloc(sizeof(YUV) * height * width);
	ARGB *colorpalette;
	switch(bfoffset){
		case 54:
			printf("该图像为24位位图\n");
			readpixels_24bits(rgbpixels_before,bfoffset,width,height,bmp);
			break;
		case 1078:
			printf("该图像为256色位图\n");
			colorpalette = (ARGB *)malloc(sizeof(ARGB) * 256);
			readcp(colorpalette,256,bmp);
			readpixels_256colors(rgbpixels_before,bfoffset,width,height,colorpalette,bmp);
			break;
		case 118:
			printf("该图像为16色位图\n");
			colorpalette = (ARGB *)malloc(sizeof(ARGB) * 16);
			readcp(colorpalette,16,bmp);
			readpixels_16colors(rgbpixels_before,bfoffset,width,height,colorpalette,bmp);
			break;
		default:
			printf("未识别出图像类型\n");
			return -2;
	}
	//printf("0x%02X 0x%02X 0x%02X \n",pixels[0].Red,pixels[0].Green,pixels[0].Blue);
	
	filenamecreate(YUV_YFILENAME,BMPFILENAME,"y.raw");
	filenamecreate(YUV_UFILENAME,BMPFILENAME,"u.raw");
	filenamecreate(YUV_VFILENAME,BMPFILENAME,"v.raw");
	
	BYTE *cy = (BYTE *)malloc(sizeof(BYTE) * height * width);
	BYTE *cu = (BYTE *)malloc(sizeof(BYTE) * height * width);
	BYTE *cv = (BYTE *)malloc(sizeof(BYTE) * height * width);
	
	rgb2yuv(yuvpixels_before,rgbpixels_before,height*width);
	yuvsplit(cy,cu,cv,yuvpixels_before,height*width);
	save2yuvraw(cy,cu,cv,width,height,YUV_YFILENAME,YUV_UFILENAME,YUV_VFILENAME);
	filenamecreate(GRAYFILENAME,BMPFILENAME,"gray.jpg");
	printf("灰度文件名: %s\n",GRAYFILENAME);
	save2jpeg(cy,cy,cy,width,height,GRAYFILENAME);

	int YUVSAMPLINGTYPE = 444;
	int YUVSAVETYPE = 0;
	BYTE *yuvrawdata;
	if(argc>2){
		YUVSAMPLINGTYPE = atoi(argv[2]);
	}else{
		printf("请输入YUV采样格式\nYUV444:444\tYUV422:422\tYUV420:420\n>");
		scanf("%d",&YUVSAMPLINGTYPE);
	}
	switch(YUVSAMPLINGTYPE){
		case 444:
			yuvrawdata = (BYTE *)malloc(sizeof(BYTE) * height * width * 3);
			if(argc>3){
				YUVSAVETYPE = atoi(argv[3]);
			}else{
				printf("请输入YUV444保存格式\nYUV:0\tYVU:1\tYUV(Plane):2\n>");
				scanf("%d",&YUVSAVETYPE);
			}
			switch(YUVSAVETYPE){
				case 1:
					filenamecreate(YUVFILENAME,BMPFILENAME,"yuv444_yvu.yuv");
					break;
				case 2:
					filenamecreate(YUVFILENAME,BMPFILENAME,"yuv444_yuv_plane.yuv");
					break;
				case 0:
				default:
					filenamecreate(YUVFILENAME,BMPFILENAME,"yuv444_yuv.yuv");
					YUVSAVETYPE = 0;
					break;
			}
			break;
		case 422:
			yuvrawdata = (BYTE *)malloc(sizeof(BYTE) * height * width * 2);
			if(argc>3){
				YUVSAVETYPE = atoi(argv[3]);
			}else{
				printf("请输入YUV422保存格式\nYUYV:0\tUYVY:1\tYVYU:2\tVYUY:3\n>");
				scanf("%d",&YUVSAVETYPE);
			}
			switch(YUVSAVETYPE){
				case 1:
					filenamecreate(YUVFILENAME,BMPFILENAME,"yuv422_uyvy.yuv");
					break;
				case 2:
					filenamecreate(YUVFILENAME,BMPFILENAME,"yuv422_yvyu.yuv");
					break;
				case 3:
					filenamecreate(YUVFILENAME,BMPFILENAME,"yuv422_vyuy.yuv");
					break;
				case 0:
				default:
					filenamecreate(YUVFILENAME,BMPFILENAME,"yuv422_yuyv.yuv");
					YUVSAVETYPE = 0;
					break;
			}
			break;
		case 420:
			yuvrawdata = (BYTE *)malloc(sizeof(BYTE) * height * width * 3 / 2);
			if(argc>3){
				YUVSAVETYPE = atoi(argv[3]);
			}else{
				printf("请输入YUV420保存格式\nI420:0\tYU12,YV12:1\tNV12,NV21:2\n>");
				scanf("%d",&YUVSAVETYPE);
			}
			switch(YUVSAVETYPE){
				case 1:
					filenamecreate(YUVFILENAME,BMPFILENAME,"yuv420_yv12.yuv");
					break;
				case 2:
					filenamecreate(YUVFILENAME,BMPFILENAME,"yuv420_nv12.yuv");
					break;
				case 0:
				default:
					filenamecreate(YUVFILENAME,BMPFILENAME,"yuv420_i420.yuv");
					YUVSAVETYPE = 0;
					break;
			}
			break;
			break;
		default:
			printf("采样格式错误,设置为默认采样YUV444.\n");
			YUVSAMPLINGTYPE = 444;
			yuvrawdata = (BYTE *)malloc(sizeof(BYTE) * height * width * 3);
			if(argc>3){
				YUVSAVETYPE = atoi(argv[3]);
			}else{
				printf("请输入YUV444保存格式\nYUV:0\tYVU:1\tYUV(Plane):2\n>");
				scanf("%d",&YUVSAVETYPE);
			}
			switch(YUVSAVETYPE){
				case 1:
					filenamecreate(YUVFILENAME,BMPFILENAME,"yuv444_yvu.yuv");
					break;
				case 2:
					filenamecreate(YUVFILENAME,BMPFILENAME,"yuv444_yuv_plane.yuv");
					break;
				case 0:
				default:
					printf("保存格式错误,设置为默认默认保存格式YUV.\n");
					filenamecreate(YUVFILENAME,BMPFILENAME,"yuv444_yuv.yuv");
					YUVSAVETYPE = 0;
					break;
			}
	}
	printf("YUV文件名: %s\n",YUVFILENAME);
	save2yuv(yuvrawdata,cy,cu,cv,width,height,YUVFILENAME,YUVSAMPLINGTYPE,YUVSAVETYPE);
	
	YUV *yuvpixels_after  = (YUV *)malloc(sizeof(YUV) * height * width);
	readyuvpixels(yuvpixels_after,yuvrawdata,width,height,YUVSAMPLINGTYPE,YUVSAVETYPE);
	filenamecreate(JPGFILENAME,BMPFILENAME,"jpg");
	printf("JPEG文件名:%s\n",JPGFILENAME);
	BYTE *cr = (BYTE *)malloc(sizeof(BYTE) * height * width);
	BYTE *cg = (BYTE *)malloc(sizeof(BYTE) * height * width);
	BYTE *cb = (BYTE *)malloc(sizeof(BYTE) * height * width);
	ARGB *rgbpixels_after  = (ARGB *)malloc(sizeof(ARGB) * height * width);
	yuv2rgb(rgbpixels_after,yuvpixels_after,height*width);
	rgbsplit(cr,cg,cb,rgbpixels_after,height*width);
	save2jpeg(cr,cg,cb,width,height,JPGFILENAME);
	return 0;
}
