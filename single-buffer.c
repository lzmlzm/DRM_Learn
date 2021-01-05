#pragma pack()
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

//jetson显卡宏
#define CONFIG_TERGA 0

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
	struct drm_mode_create_dumb create;
 	struct drm_mode_map_dumb map_dumb;
	int ret=0;
	uint8_t* map = NULL;

	//清空
	memset(&create,0,sizeof(create));
	create.width = bo->width;
	create.height = bo->height;
	create.bpp = 32;


	//向显卡申请一块显存buffer
	ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);
	if(ret<0)
	{
		printf("can not create dumb buffer\n");
		return -1;
	}

	bo->pitch = create.pitch;
	bo->size = create.size;
	bo->handle = create.handle;

	//将设备连接到panel上，设置显示区域
	drmModeAddFB(fd, bo->width/2, bo->height/2, 24, 32, bo->pitch,
			   bo->handle, &bo->fb_id);

	memset(&map_dumb,0,sizeof(map_dumb));
	map_dumb.handle = create.handle;

	//将buffer映射到用户空间，允许用户修改显存数据
	ret = drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb);
	if(ret<0)
	{
		printf("can not map dumb buffer\n");
		return -1;
	}
	//bo->vaddr = mmap(0, create.size, PROT_READ | PROT_WRITE,
			//MAP_SHARED, fd, map.offset);

	//根据位移找到显存地址
	map = (uint8_t*)(map_dumb.offset);

	//修改显存数据，设置颜色
	memset(map, 0xfe, bo->size);

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
	drmModeRes *drm_res_info;
	drmModeCrtc *drm_crtc_info;
	uint32_t conn_id;
	uint32_t crtc_id;

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

	drm_res_info = drmModeGetResources(fd);
	if(!drm_res_info)
	{
		printf("error obtain resinfo\n");
		return -1;
	}
	//获取连接器和信号源ID
	crtc_id = drm_res_info->crtcs[0];
	conn_id = drm_res_info->connectors[0];

	//获取显示器
	conn = drmModeGetConnector(fd, conn_id);
	if(!conn)
	{
		printf("error connect\n");
		return -1;
	}else if(conn->connection!= DRM_MODE_CONNECTED)
	{
		printf("requested connector is not connected\n");
		return -1;
	}else if (conn->count_modes <=0)
	{
		printf("has no available modes\n");
		return -1;
	}

	//获取信号源
	drm_crtc_info = drmModeGetCrtc(fd, crtc_id);

	if (!drm_crtc_info) {

    	printf("Unable to obtain info for crtc &s", crtc_id);
		return -1;
  	}

	buf.width = conn->modes[0].hdisplay;
	buf.height = conn->modes[0].vdisplay;

	//申请显存buffer
	modeset_create_fb(fd, &buf);
	
	//连接显示器与信号源
	drmModeSetCrtc(fd, crtc_id, buf.fb_id,
			0, 0, &conn_id, 1, &conn->modes[0]);
	
	


	getchar();

	modeset_destroy_fb(fd, &buf);

	drmModeFreeConnector(conn);
	drmModeFreeResources(drm_res_info);

	close(fd);

	return 0;
}