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


#define CONFIG_TERGA 1

struct buffer_object {
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t handle;
	uint32_t size;
	uint8_t *vaddr;
	uint32_t fb_id;
};

struct buffer_object buf;

static int modeset_create_fb(int fd, struct buffer_object *bo)
{
	struct drm_mode_create_dumb create = {};
 	struct drm_mode_map_dumb map_dumb = {};

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

    map = (uint8_t*)(map_dumb.offset);
	memset(map, 0xa83232, bo->size);

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
	drmModePlaneRes *plane_res;
	uint32_t conn_id;
	uint32_t crtc_id;
	uint32_t plane_id;

    int i = 0;

	if(CONFIG_TERGA){
        fd = drmOpen("drm-nvdc",  NULL);
    }else{
        fd = drmOpen("/dev/dri/card0",O_RDWR | O_CLOEXEC);
    }
	
    if(fd == -1 )
    {
        printf("error open device\n");
        return -1;
    }

	res = drmModeGetResources(fd);
	crtc_id = res->crtcs[0];
	conn_id = res->connectors[0];

    //1.
	drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    //2.
	plane_res = drmModeGetPlaneResources(fd);
	plane_id = plane_res->planes[0];

	conn = drmModeGetConnector(fd, conn_id);
    //4K屏幕/2
	buf.width = conn->modes[0].hdisplay/4;
	buf.height = conn->modes[0].vdisplay/4;

    printf("WIDTH IS :%d\n",buf.width);
    printf("HEIGHT IS :%d\n",buf.height);
    

	modeset_create_fb(fd, &buf);

	drmModeSetCrtc(fd, crtc_id, buf.fb_id,
			0, 0, &conn_id, 1, &conn->modes[0]);

	usleep(200000);
    srand(9);
    //3.
	/* crop the rect from framebuffer(100, 150) to crtc(50, 50) */
    while(1)
    {
        for(;i< 100;i++)
        {
    
            drmModeSetPlane(fd, plane_id, crtc_id, buf.fb_id, 0,
	            20*i,20*i, 320, 320,
	            100 << 16, 150 << 16, 320 << 16, 320 << 16);
            usleep(20000);
        }
        i=0;
    }
    
	getchar();

	modeset_destroy_fb(fd, &buf);

	drmModeFreeConnector(conn);
	drmModeFreePlaneResources(plane_res);
	drmModeFreeResources(res);

	close(fd);

	return 0;
}