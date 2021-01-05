#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <memory.h>

struct buffer_object {
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t handle;
	uint32_t size;
	uint32_t *vaddr;
	uint32_t fb_id;
};

struct buffer_object buf[2];

static int modeset_create_fb(int fd, struct buffer_object *bo, uint32_t color)
{
	struct drm_mode_create_dumb create = {};
 	struct drm_mode_map_dumb map_dumb = {};
	uint32_t i;
	uint8_t* map = NULL;

	create.width = bo->width;
	create.height = bo->height;
	create.bpp = 32;
	drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);

	bo->pitch = create.pitch;
	bo->size = create.size;
	bo->handle = create.handle;
	drmModeAddFB(fd, bo->width, bo->height, 24, 32, bo->pitch,
			   bo->handle, &bo->fb_id);

	map_dumb.handle = create.handle;
	drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb);

	//buffer除8 只显示1/8区域
	for (i = 0; i < bo->size / 8; i++)
	{
		//memset(map,0,map_dumb.offset);
		map = (uint8_t*)(map_dumb.offset);
		map[i] = color;
	}

	return 0;
}

static void modeset_destroy_fb(int fd, struct buffer_object *bo)
{
	struct drm_mode_destroy_dumb destroy = {};

	drmModeRmFB(fd, bo->fb_id);

	munmap(bo->vaddr, bo->size);

	destroy.handle = bo->handle;
	drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
}

int main(int argc, char **argv)
{
	int fd;
	drmModeConnector *conn;
	drmModeRes *res;
	uint32_t conn_id;
	uint32_t crtc_id;
    int i =30;
	fd = drmOpen("drm-nvdc",  NULL);
	if (fd < 0)
	{
		printf("error open device\n");
		return 0;
	}

	res = drmModeGetResources(fd);
	crtc_id = res->crtcs[0];
	conn_id = res->connectors[0];

	conn = drmModeGetConnector(fd, conn_id);
	buf[0].width = conn->modes[0].hdisplay;
	buf[0].height = conn->modes[0].vdisplay;
	buf[1].width = conn->modes[0].hdisplay;
	buf[1].height = conn->modes[0].vdisplay;

	modeset_create_fb(fd, &buf[0], 0xffffff);
	modeset_create_fb(fd, &buf[1], 0xa83232);
  while(i)
  {
	    drmModeSetCrtc(fd, crtc_id, buf[0].fb_id,
			    0, 0, &conn_id, 1, &conn->modes[0]);
       printf("buffer0\n");
      usleep(2000000);
	//getchar();

	    drmModeSetCrtc(fd, crtc_id, buf[1].fb_id,
			0, 0, &conn_id, 1, &conn->modes[0]);
      printf("buffer1\n");
      //getchar();
      usleep(2000000);
      i--;
  }
  
	

	modeset_destroy_fb(fd, &buf[1]);
	modeset_destroy_fb(fd, &buf[0]);

	drmModeFreeConnector(conn);
	drmModeFreeResources(res);

	close(fd);

	return 0;
}