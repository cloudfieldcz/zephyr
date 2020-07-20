#include <logging/log_backend.h>
#include <logging/log_core.h>
#include <logging/log_msg.h>
#include <logging/log_output.h>
#include "log_backend_std.h"
#include <device.h>
#include <drivers/uart.h>
#include <assert.h>


#include <fcntl.h>
#include <sys/stat.h>

int ecb_open(const char *filename, int mode);
int ecb_write(int fd, const void *buf, int nbytes);

const u8_t log_file_name[] = "/SD:/L_LATEST.TXT";

	struct sd_file_device_t {
	const u8_t *log_file_name;
	int fd;
};

static struct sd_file_device_t sd_file_device = { .fd =  -1 ,
						  .log_file_name = 
							  log_file_name  };

static int char_out(u8_t *data, size_t length, void *ctx)
{
//	struct sd_file_device_t *dev = (struct sd_file_device_t *)ctx;
//
//	if (dev->fd > 0) {
//		return 0;
//	}
//
//	int ret = ecb_write(dev->fd, data, length);
//	return ret;
	return length;
}

static u8_t buf;

LOG_OUTPUT_DEFINE(log_output, char_out, &buf, 1);

static void put(const struct log_backend *const backend, struct log_msg *msg)
{
	u32_t flag = IS_ENABLED(CONFIG_LOG_BACKEND_UART_SYST_ENABLE) ?
			     LOG_OUTPUT_FLAG_FORMAT_SYST :
			     0;

	log_backend_std_put(&log_output, flag, msg);
}

static void log_backend_sd_file_init(void)
{
	struct sd_file_device_t *dev = &sd_file_device;

	int fd = ecb_open(dev->log_file_name, O_WRONLY | O_CREAT | O_BINARY);
	if (fd >= 0) {
		//LOG_DBG("Log file open OK log_file_name:%s", log_file_name);
		dev->fd = fd;
	} else {
		//LOG_ERR("Log file open ERR log_file_name:%s fd:%d",
		//	log_file_name, fd);
		dev->fd = -1;
	}

	log_output_ctx_set(&log_output, dev);
}

static void panic(struct log_backend const *const backend)
{
	log_backend_std_panic(&log_output);
}

static void dropped(const struct log_backend *const backend, u32_t cnt)
{
	ARG_UNUSED(backend);

	log_backend_std_dropped(&log_output, cnt);
}

static void sync_string(const struct log_backend *const backend,
			struct log_msg_ids src_level, u32_t timestamp,
			const char *fmt, va_list ap)
{
	u32_t flag = IS_ENABLED(CONFIG_LOG_BACKEND_UART_SYST_ENABLE) ?
			     LOG_OUTPUT_FLAG_FORMAT_SYST :
			     0;

	log_backend_std_sync_string(&log_output, flag, src_level, timestamp,
				    fmt, ap);
}

static void sync_hexdump(const struct log_backend *const backend,
			 struct log_msg_ids src_level, u32_t timestamp,
			 const char *metadata, const u8_t *data, u32_t length)
{
	u32_t flag = IS_ENABLED(CONFIG_LOG_BACKEND_UART_SYST_ENABLE) ?
			     LOG_OUTPUT_FLAG_FORMAT_SYST :
			     0;

	log_backend_std_sync_hexdump(&log_output, flag, src_level, timestamp,
				     metadata, data, length);
}

const struct log_backend_api log_backend_sd_file_api = {
	.put = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : put,
	.put_sync_string =
		IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? sync_string : NULL,
	.put_sync_hexdump =
		IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? sync_hexdump : NULL,
	.panic = panic,
	.init = log_backend_sd_file_init,
	.dropped = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : dropped,
};

LOG_BACKEND_DEFINE(log_backend_sd_file, log_backend_sd_file_api, false);
