CC = arm-linux-gnueabihf-gcc
OBJS = main.c bytes.h image.h
INCLUDE = -I ./jpeg/include
LIBS=-L ./jpeg/lib -ljpeg -lm -ldl
conv:$(OBJS)
	$(CC)  $< -o  $@ $(INCLUDE) $(LIBS)
