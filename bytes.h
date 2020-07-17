#ifndef __BYTES_H__
#define __BYTES_H__
#include <stdio.h>

typedef unsigned char BYTE;

int bytescut(BYTE *desbytes,const BYTE *srcbytes,size_t size,const int start,const int len){
	int flen = len;
	int slen = size;
	// printf("l=%d\n",slen);
	int idx;
	if(start>=slen){
		return -1;
	}
	if(start+len>slen){
		flen = len - start - 1;
	}
	for(idx=0;idx<flen;idx++){
		desbytes[idx] = srcbytes[start + idx];
		// printf("0x%02X->0x%02X\n",srcbytes[start + idx],desbytes[idx]);
	}
	return 0;
}

int bytes2int(const BYTE *bytes,const int len,const int order){
	int idx,cov,tmp1;
	cov = 0;
	if(order){
		for(idx=0;idx<len;idx++){
			tmp1 = (int)(bytes[idx]);
			cov *= 256;
			cov += tmp1;
		}
	}else{
		for(idx=len-1;idx>=0;idx--){
			tmp1 = (int)(bytes[idx]);
			cov *= 256;
			cov += tmp1;
		}
	}
	return cov;
}

int filenamecreate(BYTE *desbytes,const BYTE *srcbytes1,const BYTE *srcbytes2){
	//char *p1 = srcbytes1;
	//char *p2 = srcbytes2;
	//desbytes = srcbytes1;
	//while(*desbytes)desbytes++;
	while((*desbytes++=*srcbytes1++)!='\0');
	/* 去除扩展名 */
	*desbytes--;*desbytes--;*desbytes--;*desbytes--;
	while((*desbytes++=*srcbytes2++)!='\0');
	return 0;
}

int bytescpy(BYTE *desbytes,const BYTE *srcbytes){
	while((*desbytes++=*srcbytes++)!='\0');	
}
/*
int bytesadd(BYTE *desbytes,const BYTE srcbytes1[],const BYTE srcbytes2[]){
	int idx;
	if(start>=slen){
		return -1;
	}
	if(start+len>slen){
		flen = len - start - 1;
	}
	for(idx=0;idx<flen;idx++){
		desbytes[start+idx] = srcbytes[idx];
		// printf("0x%02X->0x%02X\n",srcbytes[start + idx],desbytes[idx]);
	}
	return 0;
}

int bytescpy(BYTE *desbytes,const BYTE *srcbytes,size_t size,const int len){
	int flen = len;
	int slen = size;
	// printf("l=%d\n",slen);
	int idx;
	if(flen>=slen-1){
		return -1;
	}
	for(idx=0;idx<flen;idx++){
		desbytes[idx] = srcbytes[idx];
		desbytes[idx+1] = 0x00;
		// printf("0x%02X->0x%02X\n",srcbytes[start + idx],desbytes[idx]);
	}
	return 0;
}
*/
#endif
