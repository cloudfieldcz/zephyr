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

#include <fs/fs.h>

static const u8_t log_dir_name[] = "/SD:/LOG";
static const u8_t log_file_name[] = "/SD:/LOG/L_LATEST.TXT";
static const u8_t log_file_name_template[] = "/SD:/LOG/L_000000.TXT";
static const size_t log_file_sync_size = 100;
static const size_t log_file_max_size = 5 * 1024;
static struct fs_file_t log_file_fd;

struct sd_file_device_t {
	const u8_t *log_dir_name;
	const u8_t *log_file_name;
	struct fs_file_t *log_file_fd_p;
	bool log_file_open;
	size_t log_writen;
};

static struct sd_file_device_t sd_file_device = { .log_dir_name = log_dir_name,
						  .log_file_name =
							  log_file_name,
						  .log_file_fd_p = &log_file_fd,
						  .log_file_open = false,
						  .log_writen = 0 };

static int sd_file_check_file_size(struct sd_file_device_t *dev)
{
	struct fs_dirent entry;
	volatile int ret = fs_stat(dev->log_file_name, &entry);

	if (ret != 0) {
		goto sd_file_check_file_size_error;
	}

	if (entry.size > log_file_max_size) {
		dev->log_file_open = false;
		int ret = fs_close(dev->log_file_fd_p);
		if (ret != 0) {
			goto sd_file_check_file_size_error;
		}

		ret = fs_rename(dev->log_file_name, log_file_name_template);
		if (ret != 0) {
			goto sd_file_check_file_size_error;
		}

		ret = fs_open(dev->log_file_fd_p, dev->log_file_name);
		if (ret != 0) {
			dev->log_file_open = false;
			goto sd_file_check_file_size_error;
		}
		dev->log_file_open = true;
	}

	return 0;
sd_file_check_file_size_error:
	return -1;
}

static int sd_file_char_out(u8_t *data, size_t length, void *ctx)
{
	struct sd_file_device_t *dev = (struct sd_file_device_t *)ctx;

	if (dev->log_file_open == false) {
		return 0;
	}

	int ret = fs_write(dev->log_file_fd_p, data, length);
	if (ret < 0) {
		return 0;
	}

	if (dev->log_writen > log_file_sync_size) {
		int ret2 = fs_sync(dev->log_file_fd_p);
		if (ret2 != 0) {
			return 0;
		}
		dev->log_writen = 0;
		ret2 = sd_file_check_file_size(dev);
		if (ret2 != 0) {
			return 0;
		}
	} else {
		dev->log_writen++;
	}

	return ret;
	//	return length;
}

static u8_t buf;

LOG_OUTPUT_DEFINE(log_output, sd_file_char_out, &buf, 1);

static void put(const struct log_backend *const backend, struct log_msg *msg)
{
	u32_t flag = 0;
	log_backend_std_put(&log_output, flag, msg);
}

static void log_backend_sd_file_init(void)
{
	struct sd_file_device_t *dev = &sd_file_device;

	struct fs_dirent entry;
	volatile int res = fs_stat(dev->log_dir_name, &entry);
	if (res != 0) {
		volatile int res2 = fs_mkdir(dev->log_dir_name);
		if (res2 != 0) {
			dev->log_file_open = false;
			goto sd_file_init_end;
		}
	}

	res = fs_open(dev->log_file_fd_p, dev->log_file_name);
	if (res != 0) {
		dev->log_file_open = false;
		goto sd_file_init_end;
	}

	res = fs_seek(dev->log_file_fd_p, 0, FS_SEEK_END);
	if (res == 0) {
		dev->log_file_open = true;
	} else {
		dev->log_file_open = false;
	}

sd_file_init_end:
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
	u32_t flag = 0;

	log_backend_std_sync_string(&log_output, flag, src_level, timestamp,
				    fmt, ap);
}

static void sync_hexdump(const struct log_backend *const backend,
			 struct log_msg_ids src_level, u32_t timestamp,
			 const char *metadata, const u8_t *data, u32_t length)
{
	u32_t flag = 0;

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
