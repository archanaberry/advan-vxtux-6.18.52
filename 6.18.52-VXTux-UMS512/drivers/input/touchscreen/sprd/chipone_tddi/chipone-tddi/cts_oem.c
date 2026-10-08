#define LOG_TAG         "Oem"

#include "cts_config.h"
#include "cts_platform.h"
#include "cts_core.h"
#include "cts_oem.h"
#include "cts_test.h"


#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/notifier.h>
#include <linux/namei.h>
#include <linux/mnt_idmapping.h>
#include <linux/fs.h>
/* VXTux 6.18: <linux/tp_usb_notifier.h> vendor tidak ada di mainline —
 * chain kompat vxtux_usb_notify_list (di disp_notify.o) menggantikannya. */
extern struct blocking_notifier_head vxtux_usb_notify_list;
static inline int tp_usb_register_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&vxtux_usb_notify_list, nb);
}
static inline int tp_usb_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&vxtux_usb_notify_list, nb);
}
extern int g_black_test;
extern const char *lcd_name;

static void *seq_start(struct seq_file *m, loff_t *pos)
{
    return *pos < 1 ? (void *)1 : NULL;
}

static void *seq_next(struct seq_file *m, void *v, loff_t *pos)
{
    ++*pos;
    return NULL;
}

static void seq_stop(struct seq_file *m, void *v)
{
    return;
}

/* Use for NOT use seq file to set private data with chipone_ts_data */
static int proc_simple_open(struct inode *inode, struct file *file)
{
    file->private_data = pde_data(inode);
    return 0;
}

#define ALLOC_TEST_DATA_MEM(type, size) \
    do { \
        if (oem_data->test_##type) { \
            if (oem_data->type##_test_data == NULL) { \
                cts_info(" - Alloc " #type " test data mem size %d", size); \
                oem_data->type##_test_data = kmalloc(size, GFP_KERNEL); \
                if (oem_data->type##_test_data == NULL) { \
                    cts_err("Alloc " #type " test data mem failed"); \
                    return -ENOMEM; \
                } \
                oem_data->type##_test_data_buff_size = size; \
                oem_data->type##_test_data_wr_size = 0;\
            } \
            memset(oem_data->type##_test_data, 0, size); \
        } \
    } while (0)

/* NOTE: Any test data mem alloc failed will NOT clean other mem */
static int alloc_sleftest_data_mem(struct cts_oem_data *oem_data, int nodes)
{
    cts_info("Alloc selftest data");

    ALLOC_TEST_DATA_MEM(rawdata,
        nodes * 2 * oem_data->rawdata_test_frames);
    ALLOC_TEST_DATA_MEM(noise,
        nodes * 2 * (oem_data->noise_test_frames + 1));
    ALLOC_TEST_DATA_MEM(open, nodes * 2);
    ALLOC_TEST_DATA_MEM(short, nodes * 2 * 7);
    ALLOC_TEST_DATA_MEM(comp_cap, nodes);

    return 0;
}
#undef ALLOC_TEST_DATA_MEM

#define FREE_TEST_DATA_MEM(type) \
    do { \
        if (oem_data->type##_test_data) { \
            cts_info("- Free " #type " test data mem"); \
            kfree(oem_data->type##_test_data); \
            oem_data->type##_test_data = NULL; \
            oem_data->type##_test_data_buff_size = 0; \
        } \
    } while(0)

static void free_baseline_test_data_mem(struct cts_oem_data *oem_data)
{
    cts_info("Free selftest data");

    FREE_TEST_DATA_MEM(rawdata);
    FREE_TEST_DATA_MEM(noise);
    FREE_TEST_DATA_MEM(open);
    FREE_TEST_DATA_MEM(short);
    FREE_TEST_DATA_MEM(comp_cap);
}
#undef FREE_TEST_DATA_MEM

#ifdef CONFIG_CTS_GESTURE_TEST
#define ALLOC_BLACK_TEST_DATA_MEM(type, size) \
    do { \
        if (oem_data->test_##type) { \
            if (oem_data->type##_test_data == NULL) { \
                cts_info(" - Alloc " #type " test data mem size %d", size); \
                oem_data->type##_test_data = kmalloc(size, GFP_KERNEL); \
                if (oem_data->type##_test_data == NULL) { \
                    cts_err("Alloc " #type " test data mem failed"); \
                    return -ENOMEM; \
                } \
                oem_data->type##_test_data_buff_size = size; \
            } \
            memset(oem_data->type##_test_data, 0, size); \
        } \
    } while (0)

int alloc_black_selftest_data_mem(struct cts_oem_data *oem_data, int nodes)
{
	cts_info("Alloc black selftest data");
	ALLOC_BLACK_TEST_DATA_MEM(gesture_rawdata,
		nodes * 2 * oem_data->gesture_rawdata_test_frames);
	ALLOC_BLACK_TEST_DATA_MEM(gesture_lp_rawdata,
		nodes / 4 * oem_data->gesture_lp_rawdata_test_frames);
	ALLOC_BLACK_TEST_DATA_MEM(gesture_noise,
		nodes * 2 * ( oem_data->gesture_noise_test_frames + 1));
	ALLOC_BLACK_TEST_DATA_MEM(gesture_lp_noise,
		nodes / 4 * ( oem_data->gesture_lp_noise_test_frames + 1));
	return 0;
}
#undef ALLOC_BLACK_TEST_DATA_MEM

#define FREE_BLACK_TEST_DATA_MEM(type) \
    do { \
        if (oem_data->type##_test_data) { \
            cts_info("- Free " #type " test data mem"); \
            kfree(oem_data->type##_test_data); \
            oem_data->type##_test_data = NULL; \
            oem_data->type##_test_data_buff_size = 0; \
        } \
    } while(0)

static void free_black_screen_test_data_mem(struct cts_oem_data *oem_data)
{
    cts_info("Free black selftest data");

    FREE_BLACK_TEST_DATA_MEM(gesture_rawdata);
    FREE_BLACK_TEST_DATA_MEM(gesture_lp_rawdata);
    FREE_BLACK_TEST_DATA_MEM(gesture_noise);
    FREE_BLACK_TEST_DATA_MEM(gesture_lp_noise);
}
#undef FREE_BLACK_TEST_DATA_MEM
#endif



/* VXTux 6.18: set_fs/get_fs + ksys_mkdir dihapus —
 * kern_path + lookup_one + vfs_mkdir (kontrak dentry baru). */
int cts_dev_mkdir(char *name, umode_t mode)
{
	int err = 0;
	struct path parent_path;
	struct dentry *child;
	struct qstr child_name;
	char *slash;
	const char *parent_dir;

	cts_err("enter mkdir: %s\n", name);

	slash = strrchr(name, '/');
	if (!slash)
		return -EINVAL;

	*slash = '\0';
	parent_dir = (slash == name) ? "/" : name;
	err = kern_path(parent_dir, 0, &parent_path);
	*slash = '/';
	if (err)
		return err;

	child_name = (struct qstr)QSTR(slash + 1);
	child = lookup_one(&nop_mnt_idmap, &child_name, parent_path.dentry);
	if (IS_ERR(child)) {
		err = PTR_ERR(child);
		path_put(&parent_path);
		return err;
	}

	if (d_really_is_positive(child)) {
		err = 0;
		dput(child);
	} else {
		struct dentry *created;

		created = vfs_mkdir(&nop_mnt_idmap,
				    parent_path.dentry->d_inode,
				    child, mode);
		if (IS_ERR(created)) {
			err = PTR_ERR(created);
		} else {
			dput(created);
			err = 0;
		}
	}
	path_put(&parent_path);

	cts_err("exit ret == %d.\n", err);
	return err;
}

static int dump_tsdata_row_to_buffer(char *buf, size_t size, const u16 *data,
    int cols, const char *prefix, const char *suffix, char seperator)
{
    int c, count = 0;

    if (prefix) {
        count += scnprintf(buf, size, "%s", prefix);
    }

    for (c = 0; c < cols; c++) {
        count += scnprintf(buf + count, size - count,
            "%4u%c ", data[c], seperator);
    }

    if (suffix) {
        count += scnprintf(buf + count, size - count, "%s", suffix);
    }

    return count;
}

int dump_tsdata_to_csv_file(const char *filepath, int flags,
    const u16 *data, int frames, int rows, int cols)
{
    struct file *file = NULL;
    int i, r, ret = 0;
    loff_t pos = 0;

    cts_info("Dump tsdata to csv file: '%s' "
             "flags: 0x%x data: %p frames: %d row: %d col: %d",
        filepath, flags, data, frames, rows, cols);

    file = filp_open(filepath, flags, 0666);
    if (IS_ERR(file)) {
        cts_err("Open file '%s' failed %ld", filepath, PTR_ERR(file));
        return PTR_ERR(file);
    }

    for (i = 0; i < frames; i++) {
	    for (r = 0; r < rows; r++) {
	        char linebuf[256];
	        int len;

	        len = dump_tsdata_row_to_buffer(linebuf, sizeof(linebuf),
	            data, cols, NULL, "\n", ',');
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
	        ret = kernel_write(file, linebuf, len, &pos);
#else
	        ret = kernel_write(file, linebuf, len, pos);
	        pos += len;
#endif
	        if (ret != len) {
	            cts_err("Write to file '%s' failed %d",
	                filepath, ret);
	            goto close_file;
	        }

	        data += cols;
	    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
	    ret = kernel_write(file, "\n", 1, &pos);
#else
	    ret = kernel_write(file, "\n", 1, pos);
	    pos ++;
#endif
	    if (ret != 1) {
	        cts_err("Write newline to file '%s' failed %d",
	            filepath, ret);
	        goto close_file;
	    }
    }

close_file: {
        int r = filp_close(file, NULL);
        if (r) {
            cts_err("Close file '%s' failed %d", filepath, r);
        }
    }

    return ret;
}

static void dump_tsdata_to_seq_file(struct seq_file *m,
    const void *data, int rows, int cols)
{
    int r;

    for (r = 0; r < rows; r++) {
        char linebuf[256];
        int len;

        len = dump_tsdata_row_to_buffer(linebuf, sizeof(linebuf),
            data, cols, NULL, "\n", ',');
        seq_puts(m, linebuf);

        data += cols * 2;
    }
}
static int parse_selftest_dt(struct cts_oem_data *oem_data,
    struct device_node *np)
{
    int ret;

    cts_info("Parse selftest dt");

    oem_data->test_reset_pin = OEM_OF_DEF_PROPVAL_TEST_RESET_PIN ||
        of_property_read_bool(np, OEM_OF_PROPNAME_TEST_RESET_PIN);

    oem_data->test_int_pin = OEM_OF_DEF_PROPVAL_TEST_INT_PIN ||
        of_property_read_bool(np, OEM_OF_PROPNAME_TEST_INT_PIN);

    oem_data->test_rawdata = OEM_OF_DEF_PROPVAL_TEST_RAWDATA ||
        of_property_read_bool(np, OEM_OF_PROPNAME_TEST_RAWDATA);
    if (oem_data->test_rawdata) {
        oem_data->rawdata_test_frames = OEM_OF_DEF_PROPVAL_RAWDATA_FRAMES;
        ret = of_property_read_u32(np, OEM_OF_PROPNAME_RAWDATA_FRAMES,
            &oem_data->rawdata_test_frames);
        if (ret) {
            cts_warn("Parse '"OEM_OF_PROPNAME_RAWDATA_FRAMES"' failed %d", ret);
        }

        oem_data->rawdata_min = OEM_OF_DEF_PROPVAL_RAWDATA_MIN;
        ret = of_property_read_u32(np, OEM_OF_PROPNAME_RAWDATA_MIN,
            &oem_data->rawdata_min);
        if (ret) {
            cts_warn("Parse '"OEM_OF_PROPNAME_RAWDATA_MIN"' failed %d", ret);
        }

        oem_data->rawdata_max = OEM_OF_DEF_PROPVAL_RAWDATA_MAX;
        ret = of_property_read_u32(np, OEM_OF_PROPNAME_RAWDATA_MAX,
            (u32 *)&oem_data->rawdata_max);
        if (ret) {
            cts_warn("Parse '"OEM_OF_PROPNAME_RAWDATA_MAX"' failed %d", ret);
        }
        }

    oem_data->test_noise = OEM_OF_DEF_PROPVAL_TEST_NOISE ||
        of_property_read_bool(np, OEM_OF_PROPNAME_TEST_NOISE);
    if (oem_data->test_noise) {
        oem_data->noise_test_frames = OEM_OF_DEF_PROPVAL_NOISE_FRAMES;
        ret = of_property_read_u32(np, OEM_OF_PROPNAME_NOISE_FRAMES,
            &oem_data->noise_test_frames);
        if (ret) {
            cts_warn("Parse '"OEM_OF_PROPNAME_NOISE_FRAMES"' failed %d", ret);
        }

        oem_data->noise_max = OEM_OF_DEF_PROPVAL_NOISE_MAX;
        ret = of_property_read_u32(np, OEM_OF_PROPNAME_NOISE_MAX,
            (u32 *)&oem_data->noise_max);
        if (ret) {
            cts_warn("Parse '"OEM_OF_PROPNAME_NOISE_MAX"' failed %d", ret);
        }
    }

    oem_data->test_open = OEM_OF_DEF_PROPVAL_TEST_OPEN ||
        of_property_read_bool(np, OEM_OF_PROPNAME_TEST_OPEN);
    if (oem_data->test_open) {
        oem_data->open_min = OEM_OF_DEF_PROPVAL_OPEN_MIN;
        ret = of_property_read_u32(np, OEM_OF_PROPNAME_OPEN_MIN,
            (u32 *)&oem_data->open_min);
        if (ret) {
            cts_warn("Parse '"OEM_OF_PROPNAME_OPEN_MIN"' failed %d", ret);
        }
    }

    oem_data->test_short = OEM_OF_DEF_PROPVAL_TEST_SHORT ||
        of_property_read_bool(np, OEM_OF_PROPNAME_TEST_SHORT);
    if (oem_data->test_short) {
        oem_data->short_min = OEM_OF_DEF_PROPVAL_SHORT_MIN;
        ret = of_property_read_u32(np, OEM_OF_PROPNAME_SHORT_MIN,
            (u32 *)&oem_data->short_min);
        if (ret) {
            cts_warn("Parse '"OEM_OF_PROPNAME_SHORT_MIN"' failed %d", ret);
        }
    }

    oem_data->test_comp_cap = OEM_OF_DEF_PROPVAL_TEST_COMP_CAP ||
        of_property_read_bool(np, OEM_OF_PROPNAME_TEST_COMP_CAP);
    if (oem_data->test_comp_cap) {
        oem_data->comp_cap_min = OEM_OF_DEF_PROPVAL_COMP_CAP_MIN;
        ret = of_property_read_u32(np, OEM_OF_PROPNAME_COMP_CAP_MIN,
            (u32 *)&oem_data->comp_cap_min);
        if (ret) {
            cts_warn("Parse '"OEM_OF_PROPNAME_COMP_CAP_MIN"' failed %d", ret);
        }

        oem_data->comp_cap_max = OEM_OF_DEF_PROPVAL_COMP_CAP_MAX;
        ret = of_property_read_u32(np, OEM_OF_PROPNAME_COMP_CAP_MAX,
            (u32 *)&oem_data->comp_cap_max);
        if (ret) {
            cts_warn("Parse '"OEM_OF_PROPNAME_COMP_CAP_MAX"' failed %d", ret);
        }
    }

    oem_data->test_config_from_dt_has_parsed = true;

    return 0;
}

	static int auo_parse_selftest_dt(struct cts_oem_data *oem_data,
		struct device_node *np)
	{
		int ret;

		cts_info("Parse selftest dt");

		oem_data->test_reset_pin = OEM_OF_DEF_PROPVAL_TEST_RESET_PIN ||
			of_property_read_bool(np, OEM_OF_PROPNAME_TEST_RESET_PIN);

		oem_data->test_int_pin = OEM_OF_DEF_PROPVAL_TEST_INT_PIN ||
			of_property_read_bool(np, OEM_OF_PROPNAME_TEST_INT_PIN);

		oem_data->test_rawdata = OEM_OF_DEF_PROPVAL_TEST_RAWDATA ||
			of_property_read_bool(np, OEM_OF_PROPNAME_TEST_RAWDATA);
		if (oem_data->test_rawdata) {
			oem_data->rawdata_test_frames = AUO_OEM_OF_DEF_PROPVAL_RAWDATA_FRAMES;
			ret = of_property_read_u32(np, OEM_OF_PROPNAME_RAWDATA_FRAMES,
				&oem_data->rawdata_test_frames);
			if (ret) {
				cts_warn("Parse '"OEM_OF_PROPNAME_RAWDATA_FRAMES"' failed %d", ret);
			}

			oem_data->rawdata_min = AUO_OEM_OF_DEF_PROPVAL_RAWDATA_MIN;
			ret = of_property_read_u32(np, OEM_OF_PROPNAME_RAWDATA_MIN,
				&oem_data->rawdata_min);
			if (ret) {
				cts_warn("Parse '"OEM_OF_PROPNAME_RAWDATA_MIN"' failed %d", ret);
			}

			oem_data->rawdata_max = AUO_OEM_OF_DEF_PROPVAL_RAWDATA_MAX;
			ret = of_property_read_u32(np, OEM_OF_PROPNAME_RAWDATA_MAX,
				(u32 *)&oem_data->rawdata_max);
			if (ret) {
				cts_warn("Parse '"OEM_OF_PROPNAME_RAWDATA_MAX"' failed %d", ret);
			}
			}

		oem_data->test_noise = OEM_OF_DEF_PROPVAL_TEST_NOISE ||
			of_property_read_bool(np, OEM_OF_PROPNAME_TEST_NOISE);
		if (oem_data->test_noise) {
			oem_data->noise_test_frames = AUO_OEM_OF_DEF_PROPVAL_NOISE_FRAMES;
			ret = of_property_read_u32(np, OEM_OF_PROPNAME_NOISE_FRAMES,
				&oem_data->noise_test_frames);
			if (ret) {
				cts_warn("Parse '"OEM_OF_PROPNAME_NOISE_FRAMES"' failed %d", ret);
			}

			oem_data->noise_max = AUO_OEM_OF_DEF_PROPVAL_NOISE_MAX;
			ret = of_property_read_u32(np, OEM_OF_PROPNAME_NOISE_MAX,
				(u32 *)&oem_data->noise_max);
			if (ret) {
				cts_warn("Parse '"OEM_OF_PROPNAME_NOISE_MAX"' failed %d", ret);
			}
		}

		oem_data->test_open = OEM_OF_DEF_PROPVAL_TEST_OPEN ||
			of_property_read_bool(np, OEM_OF_PROPNAME_TEST_OPEN);
		if (oem_data->test_open) {
			oem_data->open_min = AUO_OEM_OF_DEF_PROPVAL_OPEN_MIN;
			ret = of_property_read_u32(np, OEM_OF_PROPNAME_OPEN_MIN,
				(u32 *)&oem_data->open_min);
			if (ret) {
				cts_warn("Parse '"OEM_OF_PROPNAME_OPEN_MIN"' failed %d", ret);
			}
		}

		oem_data->test_short = OEM_OF_DEF_PROPVAL_TEST_SHORT ||
			of_property_read_bool(np, OEM_OF_PROPNAME_TEST_SHORT);
		if (oem_data->test_short) {
			oem_data->short_min = AUO_OEM_OF_DEF_PROPVAL_SHORT_MIN;
			ret = of_property_read_u32(np, OEM_OF_PROPNAME_SHORT_MIN,
				(u32 *)&oem_data->short_min);
			if (ret) {
				cts_warn("Parse '"OEM_OF_PROPNAME_SHORT_MIN"' failed %d", ret);
			}
		}

		oem_data->test_comp_cap = OEM_OF_DEF_PROPVAL_TEST_COMP_CAP ||
			of_property_read_bool(np, OEM_OF_PROPNAME_TEST_COMP_CAP);
		if (oem_data->test_comp_cap) {
			oem_data->comp_cap_min = AUO_OEM_OF_DEF_PROPVAL_COMP_CAP_MIN;
			ret = of_property_read_u32(np, OEM_OF_PROPNAME_COMP_CAP_MIN,
				(u32 *)&oem_data->comp_cap_min);
			if (ret) {
				cts_warn("Parse '"OEM_OF_PROPNAME_COMP_CAP_MIN"' failed %d", ret);
			}

			oem_data->comp_cap_max = AUO_OEM_OF_DEF_PROPVAL_COMP_CAP_MAX;
			ret = of_property_read_u32(np, OEM_OF_PROPNAME_COMP_CAP_MAX,
				(u32 *)&oem_data->comp_cap_max);
			if (ret) {
				cts_warn("Parse '"OEM_OF_PROPNAME_COMP_CAP_MAX"' failed %d", ret);
			}
		}

		oem_data->test_config_from_dt_has_parsed = true;

		return 0;
	}

static void print_selftest_config(const struct cts_oem_data *oem_data)
{
    cts_info("Seltest configuration:");

    cts_info(" - %-32s = %c", OEM_OF_PROPNAME_TEST_RESET_PIN,
        oem_data->test_reset_pin ? 'Y' : 'N');

    cts_info(" - %-32s = %c", OEM_OF_PROPNAME_TEST_INT_PIN,
        oem_data->test_int_pin ? 'Y' : 'N');

    cts_info(" - %-32s = %c", OEM_OF_PROPNAME_TEST_RAWDATA,
        oem_data->test_rawdata ? 'Y' : 'N');
    if (oem_data->test_rawdata) {
        cts_info(" - %-32s = %u", OEM_OF_PROPNAME_RAWDATA_FRAMES,
            oem_data->rawdata_test_frames);
        cts_info(" - %-32s = %d", OEM_OF_PROPNAME_RAWDATA_MIN,
            oem_data->rawdata_min);
        cts_info(" - %-32s = %d", OEM_OF_PROPNAME_RAWDATA_MAX,
            oem_data->rawdata_max);
    }

    cts_info(" - %-32s = %c", OEM_OF_PROPNAME_TEST_NOISE,
        oem_data->test_noise ? 'Y' : 'N');
    if (oem_data->test_noise) {
        cts_info(" - %-32s = %u", OEM_OF_PROPNAME_NOISE_FRAMES ,
            oem_data->noise_test_frames);
        cts_info(" - %-32s = %d", OEM_OF_PROPNAME_NOISE_MAX,
            oem_data->noise_max);
    }

    cts_info(" - %-32s = %c", OEM_OF_PROPNAME_TEST_OPEN,
        oem_data->test_open ? 'Y' : 'N');
    if (oem_data->test_open) {
        cts_info(" - %-32s = %d", OEM_OF_PROPNAME_OPEN_MIN,
            oem_data->open_min);
    }

    cts_info(" - %-32s = %c", OEM_OF_PROPNAME_TEST_SHORT,
        oem_data->test_short ? 'Y' : 'N');
    if (oem_data->test_short) {
        cts_info(" - %-32s = %d", OEM_OF_PROPNAME_SHORT_MIN,
            oem_data->short_min);
    }

    cts_info(" - %-32s = %c", OEM_OF_PROPNAME_TEST_COMP_CAP,
        oem_data->test_comp_cap ? 'Y' : 'N');
    if (oem_data->test_comp_cap) {
        cts_info(" - %-32s = %d", OEM_OF_PROPNAME_COMP_CAP_MIN,
            oem_data->comp_cap_min);
        cts_info(" - %-32s = %d", OEM_OF_PROPNAME_COMP_CAP_MAX,
            oem_data->comp_cap_max);
    }
}

static void do_selftest(struct cts_oem_data *oem_data)
{
    struct cts_test_param test_param;
    struct timespec64 ts;
    struct rtc_time rtc_tm;
    char touch_data_filepath[256];

    ktime_get_real_ts64(&ts);
    /* VXTux 6.18: rtc_time_to_tm dihapus — rtc_ktime_to_tm(ktime) */
    rtc_tm = rtc_ktime_to_tm(ns_to_ktime(timespec64_to_ns(&ts)
                 - (s64)sys_tz.tz_minuteswest * 60 * NSEC_PER_SEC));

    cts_info("Do selftest");

    if (oem_data->test_reset_pin) {
        memset(&test_param, 0, sizeof(test_param));
        test_param.test_item = CTS_TEST_RESET_PIN;
        oem_data->reset_pin_test_result =
            cts_test_reset_pin(&oem_data->cts_data->cts_dev, &test_param);
        if (oem_data->reset_pin_test_result) {
            cts_err("Test reset pin failed %d",
                oem_data->reset_pin_test_result);
        }
    }

    if (oem_data->test_int_pin) {
        memset(&test_param, 0, sizeof(test_param));
        test_param.test_item = CTS_TEST_INT_PIN;
        oem_data->int_pin_test_result =
            cts_test_int_pin(&oem_data->cts_data->cts_dev, &test_param);
        if (oem_data->int_pin_test_result) {
            cts_err("Test int pin failed %d,%d",
                oem_data->int_pin_test_result);
        }
    }

    if (oem_data->test_rawdata) {
        struct cts_rawdata_test_priv_param priv_param = {0};

        memset(&test_param, 0, sizeof(test_param));
        test_param.test_item = CTS_TEST_RAWDATA;
        test_param.flags =
            CTS_TEST_FLAG_VALIDATE_DATA |
            CTS_TEST_FLAG_VALIDATE_MIN |
            CTS_TEST_FLAG_VALIDATE_MAX |
            CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
            CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE |
            CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE |
            CTS_TEST_FLAG_DUMP_TEST_DATA_TO_USERSPACE;
        test_param.min = &oem_data->rawdata_min;
        test_param.max = &oem_data->rawdata_max;
        test_param.test_data_buf = oem_data->rawdata_test_data;
        test_param.test_data_buf_size = oem_data->rawdata_test_data_buff_size;
        test_param.test_data_wr_size = &oem_data->rawdata_test_data_wr_size;
        snprintf(touch_data_filepath, sizeof(touch_data_filepath),
            OEM_TEST_DATA_DIR"%04d%02d%02d_%02d%02d%02d/"
            OEM_RAWDATA_TEST_DATA_FILEPATH,
            rtc_tm.tm_year + 1900, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
            rtc_tm.tm_hour, rtc_tm.tm_min,rtc_tm.tm_sec);
        test_param.test_data_filepath =
            kstrdup(touch_data_filepath, GFP_KERNEL);

        priv_param.frames = oem_data->rawdata_test_frames;
        test_param.priv_param = &priv_param;
        test_param.priv_param_size = sizeof(priv_param);

        oem_data->rawdata_test_result =
            cts_test_rawdata(&oem_data->cts_data->cts_dev, &test_param);
        if (oem_data->rawdata_test_result) {
            cts_err("Test rawdata failed %d",
                oem_data->rawdata_test_result);
        }
        if (test_param.test_data_filepath)
            kfree(test_param.test_data_filepath);
    }

    if (oem_data->test_noise) {
        struct cts_noise_test_priv_param priv_param;

        memset(&test_param, 0, sizeof(test_param));
        test_param.test_item = CTS_TEST_NOISE;
        test_param.flags =
            CTS_TEST_FLAG_VALIDATE_DATA |
            CTS_TEST_FLAG_VALIDATE_MAX |
            CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
            CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE |
            CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE |
            CTS_TEST_FLAG_DUMP_TEST_DATA_TO_USERSPACE;
        test_param.max = &oem_data->noise_max;
        test_param.test_data_buf = oem_data->noise_test_data;
        test_param.test_data_buf_size = oem_data->noise_test_data_buff_size;
        test_param.test_data_wr_size = &oem_data->noise_test_data_wr_size;
        snprintf(touch_data_filepath, sizeof(touch_data_filepath),
            OEM_TEST_DATA_DIR"%04d%02d%02d_%02d%02d%02d/"
            OEM_NOISE_TEST_DATA_FILEPATH,
            rtc_tm.tm_year + 1900, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
            rtc_tm.tm_hour, rtc_tm.tm_min,rtc_tm.tm_sec);
        test_param.test_data_filepath =
            kstrdup(touch_data_filepath, GFP_KERNEL);
        priv_param.frames = oem_data->noise_test_frames;
        test_param.priv_param = &priv_param;
        test_param.priv_param_size = sizeof(priv_param);

        oem_data->noise_test_result =
            cts_test_noise(&oem_data->cts_data->cts_dev, &test_param);
        if (oem_data->noise_test_result) {
            cts_err("Test noise failed %d",
                oem_data->noise_test_result);
        }
        if (test_param.test_data_filepath)
            kfree(test_param.test_data_filepath);
    }

    if (oem_data->test_open) {
        memset(&test_param, 0, sizeof(test_param));

        test_param.test_item = CTS_TEST_OPEN;
        test_param.flags =
            CTS_TEST_FLAG_VALIDATE_DATA |
            CTS_TEST_FLAG_VALIDATE_MIN |
            CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
            CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE |
            CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE |
            CTS_TEST_FLAG_DUMP_TEST_DATA_TO_USERSPACE;
        test_param.min = &oem_data->open_min;
        test_param.test_data_buf = oem_data->open_test_data;
        test_param.test_data_buf_size = oem_data->open_test_data_buff_size;
        test_param.test_data_wr_size = &oem_data->open_test_data_wr_size;
        snprintf(touch_data_filepath, sizeof(touch_data_filepath),
            OEM_TEST_DATA_DIR"%04d%02d%02d_%02d%02d%02d/"
            OEM_OPEN_TEST_DATA_FILEPATH,
            rtc_tm.tm_year + 1900, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
            rtc_tm.tm_hour, rtc_tm.tm_min,rtc_tm.tm_sec);
        test_param.test_data_filepath =
            kstrdup(touch_data_filepath, GFP_KERNEL);
        oem_data->open_test_result =
            cts_test_open(&oem_data->cts_data->cts_dev, &test_param);
        if (oem_data->open_test_result) {
            cts_err("Test open failed %d", oem_data->open_test_result);
        }
        if (test_param.test_data_filepath)
            kfree(test_param.test_data_filepath);
    }

    if (oem_data->test_short) {
        memset(&test_param, 0, sizeof(test_param));

        test_param.test_item = CTS_TEST_SHORT;
        test_param.flags =
            CTS_TEST_FLAG_VALIDATE_DATA |
            CTS_TEST_FLAG_VALIDATE_MIN |
            CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
            CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE |
            CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE |
            CTS_TEST_FLAG_DUMP_TEST_DATA_TO_USERSPACE;
        test_param.min = &oem_data->short_min;
        test_param.test_data_buf = oem_data->short_test_data;
        test_param.test_data_buf_size = oem_data->short_test_data_buff_size;
        test_param.test_data_wr_size = &oem_data->short_test_data_wr_size;
        snprintf(touch_data_filepath, sizeof(touch_data_filepath),
            OEM_TEST_DATA_DIR"%04d%02d%02d_%02d%02d%02d/"
            OEM_SHORT_TEST_DATA_FILEPATH,
            rtc_tm.tm_year + 1900, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
            rtc_tm.tm_hour, rtc_tm.tm_min,rtc_tm.tm_sec);
        test_param.test_data_filepath =
            kstrdup(touch_data_filepath, GFP_KERNEL);
        oem_data->short_test_result =
            cts_test_short(&oem_data->cts_data->cts_dev, &test_param);
        if (oem_data->short_test_result) {
            cts_err("Test short failed %d", oem_data->short_test_result);
        }
        if (test_param.test_data_filepath)
            kfree(test_param.test_data_filepath);
    }

    if (oem_data->test_comp_cap) {
        memset(&test_param, 0, sizeof(test_param));

        test_param.test_item = CTS_TEST_COMPENSATE_CAP;
        test_param.flags =
            CTS_TEST_FLAG_VALIDATE_DATA |
            CTS_TEST_FLAG_VALIDATE_MIN |
            CTS_TEST_FLAG_VALIDATE_MAX |
            CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
            CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE |
            CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE |
            CTS_TEST_FLAG_DUMP_TEST_DATA_TO_USERSPACE;
        test_param.min = &oem_data->comp_cap_min;
        test_param.max = &oem_data->comp_cap_max;
        test_param.test_result = &oem_data->comp_cap_test_result;
        test_param.test_data_buf = oem_data->comp_cap_test_data;
        test_param.test_data_buf_size = oem_data->comp_cap_test_data_buff_size;
        test_param.test_data_wr_size = &oem_data->comp_cap_test_data_wr_size;
        snprintf(touch_data_filepath, sizeof(touch_data_filepath),
            OEM_TEST_DATA_DIR"%04d%02d%02d_%02d%02d%02d/"
            OEM_COMP_CAP_TEST_DATA_FILEPATH,
            rtc_tm.tm_year + 1900, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
            rtc_tm.tm_hour, rtc_tm.tm_min,rtc_tm.tm_sec);
        test_param.test_data_filepath =
            kstrdup(touch_data_filepath, GFP_KERNEL);
        oem_data->comp_cap_test_result =
            cts_test_compensate_cap(&oem_data->cts_data->cts_dev, &test_param);
        if (oem_data->comp_cap_test_result) {
            cts_err("Test compensate cap failed %d", oem_data->comp_cap_test_result);
        }
        if (test_param.test_data_filepath)
            kfree(test_param.test_data_filepath);
    }
}

static void *selftest_seq_start(struct seq_file *m, loff_t *pos)
{
    return *pos < 1 ? (void *)1 : NULL;
}

static void *selftest_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
    ++*pos;
    return NULL;
}

static void selftest_seq_stop(struct seq_file *m, void *v)
{
    return;
}

static int selftest_seq_show(struct seq_file *m, void *v)
{
    struct chipone_ts_data *cts_data = (struct chipone_ts_data *)m->private;
    struct cts_oem_data *oem_data = NULL;
    int rows, cols;
	int test_result = 0;

    cts_info("Show seq selftest");

    if (cts_data == NULL) {
        cts_err("Selftest seq file private data = NULL");
        return -EFAULT;
    }

    oem_data = cts_data->vendor_data;
    rows  = cts_data->cts_dev.fwdata.rows;
    cols  = cts_data->cts_dev.fwdata.cols;

    cts_info("FW Version %04x!\n\n", cts_data->cts_dev.fwdata.version);

    //reset pin test
    if (oem_data->test_reset_pin) {
        //cts_info("Reset-Pin Test %s!\n\n", oem_data->reset_pin_test_result == 0 ? "PASS" : "FAIL");
        if(oem_data->reset_pin_test_result != 0) {
          test_result += 1;
		  cts_info("Reset-Pin Test FAIL.\n");
		} else {
           cts_info("Reset-Pin Test PASS.\n");
		}
    }

    //int pin test
    if (oem_data->test_int_pin) {
        cts_info("Int-Pin Test %s!\n\n",
            oem_data->int_pin_test_result == 0 ? "PASS" : "FAIL");
    }

	//FW Rawdata Test
    if (oem_data->test_rawdata) {
        cts_info("FW Rawdata Test");
        if (oem_data->rawdata_test_result == 0) {
            cts_info(" PASS!\n\n");
        } else if (oem_data->rawdata_test_result > 0) {
            test_result += 1;
            cts_info(" FAIL!\n");
        } else {
            test_result += 1;
            cts_info(" ERROR(%d)!\n\n", oem_data->rawdata_test_result);
        }
    }

    //Noise Test
    if (oem_data->test_noise) {
        cts_info("Noise Test");
        if (oem_data->noise_test_result == 0) {
            cts_info(" PASS!\n\n");
        } else if (oem_data->noise_test_result > 0) {
            test_result += 1;
            cts_info(" FAIL!\n");
        } else {
            test_result += 1;
            cts_info(" ERROR(%d)!\n\n", oem_data->noise_test_result);
        }
    }

    //Open Test
    if (oem_data->test_open) {
        cts_info("Open Test");
        if (oem_data->open_test_result == 0) {
            cts_info(" PASS!\n\n");
        } else if (oem_data->open_test_result > 0) {
             test_result += 1;
            cts_info(" FAIL!\n");
        } else {
             test_result += 1;
            cts_info(" ERROR(%d)!\n\n", oem_data->open_test_result);
        }
    }

	//Short Test
    if (oem_data->test_short) {
        cts_info("Short Test");
        if (oem_data->short_test_result == 0) {
            cts_info(" PASS!\n\n");
        } else if (oem_data->short_test_result > 0) {
            test_result += 1;
            cts_info(" FAIL!\n");
        } else {
             test_result += 1;
            cts_info(" ERROR(%d)!\n\n", oem_data->short_test_result);
        }
    }

	//Compensate-Cap Test
    if (oem_data->test_comp_cap) {
        cts_info("Compensate-Cap Test");
        if (oem_data->comp_cap_test_result == 0) {
            cts_info(" PASS!\n\n");
        } else if (oem_data->comp_cap_test_result > 0) {
           test_result += 1;
            cts_info(" FAIL!\n");
        } else {
           test_result += 1;
            cts_info(" ERROR(%d)!\n\n", oem_data->comp_cap_test_result);
        }
    }

	cts_info("%d error(s). %s\n", test_result, test_result ? "" : "All test passed.");
	seq_printf(m, "%d error(s). %s\n", test_result, test_result ? "" : "All test passed.");

    free_baseline_test_data_mem(oem_data);
    return 0;
}

const struct seq_operations selftest_seq_ops = {
    .start  = selftest_seq_start,
    .next   = selftest_seq_next,
    .stop   = selftest_seq_stop,
    .show   = selftest_seq_show,
};

static int32_t selftest_proc_open(struct inode *inode, struct file *file)
{
    struct chipone_ts_data *cts_data = pde_data(inode);
    struct cts_oem_data *oem_data = NULL;
    int ret;

    if (cts_data == NULL) {
        cts_err("Open selftest proc with cts_data = NULL");
        return -EFAULT;
    }

    oem_data = cts_data->vendor_data;//oem_data
    if (oem_data == NULL) {
        cts_err("Open selftest proc with oem_data = NULL");
        return -EFAULT;
    }

    cts_info("Open '/proc/" OEM_SELFTEST_PROC_FILENAME "'");

	/* Create the directory for mp_test result */
    if ((cts_dev_mkdir(OEM_TPTEST_REPORT, S_IRUGO | S_IWUSR)) != 0){
		cts_err("Failed to create directory for factory test\n");
	}
	if (strncmp("lcd_icnl9911c_hkc_mipi_hd", lcd_name, sizeof("lcd_icnl9911c_hkc_mipi_hd")) == 0){
	    if (!oem_data->test_config_from_dt_has_parsed) {
	        ret = parse_selftest_dt(oem_data, cts_data->device->of_node);
	        if (ret) {
	            cts_err("Parse selftest dt failed %d", ret);
	            return ret;
	        }
	    }
	} else if (strncmp("lcd_icnl9911c_auo_mipi_hd", lcd_name, sizeof("lcd_icnl9911c_auo_mipi_hd")) == 0){
			if (!oem_data->test_config_from_dt_has_parsed) {
				ret = auo_parse_selftest_dt(oem_data, cts_data->device->of_node);
				if (ret) {
					cts_err("Parse selftest dt failed %d", ret);
					return ret;
				}
			}
	} else {
			cts_err("nothing match lcd name\n");
			return 0;
	}

    print_selftest_config(oem_data);

    ret = alloc_sleftest_data_mem(oem_data,
        cts_data->cts_dev.fwdata.rows * cts_data->cts_dev.fwdata.cols);
    if (ret) {
        cts_err("Alloc test data mem failed");
        return ret;
    }

    do_selftest(oem_data);

    ret = seq_open(file, &selftest_seq_ops);
    if (ret) {
        cts_err("Open selftest seq file failed %d", ret);
        return ret;
    }

    ((struct seq_file *)file->private_data)->private = cts_data;

    return 0;
}

static const struct proc_ops proc_baseline_test_fops = {
    .proc_open = selftest_proc_open,
    .proc_read = seq_read,
    .proc_release = seq_release,
};

#ifdef CONFIG_CTS_GESTURE_TEST
int prepare_black_test(struct cts_device *cts_dev) {
    int ret;
	u8 buf;

    cts_info("Prepare black test");

	cts_plat_reset_device(cts_dev->pdata);

    ret = cts_set_dev_esd_protection(cts_dev, false);
    if (ret) {
        cts_err("Disable firmware ESD protection failed %d(%s)",
            ret, cts_strerror(ret));
        return ret;
    }
    ret = disable_fw_monitor_mode(cts_dev);
    if (ret) {
        cts_err("Disable firmware monitor mode failed %d(%s)",
            ret, cts_strerror(ret));
        return ret;
    }

	/*Disable GSTR ONLY FS Switch*/
	ret = cts_fw_reg_readb(cts_dev, CTS_DEVICE_FW_REG_GSTR_ONLY_FS_EN, &buf);
	if (ret) {
		cts_err("Get GSTR ONLY FS EN failed %d(%s)", ret, cts_strerror(ret));
		return ret;
	}
	ret = cts_fw_reg_writeb(cts_dev, CTS_DEVICE_FW_REG_GSTR_ONLY_FS_EN, (buf & 0xFB));
	if (ret) {
        cts_err("Disable GSTR ONLY FS failed %d(%s)", ret, cts_strerror(ret));
		return ret;
	}

	/*Enable GSTR DATA DBG*/
	ret = cts_fw_reg_readb(cts_dev, CTS_DEVICE_FW_REG_GSTR_DATA_DBG_EN, &buf);
	if (ret) {
		cts_err("get GSTR DATA DBG EN failed %d(%s)", ret, cts_strerror(ret));
		return ret;
	}

	ret = cts_fw_reg_writeb(cts_dev, CTS_DEVICE_FW_REG_GSTR_DATA_DBG_EN, (buf | BIT(6)));
	if (ret) {
        cts_err("Enable GSTR DATA DBG failed %d(%s)", ret, cts_strerror(ret));
		return ret;
	}

	ret = set_fw_work_mode(cts_dev, CTS_FIRMWARE_WORK_MODE_GSTR_DBG);
	cts_info("set_fw_work_mode ok");
	if (ret) {
		cts_err("Set firmware work mode to WORK_MODE_GSTR_DBG failed %d(%s)", ret, cts_strerror(ret));
		return ret;
	}
	ret = cts_fw_reg_readb(cts_dev, CTS_DEVICE_FW_REG_POWER_MODE, &buf);
	cts_info("cts_fw_reg_readb ok");
	if (ret) {
		cts_err("get POWER MODE failed %d(%s)", ret, cts_strerror(ret));
		return ret;
	}
    return 0;
}

static int save_black_selftest_data_to_file(struct cts_oem_data *oem_data)
{
    int rows, cols;
    int ret;

    cts_info("Save black selftest data to file");

    rows  = oem_data->cts_data->cts_dev.fwdata.rows;
    cols  = oem_data->cts_data->cts_dev.fwdata.cols;

    ret = dump_tsdata_to_csv_file(GESTURE_RAWDATA_TEST_DATA_FILENAME,
		O_RDWR | O_CREAT | O_TRUNC, oem_data->gesture_rawdata_test_data,
		oem_data->gesture_rawdata_test_frames, rows, cols);
    if (ret < 0) {
        cts_err("Dump gesture rawdata test data to file failed");
		goto tsdata_to_csv_file;
    }

    ret = dump_tsdata_to_csv_file(GESTURE_LP_RAWDATA_TEST_DATA_FILENAME,
		O_RDWR | O_CREAT | O_TRUNC, oem_data->gesture_lp_rawdata_test_data,
		oem_data->gesture_lp_rawdata_test_frames, rows / 8, cols);
    if (ret < 0) {
        cts_err("Dump gesture lp rawdata test data to file failed");
		goto tsdata_to_csv_file;
    }

    ret = dump_tsdata_to_csv_file(GESTURE_NOISE_TEST_DATA_FILENAME,
		O_RDWR | O_CREAT | O_TRUNC, oem_data->gesture_noise_test_data,
		oem_data->gesture_noise_test_frames + 1, rows, cols);
    if (ret < 0) {
        cts_err("Dump gesture noise test data to file failed");
		goto tsdata_to_csv_file;
    }

    ret = dump_tsdata_to_csv_file(GESTURE_LP_NOISE_TEST_DATA_FILENAME,
		O_RDWR | O_CREAT | O_TRUNC, oem_data->gesture_lp_noise_test_data,
		oem_data->gesture_lp_noise_test_frames + 1, rows / 8, cols);
    if (ret < 0) {
        cts_err("Dump gesture lp noise test data to file failed");
		goto tsdata_to_csv_file;
    }

	return 0;
tsdata_to_csv_file:

    return ret;
}

static void do_black_selftest(struct cts_device *cts_dev, struct cts_oem_data *oem_data)
{
    struct cts_test_param test_param;
	struct cts_rawdata_test_priv_param gesture_rawdata_test_priv_param;
	struct cts_rawdata_test_priv_param gesture_noise_test_priv_param;
	struct cts_rawdata_test_priv_param gesture_lp_rawdata_test_priv_param;
	struct cts_rawdata_test_priv_param gesture_lp_noise_test_priv_param;

    cts_info("Do black selftest");

	gesture_rawdata_test_priv_param.frames = oem_data->gesture_rawdata_test_frames;
	gesture_rawdata_test_priv_param.work_mode = 1;
	memset(&test_param, 0, sizeof(test_param));
	oem_data->gesture_rawdata_test_data_wr_size = 0;
	test_param.test_item = CTS_TEST_GESTURE_RAWDATA;
	test_param.flags =
		CTS_TEST_FLAG_VALIDATE_DATA |
		CTS_TEST_FLAG_VALIDATE_MIN |
		CTS_TEST_FLAG_VALIDATE_MAX |
		//CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE|
		CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
		CTS_TEST_FLAG_DUMP_TEST_DATA_TO_USERSPACE;
	test_param.min = &oem_data->gesture_rawdata_min;
	test_param.max = &oem_data->gesture_rawdata_max;
	test_param.test_data_buf = oem_data->gesture_rawdata_test_data;
	test_param.test_data_buf_size = oem_data->gesture_rawdata_test_data_buff_size;
	test_param.test_data_wr_size = &oem_data->gesture_rawdata_test_data_wr_size;

	test_param.priv_param = &gesture_rawdata_test_priv_param;
	test_param.priv_param_size = sizeof(gesture_rawdata_test_priv_param);

	oem_data->gesture_rawdata_test_result =
		cts_test_gesture_rawdata(cts_dev, &test_param);
	if (oem_data->gesture_rawdata_test_result) {
        oem_data->blackscreen_test_reault += 1;
		cts_err("Test gesture rawdata failed %d",oem_data->gesture_rawdata_test_result);
	}

	gesture_noise_test_priv_param.frames = oem_data->gesture_noise_test_frames;
	gesture_noise_test_priv_param.work_mode = 1;
	memset(&test_param, 0, sizeof(test_param));
	oem_data->gesture_noise_test_data_wr_size = 0;
	test_param.test_item = CTS_TEST_GESTURE_NOISE;
	test_param.flags =
		CTS_TEST_FLAG_VALIDATE_DATA |
		CTS_TEST_FLAG_VALIDATE_MAX |
		//CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE|
		CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
		CTS_TEST_FLAG_DUMP_TEST_DATA_TO_USERSPACE;
	test_param.max = &oem_data->gesture_noise_max;
	test_param.test_data_buf = oem_data->gesture_noise_test_data;
	test_param.test_data_buf_size = oem_data->gesture_noise_test_data_buff_size;
	test_param.test_data_wr_size = &oem_data->gesture_noise_test_data_wr_size;

	test_param.priv_param = &gesture_noise_test_priv_param;
	test_param.priv_param_size = sizeof(gesture_noise_test_priv_param);

	oem_data->gesture_noise_test_result =
		cts_test_gesture_noise(cts_dev, &test_param);
	if (oem_data->gesture_noise_test_result) {
		oem_data->blackscreen_test_reault += 1;
		cts_err("Test gesture noise failed %d",	oem_data->gesture_noise_test_result);
	}

	cts_dev->fwdata.rows = cts_dev->fwdata.rows / 8;
	gesture_lp_rawdata_test_priv_param.frames = oem_data->gesture_lp_rawdata_test_frames;
	gesture_lp_rawdata_test_priv_param.work_mode = 0;
	memset(&test_param, 0, sizeof(test_param));
	oem_data->gesture_lp_rawdata_test_data_wr_size = 0;
	test_param.test_item = CTS_TEST_GESTURE_LP_RAWDATA;
	test_param.flags =
		CTS_TEST_FLAG_VALIDATE_DATA |
		CTS_TEST_FLAG_VALIDATE_MIN |
		CTS_TEST_FLAG_VALIDATE_MAX |
		//CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE|
		CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
		CTS_TEST_FLAG_DUMP_TEST_DATA_TO_USERSPACE;
	test_param.min = &oem_data->gesture_lp_rawdata_min;
	test_param.max = &oem_data->gesture_lp_rawdata_max;
	test_param.test_data_buf = oem_data->gesture_lp_rawdata_test_data;
	test_param.test_data_buf_size = oem_data->gesture_lp_rawdata_test_data_buff_size;
	test_param.test_data_wr_size = &oem_data->gesture_lp_rawdata_test_data_wr_size;

	test_param.priv_param = &gesture_lp_rawdata_test_priv_param;
	test_param.priv_param_size = sizeof(gesture_lp_rawdata_test_priv_param);

	oem_data->gesture_lp_rawdata_test_result = cts_test_gesture_rawdata(cts_dev, &test_param);
	if (oem_data->gesture_lp_rawdata_test_result) {
		oem_data->blackscreen_test_reault += 1;
		cts_err("Test gesture lp rawdata failed %d", oem_data->gesture_lp_rawdata_test_result);
	}

	gesture_lp_noise_test_priv_param.frames = oem_data->gesture_lp_noise_test_frames;
	gesture_lp_noise_test_priv_param.work_mode = 0;
	memset(&test_param, 0, sizeof(test_param));
	oem_data->gesture_lp_noise_test_data_wr_size = 0;
	test_param.test_item = CTS_TEST_GESTURE_LP_NOISE;
	test_param.flags =
		CTS_TEST_FLAG_VALIDATE_DATA |
		CTS_TEST_FLAG_VALIDATE_MAX |
		//CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE|
		CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
		CTS_TEST_FLAG_DUMP_TEST_DATA_TO_USERSPACE;
	test_param.max = &oem_data->gesture_lp_noise_max;
	test_param.test_data_buf = oem_data->gesture_lp_noise_test_data;
	test_param.test_data_buf_size = oem_data->gesture_lp_noise_test_data_buff_size;
	test_param.test_data_wr_size = &oem_data->gesture_lp_noise_test_data_wr_size;

	test_param.priv_param = &gesture_lp_noise_test_priv_param;
	test_param.priv_param_size = sizeof(gesture_lp_noise_test_priv_param);

	oem_data->gesture_lp_noise_test_result = cts_test_gesture_noise(cts_dev, &test_param);
	if (oem_data->gesture_lp_noise_test_result) {
		oem_data->blackscreen_test_reault += 1;
		cts_err("Test gesture lp noise failed %d", oem_data->gesture_lp_noise_test_result);
	}
	cts_dev->fwdata.rows = cts_dev->fwdata.rows * 8;
}

static ssize_t black_selftest_proc_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos)
{
    int ret = 0;
	int len = 0;
	int msg_size = 32;
	char msg[msg_size];
	struct cts_device *cts_dev = NULL;
    struct chipone_ts_data *cts_data = NULL;
	struct cts_oem_data *oem_data = NULL;

	cts_info("%s %ld %lld\n", __func__, count, *ppos);

    cts_data = pde_data(file_inode(file));
	if (cts_data == NULL) {
		cts_err("Open black selftest proc with cts_data = NULL");
		return -EFAULT;
	}

	oem_data = cts_data->vendor_data;
	if (oem_data == NULL) {
		cts_err("Open black selftest proc with oem_data = NULL");
		return -EFAULT;
	}

    if (buffer == NULL) {
        cts_err("Read proc file '"PROC_BLACK_SCREEN_TEST_FILEPATH
                "' with buffer = NULL");
        return -EINVAL;
    }

    if(*ppos != 0 ) {
		cts_info("*ppos != 0, return.\n");
		return 0;
	}
    cts_dev = &cts_data->cts_dev;
	cts_info("Open '/proc/touchpanel/" OEM_SELFTEST_BLACK_FILENAME "'");
	//msleep(500);


    //cts_plat_reset_device(cts_dev->pdata);

	if (cts_data->pdata->irq_wake_enabled) {
		ret = cts_plat_disable_irq_wake(cts_data->pdata);
		if (ret) {
			cts_warn("Disable IRQ wake failed %d(%s)", ret, cts_strerror(ret));
			return ret;
		}
	}

    if ((ret = cts_plat_disable_irq(cts_dev->pdata)) < 0) {
        cts_err("Disable IRQ failed %d(%s)", ret, cts_strerror(ret));
        return ret;
    }

	ret = prepare_black_test(cts_dev);
	if (ret) {
		cts_err("Prepare test failed %d", ret);
		return ret;
	}
	if (strncmp("lcd_icnl9911c_hkc_mipi_hd", lcd_name, sizeof("lcd_icnl9911c_hkc_mipi_hd")) == 0){
		if (!oem_data->black_test_config_from_dt_has_parsed) {
			ret = parse_black_selftest_dt(oem_data, cts_data->device->of_node);
			if (ret) {
				cts_err(" HKC Parse selftest dt failed %d", ret);
				return ret;
			}
		}
	} else if (strncmp("lcd_icnl9911c_auo_mipi_hd", lcd_name, sizeof("lcd_icnl9911c_auo_mipi_hd")) == 0){
			if (!oem_data->black_test_config_from_dt_has_parsed) {
			ret = auo_parse_black_selftest_dt(oem_data, cts_data->device->of_node);
			if (ret) {
				cts_err("AUO Parse selftest dt failed %d", ret);
				return ret;
			}
		}
	} else {
        cts_err("nothing match lcd name\n");
        return 0;
	}

	ret = alloc_black_selftest_data_mem(oem_data, cts_data->cts_dev.fwdata.rows * cts_data->cts_dev.fwdata.cols);
	if (ret) {
		cts_err("Alloc black test data mem failed");
		return ret;
	}

    /* Create the directory for mp_test result */
    if ((cts_dev_mkdir(OEM_TPTEST_REPORT, S_IRUGO | S_IWUSR)) != 0){
		cts_err("already have or Failed to create directory for factory test\n");
	}

    oem_data->blackscreen_test_reault = 0;
    do_black_selftest(cts_dev, oem_data);
    ret = save_black_selftest_data_to_file(oem_data);
    if (ret) {
        cts_err("Save black selftest data to file failed %d", ret);
    }

	cts_info("black_selftest result is %s\n", ( oem_data->blackscreen_test_reault > 0) ? "FAIL" : "PASS");
    len = snprintf(msg, msg_size, "%d error(s) %s\n", oem_data->blackscreen_test_reault, (oem_data->blackscreen_test_reault > 0) ? "test failed" : "All test passed");	

	if (copy_to_user(buffer, msg, len)) {
	   cts_info("black test result msg copy to user fail.\n");
	   ret = -EFAULT;
	}
    cts_info("copy userspace msg len is %d\n", len);

    cts_plat_reset_device(cts_dev->pdata);


	/* Enable IRQ wake if gesture wakeup enabled */
	if (cts_is_gesture_wakeup_enabled(&cts_data->cts_dev)) {
		ret = cts_send_command(cts_dev,
			cts_dev->rtdata.gesture_wakeup_enabled ?
				CTS_CMD_SUSPEND_WITH_GESTURE : CTS_CMD_SUSPEND);
		if (ret){
			cts_err("Blacktest Send GSTR cmd failed %d(%s)",
				ret, cts_strerror(ret));
		}
	    	
		ret = cts_plat_enable_irq_wake(cts_data->pdata);
		if (ret) {
			cts_err("Enable IRQ wake failed %d(%s)",
			ret, cts_strerror(ret));
			return ret;
		}
		
		ret = cts_plat_enable_irq(cts_data->pdata);
		if (ret){
			cts_err("Enable IRQ failed %d(%s)",
				ret, cts_strerror(ret));
			return ret;
		}
	}


	*ppos += len;
	free_black_screen_test_data_mem(oem_data);
	g_black_test = 0;

	return len;
}

static ssize_t black_selftest_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int value = 0;
	int ret = 0;
    char buf[4] = {0};

	if (buffer != NULL) {
       ret = copy_from_user(buf, buffer, count);
       if (ret < 0) {
			cts_err("%s: read proc input error.\n", __func__);
			return -1;
		}
	}

    ret = sscanf(buf, "%d", &value);
    if (ret < 0) {
		cts_err("%s:sscanf failed\n", __func__);
	}

    cts_info(" value is %d\n",value);
    g_black_test = value;

    return count;
}

static const struct proc_ops proc_black_screen_test_fops = {
   .proc_read = black_selftest_proc_read,
   .proc_write = black_selftest_proc_write,
};
#endif

static ssize_t proc_coordinate_read(struct file *filp,
    char __user *buffer, size_t size, loff_t *ppos)
{
    struct chipone_ts_data *cts_data = NULL;
    struct cts_oem_data *vdata = NULL;
    int ret;

    cts_data = (struct chipone_ts_data *)filp->private_data;
    if (cts_data == NULL) {
        cts_err("Read proc file '"PROC_COORDINATE_FILEPATH
                "' with cts_data = NULL");
        return -EFAULT;
    }

    vdata = cts_data->vendor_data;
    if (vdata == NULL) {
        cts_err("Read proc file '"PROC_COORDINATE_FILEPATH
                "' with vdata = NULL");
        return -EFAULT;
    }

    if (buffer == NULL) {
        cts_err("Read proc file '"PROC_COORDINATE_FILEPATH
                "' with buffer = NULL");
        return -EINVAL;
    }

	if (*ppos != 0) {
	    return 0;
    }

    cts_info("Read '"PROC_COORDINATE_FILEPATH"'");

    ret = copy_to_user(buffer, vdata->coordinate, vdata->coordinate_len);
    if (ret) {
        cts_err("Copy data to user buffer failed %d(%s)",
            ret, cts_strerror(ret));
        return 0;
    }

    return vdata->coordinate_len;
}
static const struct proc_ops proc_coordinate_fops = {
    .proc_open = proc_simple_open,
    .proc_read = proc_coordinate_read,
};

static ssize_t proc_debug_level_read(struct file *filp,
    char __user *buffer, size_t size, loff_t *ppos)
{
    struct chipone_ts_data *cts_data = NULL;
    struct cts_oem_data *vdata = NULL;
    char debug_level_str[32];
    int ret, len;

    cts_data = (struct chipone_ts_data *)filp->private_data;
    if (cts_data == NULL) {
        cts_err("Read proc file '"PROC_DEBUG_LEVEL_FILEPATH
                "' with cts_data = NULL");
        return -EFAULT;
    }

    vdata = cts_data->vendor_data;
    if (vdata == NULL) {
        cts_err("Read proc file '"PROC_DEBUG_LEVEL_FILEPATH
                "' with vdata = NULL");
        return -EFAULT;
    }

    if (buffer == NULL) {
        cts_err("Read proc file '"PROC_DEBUG_LEVEL_FILEPATH
                "' with buffer = NULL");
        return -EINVAL;
    }

	if (*ppos != 0) {
	    return 0;
    }

    cts_info("Read '"PROC_DEBUG_LEVEL_FILEPATH"'");

	len = scnprintf(debug_level_str, sizeof(debug_level_str),
	    "DEBUG_LEVEL = %d\n", vdata->debug_level);
	*ppos += len;

    ret = copy_to_user(buffer, debug_level_str, len);
	if (ret) {
        cts_err("Copy data to user buffer failed %d(%s)",
            ret, cts_strerror(ret));
		return 0;
    }

	return len;
}

static ssize_t proc_debug_level_write(struct file *filp,
    const char __user *buffer, size_t size, loff_t *ppos)
{
    struct chipone_ts_data *cts_data = NULL;
    struct cts_oem_data *vdata = NULL;
    int debug_level, ret;

    cts_data = (struct chipone_ts_data *)filp->private_data;
    if (cts_data == NULL) {
        cts_err("Write proc file '"PROC_DEBUG_LEVEL_FILEPATH
                "' with cts_data = NULL");
        return -EFAULT;
    }

    vdata = cts_data->vendor_data;
    if (vdata == NULL) {
        cts_err("Write proc file '"PROC_DEBUG_LEVEL_FILEPATH
                "' with vdata = NULL");
        return -EFAULT;
    }

    if (buffer == NULL) {
        cts_err("Write proc file '"PROC_DEBUG_LEVEL_FILEPATH
                "' with buffer = NULL");
        return -EINVAL;
    }

    cts_info("Write '"PROC_DEBUG_LEVEL_FILEPATH"' size: %zu", size);

	ret = kstrtoint_from_user(buffer, size, 0, &debug_level);
	if (ret) {
        cts_err("Write invalid DEBUG_LEVEL %d(%s)",
            ret, cts_strerror(ret));
        return -EINVAL;
	}

    vdata->debug_level = debug_level;
    cts_info("Set DEBUG_LEVEL = %d", debug_level);

	return size;
}
static const struct proc_ops proc_debug_level_fops = {
    .proc_open = proc_simple_open,
    .proc_read = proc_debug_level_read,
    .proc_write = proc_debug_level_write,
};

static ssize_t proc_double_tap_enable_read(struct file *filp,
    char __user *buffer, size_t size, loff_t *ppos)
{
    struct chipone_ts_data *cts_data = NULL;
    struct cts_oem_data *vdata = NULL;
    char double_tap_enable_str[32];
    int ret, len;

    cts_data = (struct chipone_ts_data *)filp->private_data;
    if (cts_data == NULL) {
        cts_err("Read proc file '"PROC_DOUBLE_TAP_ENABLE_FILEPATH
                "' with cts_data = NULL");
        return -EFAULT;
    }

    vdata = cts_data->vendor_data;
    if (vdata == NULL) {
        cts_err("Read proc file '"PROC_DOUBLE_TAP_ENABLE_FILEPATH
                "' with vdata = NULL");
        return -EFAULT;
    }

    if (buffer == NULL) {
        cts_err("Read proc file '"PROC_DOUBLE_TAP_ENABLE_FILEPATH
                "' with buffer = NULL");
        return -EINVAL;
    }

	if (*ppos != 0) {
	    return 0;
    }

    cts_info("Read '"PROC_DOUBLE_TAP_ENABLE_FILEPATH"'");

	len = scnprintf(double_tap_enable_str, sizeof(double_tap_enable_str),
	    "DOUBLE_TAP_ENABLE = %d\n", vdata->double_tap_enable);
	*ppos += len;

    ret = copy_to_user(buffer, double_tap_enable_str, len);
	if (ret) {
        cts_err("Copy data to user buffer failed %d(%s)",
            ret, cts_strerror(ret));
		return 0;
    }

	return len;
}

static ssize_t proc_double_tap_enable_write(struct file *filp,
    const char __user *buffer, size_t size, loff_t *ppos)
{
#if !defined(CFG_CTS_GESTURE)
    cts_err("Set DOUBLE_TAP_ENABLE while CFG_CTS_GESTURE not defined");
    return -EFAULT;
#else
	struct chipone_ts_data *cts_data = NULL;
    struct cts_oem_data *vdata = NULL;
    int double_tap_enable, ret;

    cts_data = (struct chipone_ts_data *)filp->private_data;
    if (cts_data == NULL) {
        cts_err("Write proc file '"PROC_DOUBLE_TAP_ENABLE_FILEPATH
                "' with cts_data = NULL");
        return -EFAULT;
    }

    vdata = cts_data->vendor_data;
    if (vdata == NULL) {
        cts_err("Write proc file '"PROC_DOUBLE_TAP_ENABLE_FILEPATH
                "' with vdata = NULL");
        return -EFAULT;
    }

    if (buffer == NULL) {
        cts_err("Write proc file '"PROC_DOUBLE_TAP_ENABLE_FILEPATH
                "' with buffer = NULL");
        return -EINVAL;
    }

    cts_info("Write '"PROC_DOUBLE_TAP_ENABLE_FILEPATH"' size: %zu", size);

    ret = kstrtoint_from_user(buffer, size, 0, &double_tap_enable);
    if (ret) {
        cts_err("Write invalid DOUBLE_TAP_ENABLE %d(%s)",
            ret, cts_strerror(ret));
        return -EINVAL;
    }

    vdata->double_tap_enable = double_tap_enable;
    cts_info("Set DOUBLE_TAP_ENABLE = %d", double_tap_enable);

    if (double_tap_enable) {
        cts_enable_gesture_wakeup(&cts_data->cts_dev);
    } else {
        cts_disable_gesture_wakeup(&cts_data->cts_dev);
    }

    return size;
#endif
}

static const struct proc_ops proc_double_tap_enable_fops = {
    .proc_open = proc_simple_open,
    .proc_read = proc_double_tap_enable_read,
	.proc_write = proc_double_tap_enable_write,
};

static ssize_t proc_game_switch_enable_read(struct file *filp,
    char __user *buffer, size_t size, loff_t *ppos)
{
    struct chipone_ts_data *cts_data = NULL;
    struct cts_oem_data *vdata = NULL;
    char game_switch_enable_str[32];
    int ret, len;

    cts_data = (struct chipone_ts_data *)filp->private_data;
    if (cts_data == NULL) {
        cts_err("Read proc file '"PROC_GAME_SWITCH_ENABLE_FILEPATH
                "' with cts_data = NULL");
        return -EFAULT;
    }

    vdata = cts_data->vendor_data;
    if (vdata == NULL) {
        cts_err("Read proc file '"PROC_GAME_SWITCH_ENABLE_FILEPATH
                "' with vdata = NULL");
        return -EFAULT;
    }

    if (buffer == NULL) {
        cts_err("Read proc file '"PROC_GAME_SWITCH_ENABLE_FILEPATH
                "' with buffer = NULL");
        return -EINVAL;
    }

	if (*ppos != 0) {
	    return 0;
    }

    cts_info("Read '"PROC_GAME_SWITCH_ENABLE_FILEPATH"'");

	len = scnprintf(game_switch_enable_str, sizeof(game_switch_enable_str),
	    "GAME_SWITCH_ENABLE = %d\n", vdata->game_switch_enable);
	*ppos += len;

    ret = copy_to_user(buffer, game_switch_enable_str, len);
	if (ret) {
        cts_err("Copy data to user buffer failed %d(%s)",
            ret, cts_strerror(ret));
		return 0;
    }

    return len;
}

static ssize_t proc_game_switch_enable_write(struct file *filp,
    const char __user *buffer, size_t size, loff_t *ppos)
{
    struct chipone_ts_data *cts_data = NULL;
    struct cts_oem_data *vdata = NULL;
    int game_switch_enable, ret;

    cts_data = (struct chipone_ts_data *)filp->private_data;
    if (cts_data == NULL) {
        cts_err("Write proc file '"PROC_GAME_SWITCH_ENABLE_FILEPATH
                "' with cts_data = NULL");
        return -EFAULT;
    }

    vdata = cts_data->vendor_data;
    if (vdata == NULL) {
        cts_err("Write proc file '"PROC_GAME_SWITCH_ENABLE_FILEPATH
                "' with vdata = NULL");
        return -EFAULT;
    }

    if (buffer == NULL) {
        cts_err("Write proc file '"PROC_GAME_SWITCH_ENABLE_FILEPATH
                "' with buffer = NULL");
        return -EINVAL;
    }

    cts_info("Write '"PROC_GAME_SWITCH_ENABLE_FILEPATH"' size: %zu", size);

    ret = kstrtoint_from_user(buffer, size, 0, &game_switch_enable);
    if (ret) {
        cts_err("Write invalid GAME_SWITCH_ENABLE %d(%s)",
            ret, cts_strerror(ret));
        return -EINVAL;
    }

    vdata->game_switch_enable = game_switch_enable;
    cts_info("Set GAME_SWITCH_ENABLE = %d", game_switch_enable);

    ret = cts_fw_reg_writeb(&cts_data->cts_dev, 0x086E,
        game_switch_enable ? 1 : 0);
    if (ret) {
        cts_err("Set dev game mode failed %d(%s)",
            ret, cts_strerror(ret));
        return -EINVAL;
    }

    return size;

}
static const struct proc_ops proc_game_switch_enable_fops = {
    .proc_open = proc_simple_open,
	.proc_read = proc_game_switch_enable_read,
	.proc_write = proc_game_switch_enable_write,
};

static ssize_t proc_incell_panel_read(struct file *filp,
    char __user *buffer, size_t size, loff_t *ppos)
{
    struct chipone_ts_data *cts_data = NULL;
    struct cts_oem_data *vdata = NULL;
    char incell_panel_str[32];
    int ret, len;

    cts_data = (struct chipone_ts_data *)filp->private_data;
    if (cts_data == NULL) {
        cts_err("Read proc file '"PROC_INCELL_PANEL_FILEPATH
                "' with cts_data = NULL");
        return -EFAULT;
    }

    vdata = cts_data->vendor_data;
    if (vdata == NULL) {
        cts_err("Read proc file '"PROC_INCELL_PANEL_FILEPATH
                "' with vdata = NULL");
        return -EFAULT;
    }

    if (buffer == NULL) {
        cts_err("Read proc file '"PROC_INCELL_PANEL_FILEPATH
                "' with buffer = NULL");
        return -EINVAL;
    }

	if (*ppos != 0) {
	    return 0;
    }

    cts_info("Read '"PROC_INCELL_PANEL_FILEPATH"'");

	len = scnprintf(incell_panel_str, sizeof(incell_panel_str),
	    "INCELL_PANEL = 1\n");
	*ppos += len;

    ret = copy_to_user(buffer, incell_panel_str, len);
	if (ret) {
        cts_err("Copy data to user buffer failed %d(%s)",
            ret, cts_strerror(ret));
		return 0;
    }

    return len;
}
static const struct proc_ops proc_incell_panel_fops = {
    .proc_open = proc_simple_open,
    .proc_read = proc_incell_panel_read,
};

static ssize_t proc_irq_depth_read(struct file *filp,
    char __user *buffer, size_t size, loff_t *ppos)
{
    struct chipone_ts_data *cts_data = NULL;
    struct irq_desc *desc;
    char irq_depth_str[128];
    int ret, len;

    cts_data = (struct chipone_ts_data *)filp->private_data;
    if (cts_data == NULL) {
        cts_err("Read proc file '"PROC_IRQ_DEPTH_FILEPATH
                "' with cts_data = NULL");
        return -EFAULT;
    }

    if (buffer == NULL) {
        cts_err("Read proc file '"PROC_IRQ_DEPTH_FILEPATH
                "' with buffer = NULL");
        return -EINVAL;
    }

	if (*ppos != 0) {
	    return 0;
    }

    cts_info("Read '"PROC_IRQ_DEPTH_FILEPATH"'");

    desc = irq_to_desc(cts_data->pdata->irq);
    if (desc == NULL) {
        len = scnprintf(irq_depth_str, sizeof(irq_depth_str),
            "IRQ: %d descriptor not found\n",
            cts_data->pdata->irq);
    } else {
        len = scnprintf(irq_depth_str, sizeof(irq_depth_str),
            "IRQ num: %d, depth: %u, "
            "count: %u, unhandled: %u, last unhandled eslape: %lu\n",
            cts_data->pdata->irq, desc->depth,
            desc->irq_count, desc->irqs_unhandled,
            desc->last_unhandled);
    }
	*ppos += len;

    ret = copy_to_user(buffer, irq_depth_str, len);
	if (ret) {
        cts_err("Copy data to user buffer failed %d(%s)",
            ret, cts_strerror(ret));
		return 0;
    }

    return len;
}
static const struct proc_ops proc_irq_depth_fops = {
    .proc_open = proc_simple_open,
    .proc_read = proc_irq_depth_read,
};

static ssize_t proc_oppo_register_info_read(struct file *filp,
    char __user *buffer, size_t size, loff_t *ppos)
{
    return 0;
}

/* echo r/w fw/hw addr len/val0 val1 val2 ... */
static ssize_t proc_oppo_register_info_write(struct file *filp,
    const char __user *buffer, size_t size, loff_t *ppos)
{
    int parse_arg(const char *buf, size_t count);

    parse_arg(buffer, size);

    return size;
}
static const struct proc_ops proc_oppo_register_info_fops = {
    .proc_open = proc_simple_open,
    .proc_read = proc_oppo_register_info_read,
	.proc_write = proc_oppo_register_info_write,
};

static ssize_t proc_oppo_tp_direction_read(struct file *filp,
    char __user *buffer, size_t size, loff_t *ppos)
{
    struct chipone_ts_data *cts_data = NULL;
    struct cts_oem_data *vdata = NULL;
    char oppo_tp_direction_str[32];
    int ret, len;

    cts_data = (struct chipone_ts_data *)filp->private_data;
    if (cts_data == NULL) {
        cts_err("Read proc file '"PROC_OPPO_TP_DIRECTION_FILEPATH
                "' with cts_data = NULL");
        return -EFAULT;
    }

    vdata = cts_data->vendor_data;
    if (vdata == NULL) {
        cts_err("Read proc file '"PROC_OPPO_TP_DIRECTION_FILEPATH
                "' with vdata = NULL");
        return -EFAULT;
    }

    if (buffer == NULL) {
        cts_err("Read proc file '"PROC_OPPO_TP_DIRECTION_FILEPATH
                "' with buffer = NULL");
        return -EINVAL;
    }

	if (*ppos != 0) {
	    return 0;
    }

    cts_info("Read '"PROC_OPPO_TP_DIRECTION_FILEPATH"'");

	len = scnprintf(oppo_tp_direction_str, sizeof(oppo_tp_direction_str),
	    "OPPO_TP_DIRECTION = %d\n", vdata->oppo_tp_direction);
	*ppos += len;

    ret = copy_to_user(buffer, oppo_tp_direction_str, len);
	if (ret) {
        cts_err("Copy data to user buffer failed %d(%s)",
            ret, cts_strerror(ret));
		return 0;
    }

	return len;
}

static ssize_t proc_oppo_tp_direction_write(struct file *filp,
    const char __user *buffer, size_t size, loff_t *ppos)
{
    struct chipone_ts_data *cts_data = NULL;
    struct cts_oem_data *vdata = NULL;
    int ret = 0;
    u32 oppo_tp_direction = 0;

    cts_data = (struct chipone_ts_data *)filp->private_data;
    if (cts_data == NULL) {
        cts_err("Write proc file '"PROC_OPPO_TP_DIRECTION_FILEPATH
                "' with cts_data = NULL");
        return -EFAULT;
    }

    vdata = cts_data->vendor_data;
    if (vdata == NULL) {
        cts_err("Write proc file '"PROC_OPPO_TP_DIRECTION_FILEPATH
                "' with vdata = NULL");
        return -EFAULT;
    }

    if (buffer == NULL) {
        cts_err("Write proc file '"PROC_OPPO_TP_DIRECTION_FILEPATH
                "' with buffer = NULL");
        return -EINVAL;
    }

    cts_info("Write '"PROC_OPPO_TP_DIRECTION_FILEPATH"' size: %zu", size);

	ret = kstrtoint_from_user(buffer, size, 0, &oppo_tp_direction);
	if (ret) {
        cts_err("Write invalid OPPO_TP_DIRECTION %d(%s)",
            ret, cts_strerror(ret));
        return -EINVAL;
	}

    switch (oppo_tp_direction) {
        case 0:
        case 1:
        case 2:
            ret = cts_fw_reg_writeb(&cts_data->cts_dev, 0x0801,
                (u8)oppo_tp_direction);
            if (ret) {
                cts_err("Write tp direction firmware failed %d(%d)",
                    ret, cts_strerror(ret));
                return ret;
            }
            break;
        default:
            cts_err("Write invalid OPPO_TP_DIRECTION %d",
                oppo_tp_direction);
            return -EINVAL;
    }


    vdata->oppo_tp_direction = oppo_tp_direction;
    cts_info("Set OPPO_TP_DIRECTION = %d", oppo_tp_direction);

	return size;
}
static const struct proc_ops proc_oppo_tp_direction_fops = {
    .proc_open = proc_simple_open,
	.proc_read = proc_oppo_tp_direction_read,
	.proc_write = proc_oppo_tp_direction_write,
};

static ssize_t proc_oppo_tp_limit_enable_read(struct file *filp,
    char __user *buffer, size_t size, loff_t *ppos)
{
    struct chipone_ts_data *cts_data = NULL;
    struct cts_oem_data *vdata = NULL;
    char oppo_tp_limit_enable_str[32];
    int ret, len;

    cts_data = (struct chipone_ts_data *)filp->private_data;
    if (cts_data == NULL) {
        cts_err("Read proc file '"PROC_OPPO_TP_LIMIT_ENABLE_FILEPATH
                "' with cts_data = NULL");
        return -EFAULT;
    }

    vdata = cts_data->vendor_data;
    if (vdata == NULL) {
        cts_err("Read proc file '"PROC_OPPO_TP_LIMIT_ENABLE_FILEPATH
                "' with vdata = NULL");
        return -EFAULT;
    }

    if (buffer == NULL) {
        cts_err("Read proc file '"PROC_OPPO_TP_LIMIT_ENABLE_FILEPATH
                "' with buffer = NULL");
        return -EINVAL;
    }

	if (*ppos != 0) {
	    return 0;
    }

    cts_info("Read '"PROC_OPPO_TP_LIMIT_ENABLE_FILEPATH"'");

	len = scnprintf(oppo_tp_limit_enable_str, sizeof(oppo_tp_limit_enable_str),
	    "OPPO_TP_LIMIT_ENABLE = %d\n", vdata->oppo_tp_limit_enable);
	*ppos += len;

    ret = copy_to_user(buffer, oppo_tp_limit_enable_str, len);
	if (ret) {
        cts_err("Copy data to user buffer failed %d(%s)",
            ret, cts_strerror(ret));
		return 0;
    }

	return len;
}

static ssize_t proc_oppo_tp_limit_enable_write(struct file *filp,
    const char __user *buffer, size_t size, loff_t *ppos)
{
    struct chipone_ts_data *cts_data = NULL;
    struct cts_oem_data *vdata = NULL;
    int oppo_tp_limit_enable, ret;

    cts_data = (struct chipone_ts_data *)filp->private_data;
    if (cts_data == NULL) {
        cts_err("Write proc file '"PROC_OPPO_TP_LIMIT_ENABLE_FILEPATH
                "' with cts_data = NULL");
        return -EFAULT;
    }

    vdata = cts_data->vendor_data;
    if (vdata == NULL) {
        cts_err("Write proc file '"PROC_OPPO_TP_LIMIT_ENABLE_FILEPATH
                "' with vdata = NULL");
        return -EFAULT;
    }

    if (buffer == NULL) {
        cts_err("Write proc file '"PROC_OPPO_TP_LIMIT_ENABLE_FILEPATH
                "' with buffer = NULL");
        return -EINVAL;
    }

    cts_info("Write '"PROC_OPPO_TP_LIMIT_ENABLE_FILEPATH"' size: %zu", size);

	ret = kstrtoint_from_user(buffer, size, 0, &oppo_tp_limit_enable);
	if (ret) {
        cts_err("Write invalid OPPO_TP_LIMIT_ENABLE %d(%s)",
            ret, cts_strerror(ret));
        return -EINVAL;
	}

    switch (oppo_tp_limit_enable) {
        case 0:
        case 1:
        case 2:
            ret = cts_fw_reg_writeb(&cts_data->cts_dev, 0x0801,
                (u8)oppo_tp_limit_enable);
            if (ret) {
                cts_err("Write tp limit to firmware failed %d(%d)",
                    ret, cts_strerror(ret));
                return ret;
            }
            break;
        default:
            cts_err("Write invalid OPPO_TP_LIMIT_ENABLE %d",
                oppo_tp_limit_enable);
            return -EINVAL;
    }

    vdata->oppo_tp_limit_enable = oppo_tp_limit_enable;
    cts_info("Set OPPO_TP_LIMIT_ENABLE = %d", oppo_tp_limit_enable);

	return size;
}

static const struct proc_ops proc_oppo_tp_limit_enable_fops =
{
    .proc_open = proc_simple_open,
    .proc_read = proc_oppo_tp_limit_enable_read,
    .proc_write = proc_oppo_tp_limit_enable_write,
};

static ssize_t proc_tp_fw_update_write(struct file *filp,
    const char __user *buffer, size_t size, loff_t *ppos)
{
    //TODO:
    return size;
}
static const struct proc_ops proc_tp_fw_update_fops = {
    .proc_open = proc_simple_open,
	.proc_write = proc_tp_fw_update_write,
};

static int baseline_seq_show(struct seq_file *m, void *v)
{
    struct chipone_ts_data *cts_data = NULL;
    struct cts_oem_data *vdata = NULL;

    cts_data = (struct chipone_ts_data *)m->private;
    if (cts_data == NULL) {
        cts_err("Baseline seq file private data = NULL");
        return -EFAULT;
    }

    vdata = cts_data->vendor_data;
    if (vdata == NULL) {
        cts_err("Baseline seq file with vdata = NULL");
        return -EFAULT;
    }

    if (vdata->rawdata == NULL) {
        cts_err("Baseline seq file with vdata->rawdata = NULL");
        return -EFAULT;
    }

    dump_tsdata_to_seq_file(m, vdata->rawdata,
        cts_data->cts_dev.fwdata.rows, cts_data->cts_dev.fwdata.cols);

    return 0;
}

const struct seq_operations baseline_seq_ops = {
    .start  = seq_start,
    .next   = seq_next,
    .stop   = seq_stop,
    .show   = baseline_seq_show,
};

static int proc_baseline_open(struct inode *inode, struct file *file)
{
    struct chipone_ts_data *cts_data = NULL;
    struct cts_device *cts_dev = NULL;
    struct cts_oem_data *vdata = NULL;
    int ret;

    cts_data = pde_data(inode);
    if (cts_data == NULL) {
        cts_err("Open proc file '"PROC_BASELINE_FILEPATH
                "' with cts_data = NULL");
        return -EFAULT;
    }

    cts_dev = &cts_data->cts_dev;

    vdata = cts_data->vendor_data;
    if (vdata == NULL) {
        cts_err("Open proc file '"PROC_BASELINE_FILEPATH
                "' with vdata = NULL");
        return -EFAULT;
    }

    cts_info("Open '"PROC_BASELINE_FILEPATH"'");

    vdata->rawdata = kzalloc(2 * cts_dev->fwdata.rows * cts_dev->fwdata.cols,
        GFP_KERNEL);
    if (vdata->rawdata == NULL) {
        cts_err("Alloc mem for rawdata failed");
        return -ENOMEM;
    }

    cts_lock_device(cts_dev);
    ret = cts_enable_get_rawdata(cts_dev);
    if (ret) {
        cts_err("Enable read touch data failed %d(%s)",
            ret, cts_strerror(ret));
        goto unlock_device_and_return;
    }

    ret = cts_send_command(cts_dev, CTS_CMD_QUIT_GESTURE_MONITOR);
    if (ret) {
        cts_err("Send cmd QUIT_GESTURE_MONITOR failed %d(%s)",
            ret, cts_strerror(ret));
        goto disable_get_touch_data_and_return;
    }
    msleep(50);

    ret = cts_get_rawdata(cts_dev, vdata->rawdata);
    if(ret) {
        cts_err("Get rawdata failed %d(%s)", ret, cts_strerror(ret));
        goto disable_get_touch_data_and_return;
    }
    ret = cts_disable_get_rawdata(cts_dev);
    if (ret) {
        cts_err("Disable read touch data failed %d(%s)",
            ret, cts_strerror(ret));
        // TODO: Try to recovery
    }
    cts_unlock_device(cts_dev);

    ret = seq_open(file, &baseline_seq_ops);
    if (ret) {
        cts_err("Open baseline seq file failed %d(%s)",
            ret, cts_strerror(ret));
        return ret;
    }

    ((struct seq_file *)file->private_data)->private = cts_data;

    return 0;

disable_get_touch_data_and_return: {
        int r = cts_disable_get_rawdata(cts_dev);
        if (r) {
            cts_err("Disable read touch data failed %d(%s)",
                ret, cts_strerror(ret));
        }
    }
unlock_device_and_return:
    cts_unlock_device(cts_dev);
    if (vdata->rawdata) {
        kfree(vdata->rawdata);
        vdata->rawdata = NULL;
    }

    return ret;
}

static int proc_baseline_release(struct inode *inode, struct file *file)
{
    struct chipone_ts_data *cts_data = NULL;
    struct cts_oem_data *vdata = NULL;

    cts_data = pde_data(inode);
    if (cts_data == NULL) {
        cts_err("Release proc file '"PROC_BASELINE_FILEPATH
                "' with cts_data = NULL");
        return -EFAULT;
    }

    vdata = cts_data->vendor_data;
    if (vdata == NULL) {
        cts_err("Release proc file '"PROC_BASELINE_FILEPATH
                "' with vdata = NULL");
        return -EFAULT;
    }

    if(vdata->rawdata) {
        kfree(vdata->rawdata);
        vdata->rawdata = NULL;
    }

	return 0;
}

static const struct proc_ops proc_baseline_fops = {
    .proc_open = proc_baseline_open,
    .proc_release = proc_baseline_release,
    .proc_read = seq_read,
    .proc_release = seq_release,
};


static ssize_t proc_data_limit_read(struct file *filp,
    char __user *buffer, size_t size, loff_t *ppos)
{
    return 0;
}
static const struct proc_ops proc_data_limit_fops = {
    .proc_open = proc_simple_open,
	.proc_read = proc_data_limit_read,
};

static int delta_seq_show(struct seq_file *m, void *v)
{
    struct chipone_ts_data *cts_data = NULL;
    struct cts_oem_data *vdata = NULL;

    cts_data = (struct chipone_ts_data *)m->private;
    if (cts_data == NULL) {
        cts_err("Delta seq file private data = NULL");
        return -EFAULT;
    }

    vdata = cts_data->vendor_data;
    if (vdata == NULL) {
        cts_err("Delta seq file with vdata = NULL");
        return -EFAULT;
    }

    if (vdata->delta == NULL) {
        cts_err("Delta seq file with vdata->delta = NULL");
        return -EFAULT;
    }

    dump_tsdata_to_seq_file(m, vdata->delta,
        cts_data->cts_dev.fwdata.rows, cts_data->cts_dev.fwdata.cols);

    return 0;
}

const struct seq_operations delta_seq_ops = {
    .start  = seq_start,
    .next   = seq_next,
    .stop   = seq_stop,
    .show   = delta_seq_show,
};

static int proc_delta_open(struct inode *inode, struct file *file)
{
    struct chipone_ts_data *cts_data = NULL;
    struct cts_device *cts_dev = NULL;
    struct cts_oem_data *vdata = NULL;
    int ret;

    cts_data = pde_data(inode);
    if (cts_data == NULL) {
        cts_err("Open proc file '"PROC_DELTA_FILEPATH
                "' with cts_data = NULL");
        return -EFAULT;
    }

    cts_dev = &cts_data->cts_dev;

    vdata = cts_data->vendor_data;
    if (vdata == NULL) {
        cts_err("Open proc file '"PROC_DELTA_FILEPATH
                "' with vdata = NULL");
        return -EFAULT;
    }

    cts_info("Open '"PROC_DELTA_FILEPATH"'");

    vdata->delta = kzalloc(2 * cts_dev->fwdata.rows * cts_dev->fwdata.cols,
        GFP_KERNEL);
    if (vdata->delta == NULL) {
        cts_err("Alloc mem for delta failed");
        return -ENOMEM;
    }

    cts_lock_device(cts_dev);
    ret = cts_enable_get_rawdata(cts_dev);
    if (ret) {
        cts_err("Enable read touch data failed %d(%s)",
            ret, cts_strerror(ret));
        goto unlock_device_and_return;
    }

    ret = cts_send_command(cts_dev, CTS_CMD_QUIT_GESTURE_MONITOR);
    if (ret) {
        cts_err("Send cmd QUIT_GESTURE_MONITOR failed %d(%s)",
            ret, cts_strerror(ret));
        goto disable_get_touch_data_and_return;
    }
    msleep(50);

    ret = cts_get_diffdata(cts_dev, vdata->delta);
    if(ret) {
        cts_err("Get diffdata failed %d(%s)", ret, cts_strerror(ret));
        goto disable_get_touch_data_and_return;
    }
    ret = cts_disable_get_rawdata(cts_dev);
    if (ret) {
        cts_err("Disable read touch data failed %d(%s)",
            ret, cts_strerror(ret));
        // TODO: Try to recovery
    }
    cts_unlock_device(cts_dev);

    ret = seq_open(file, &delta_seq_ops);
    if (ret) {
        cts_err("Open delta seq file failed %d(%s)",
            ret, cts_strerror(ret));
        return ret;
    }

    ((struct seq_file *)file->private_data)->private = cts_data;

    return 0;

disable_get_touch_data_and_return: {
        int r = cts_disable_get_rawdata(cts_dev);
        if (r) {
            cts_err("Disable read touch data failed %d(%s)",
                ret, cts_strerror(ret));
        }
    }
unlock_device_and_return:
    cts_unlock_device(cts_dev);
    if (vdata->delta) {
        kfree(vdata->delta);
        vdata->delta = NULL;
    }

    return ret;
}

static int proc_delta_release(struct inode *inode, struct file *file)
{
    struct chipone_ts_data *cts_data = NULL;
    struct cts_oem_data *vdata = NULL;

    cts_data = pde_data(inode);
    if (cts_data == NULL) {
        cts_err("Release proc file '"PROC_DELTA_FILEPATH
                "' with cts_data = NULL");
        return -EFAULT;
    }

    vdata = cts_data->vendor_data;
    if (vdata == NULL) {
        cts_err("Release proc file '"PROC_DELTA_FILEPATH
                "' with vdata = NULL");
        return -EFAULT;
    }

    if (vdata->delta) {
        kfree(vdata->delta);
        vdata->delta = NULL;
    }

	return 0;
}

static const struct proc_ops proc_delta_fops = {
    .proc_open = proc_delta_open,
    .proc_release = proc_delta_release,
    .proc_read = seq_read,
    .proc_release = seq_release,
};

static ssize_t proc_main_register_read(struct file *filp,
    char __user *buffer, size_t size, loff_t *ppos)
{
    struct chipone_ts_data *cts_data = NULL;
    char main_register_str[32];
    int ret, len;

    cts_data = (struct chipone_ts_data *)filp->private_data;
    if (cts_data == NULL) {
        cts_err("Read proc file '"PROC_MAIN_REGISTER_FILEPATH
                "' with cts_data = NULL");
        return -EFAULT;
    }

    if (buffer == NULL) {
        cts_err("Read proc file '"PROC_MAIN_REGISTER_FILEPATH
                "' with buffer = NULL");
        return -EINVAL;
    }

	if (*ppos != 0) {
	    return 0;
    }

    cts_info("Read '"PROC_MAIN_REGISTER_FILEPATH"'");

	len = scnprintf(main_register_str, sizeof(main_register_str),
	    "FW_VERSION = %04x\n", cts_data->cts_dev.fwdata.version);
	*ppos += len;

    ret = copy_to_user(buffer, main_register_str, len);
	if (ret) {
        cts_err("Copy data to user buffer failed %d(%s)",
            ret, cts_strerror(ret));
		return 0;
    }

	return len;
}
static const struct proc_ops proc_main_register_fops = {
	.proc_open = proc_simple_open,
	.proc_read = proc_main_register_read,
};


struct proc_file_create_data {
    const char *filename;
    const char *filepath;
    const struct proc_ops *fops;
};

struct proc_dir_create_data {
    const char *dirname;
    const char *dirpath;
    int nfiles;
    const struct proc_file_create_data *files;
};

static const struct proc_file_create_data proc_dir_debuginfo_files_create_data[] = {
    {
        .filename = PROC_BASELINE_FILENAME,
        .filepath = PROC_BASELINE_FILEPATH,
        .fops = &proc_baseline_fops,
    },
    {
        .filename = PROC_DATA_LIMIT_FILENAME,
        .filepath = PROC_DATA_LIMIT_FILEPATH,
        .fops = &proc_data_limit_fops,
    },
    {
        .filename = PROC_DELTA_FILENAME,
        .filepath = PROC_DELTA_FILEPATH,
        .fops = &proc_delta_fops,
    },
    {
        .filename = PROC_MAIN_REGISTER_FILENAME,
        .filepath = PROC_MAIN_REGISTER_FILEPATH,
        .fops = &proc_main_register_fops,
    },
};

static const struct proc_dir_create_data proc_dir_debuginfo_create_data = {
    .dirname = PROC_DEBUG_INFO_DIR_NAME,
    .dirpath = PROC_DEBUG_INFO_DIR_PATH,
    .nfiles = ARRAY_SIZE(proc_dir_debuginfo_files_create_data),
    .files = proc_dir_debuginfo_files_create_data,
};

static const struct proc_file_create_data proc_dir_touchpanel_files_create_data [] = {
    {
        .filename = PROC_BASELINE_TEST_FILENAME,
        .filepath = PROC_BASELINE_TEST_FILEPATH,
        .fops = &proc_baseline_test_fops,
    },
#ifdef CONFIG_CTS_GESTURE_TEST
    {
        .filename = PROC_BLACK_SCREEN_TEST_FILENAME,
        .filepath = PROC_BLACK_SCREEN_TEST_FILEPATH,
        .fops = &proc_black_screen_test_fops,
    },
#endif
    {
        .filename = PROC_COORDINATE_FILENAME,
        .filepath = PROC_COORDINATE_FILEPATH,
        .fops = &proc_coordinate_fops,
    },
    {
        .filename = PROC_DEBUG_LEVEL_FILENAME,
        .filepath = PROC_DEBUG_LEVEL_FILEPATH,
        .fops = &proc_debug_level_fops,
    },
    {
        .filename = PROC_DOUBLE_TAP_ENABLE_FILENAME,
        .filepath = PROC_DOUBLE_TAP_ENABLE_FILEPATH,
        .fops = &proc_double_tap_enable_fops,
    },
    {
        .filename = PROC_GAME_SWITCH_ENABLE_FILENAME,
        .filepath = PROC_GAME_SWITCH_ENABLE_FILEPATH,
        .fops = &proc_game_switch_enable_fops,
    },
    {
        .filename = PROC_INCELL_PANEL_FILENAME,
        .filepath = PROC_INCELL_PANEL_FILEPATH,
        .fops = &proc_incell_panel_fops,
    },
    {
        .filename = PROC_IRQ_DEPTH_FILENAME,
        .filepath = PROC_IRQ_DEPTH_FILEPATH,
        .fops = &proc_irq_depth_fops,
    },
    {
        .filename = PROC_OPPO_REGISTER_INFO_FILENAME,
        .filepath = PROC_OPPO_REGISTER_INFO_FILEPATH,
        .fops = &proc_oppo_register_info_fops,
    },
    {
        .filename = PROC_OPPO_TP_DIRECTION_FILENAME,
        .filepath = PROC_OPPO_TP_DIRECTION_FILEPATH,
        .fops = &proc_oppo_tp_direction_fops,
    },
    {
        .filename = PROC_OPPO_TP_LIMIT_ENABLE_FILENAME,
        .filepath = PROC_OPPO_TP_LIMIT_ENABLE_FILEPATH,
        .fops = &proc_oppo_tp_limit_enable_fops,
    },
    {
        .filename = PROC_TP_FW_UPDATE_FILENAME,
        .filepath = PROC_TP_FW_UPDATE_FILEPATH,
        .fops = &proc_tp_fw_update_fops,
    },
};

static const struct proc_dir_create_data proc_dir_touchpanel_create_data = {
    .dirname = PROC_TOUCHPANEL_DIR_NAME,
    .dirpath = PROC_TOUCHPANEL_DIR_PATH,
    .nfiles = ARRAY_SIZE(proc_dir_touchpanel_files_create_data),
    .files = proc_dir_touchpanel_files_create_data,
};


static int create_proc_file(struct cts_oem_data *vdata,
    struct proc_dir_entry *parent,
    const struct proc_file_create_data *create_data)
{
    struct proc_dir_entry *file_entry;
    umode_t mode = 0044;

    if (create_data == NULL) {
        cts_err("Create proc file with create_data = NULL");
        return -EINVAL;
    }
    if (create_data->fops->proc_read == NULL &&
        create_data->fops->proc_write == NULL) {
        cts_err("Create proc file '%s' both ops->read & ops->write = NULL",
            create_data->filepath);
        return -EINVAL;
    }

    if(create_data->fops->proc_read) {
        mode |= S_IRUSR;
    }
    if(create_data->fops->proc_write) {
        mode |= (S_IWUSR | S_IWUGO);
    }

    cts_info("Create proc file '%s', mode: 0%o",
        create_data->filepath, mode);

    file_entry = proc_create_data(create_data->filename, mode,
        parent, create_data->fops, vdata->cts_data);
    if (file_entry == NULL) {
        cts_err("Create proc file '%s' failed", create_data->filename);
        return -EIO;
    }

    return 0;
}

static struct proc_dir_entry * create_proc_dir(struct cts_oem_data *vdata,
     struct proc_dir_entry *parent,
     const struct proc_dir_create_data *create_data)
{
    struct proc_dir_entry *dir_entry;
    int i, ret;

    if (create_data == NULL) {
        cts_err("Create proc file with create_data = NULL");
        return ERR_PTR(-EINVAL);
    }

    cts_info("Create proc dir '%s' include %d files parent %p",
        create_data->dirpath, create_data->nfiles, parent);

    dir_entry = proc_mkdir(create_data->dirname, parent);
    if (dir_entry == NULL) {
        cts_err("Create proc dir '%s' failed", create_data->dirpath);
        return ERR_PTR(-EIO);
    }

    for (i = 0; i < create_data->nfiles; i++) {
        ret = create_proc_file(vdata, dir_entry, &create_data->files[i]);
        if (ret) {
            cts_err("Create proc file '%s' failed %d(%s)",
                create_data->files[i].filepath, ret, cts_strerror(ret));
            /* Ignore ??? */

            /* Roll back */
            remove_proc_subtree(create_data->dirname, parent);

            return ERR_PTR(ret);
        }
    }

    return dir_entry;
}


static const char *usb_state_str(bool state)
{
    return state ? "ATTACHED" : "DETACHED";
}

static void set_dev_usb_state_work(struct work_struct *work)
{
    struct cts_oem_data *vdata;
    int ret;

    vdata = container_of(work, struct cts_oem_data,
        set_usb_state_work);

    cts_lock_device(&vdata->cts_data->cts_dev);
    ret = cts_set_dev_charger_attached(&vdata->cts_data->cts_dev,
        vdata->usb_state);
    cts_unlock_device(&vdata->cts_data->cts_dev);
    if (ret) {
        cts_err("Set dev USB state to %s failed %d(%s)",
            usb_state_str(vdata->usb_state),/*usb_state or headset_state ?*/
            ret, cts_strerror(ret));
        /* Set to previous state, try set again in next loop */
        vdata->usb_state = !vdata->usb_state;
    }
}
static int usb_notify_callback(struct notifier_block *nb,
                           unsigned long value, void *data)
{
    struct cts_oem_data *vdata;
    bool   prev_state;

    if (nb == NULL) {
        cts_err("USB notify with notifier block = NULL");
        return NOTIFY_DONE;
    }

    vdata = container_of(nb, struct cts_oem_data, usb_notifier);

    prev_state = vdata->usb_state;
    vdata->usb_state = value;

    if (vdata->usb_state != prev_state) {
        cts_info("State changed: %s -> %s",
            usb_state_str(prev_state),
            usb_state_str(vdata->usb_state));

        if (cts_is_device_enabled(&vdata->cts_data->cts_dev)) {
            if (!queue_work(vdata->cts_data->workqueue,
                &vdata->set_usb_state_work)) {
                cts_warn("Set device USB state work is PENDING");
            }
        }
    }

    return NOTIFY_OK;
}

int cts_oem_init(struct chipone_ts_data *cts_data)
{
    struct cts_oem_data *vdata = NULL;
    int ret;

    if (cts_data == NULL) {
        cts_err("Init with cts_data = NULL");
        return -EINVAL;
    }

    cts_info("Init");

    cts_data->vendor_data = NULL;

    vdata = kzalloc(sizeof(*vdata), GFP_KERNEL);
    if (vdata == NULL) {
        cts_err("Alloc vendor data failed");
        return -ENOMEM;
    }

    cts_data->vendor_data = vdata;
	vdata->cts_data = cts_data;

    vdata->proc_dir_touchpanel_entry = create_proc_dir(vdata,
        NULL, &proc_dir_touchpanel_create_data);
    if (IS_ERR_OR_NULL(vdata->proc_dir_touchpanel_entry)) {
        ret = PTR_ERR(vdata->proc_dir_touchpanel_entry);
        vdata->proc_dir_touchpanel_entry = NULL;
        cts_err("Create proc dir '"PROC_TOUCHPANEL_DIR_PATH"' failed %d(%s)",
            ret, cts_strerror(ret));
        goto free_vendor_data;
    }
    vdata->proc_dir_debuginfo_entry = create_proc_dir(vdata,
        vdata->proc_dir_touchpanel_entry,
        &proc_dir_debuginfo_create_data);
    if (IS_ERR_OR_NULL(vdata->proc_dir_debuginfo_entry)) {
        ret = PTR_ERR(vdata->proc_dir_debuginfo_entry);
        vdata->proc_dir_debuginfo_entry = NULL;
        cts_err("Create proc dir '"PROC_DEBUG_INFO_DIR_PATH"' failed %d(%s)",
            ret, cts_strerror(ret));
        goto remove_proc_dir_touchpanel;
    }

    /* USB */
    INIT_WORK(&vdata->set_usb_state_work, set_dev_usb_state_work);

    vdata->usb_notifier.notifier_call = usb_notify_callback;
    ret = tp_usb_register_client(&vdata->usb_notifier);
    if (ret) {
        cts_err("Register usb notifier failed: %d(%s)",
            ret, cts_strerror(ret));
        // Ignore this error
    }
    vdata->usb_notifier_registered = true;

    cts_data->vendor_data = vdata;
	vdata->cts_data = cts_data;

    return 0;

remove_proc_dir_touchpanel:
    if (vdata->proc_dir_touchpanel_entry) {
        if (vdata->proc_dir_debuginfo_entry) {
            cts_info("Remove proc dir '"PROC_DEBUG_INFO_DIR_PATH"'");
            remove_proc_subtree(PROC_DEBUG_INFO_DIR_NAME,
                vdata->proc_dir_touchpanel_entry);
            vdata->proc_dir_debuginfo_entry = NULL;
        }

        cts_info("Remove proc dir '"PROC_TOUCHPANEL_DIR_PATH"'");
        remove_proc_subtree(PROC_TOUCHPANEL_DIR_NAME, NULL);
        vdata->proc_dir_touchpanel_entry = NULL;
    }

free_vendor_data:
    kfree(vdata);
    cts_data->vendor_data = NULL;

    return ret;
}


int cts_oem_deinit(struct chipone_ts_data *cts_data)
{
    struct cts_oem_data *vdata = NULL;

    if (cts_data == NULL) {
        cts_err("Deinit with cts_data = NULL");
        return -EINVAL;
    }

    if (cts_data->vendor_data == NULL) {
        cts_warn("Deinit with vendor_data = NULL");
        return -EINVAL;
    }

    cts_info("Deinit");

    vdata = cts_data->vendor_data;

    if (vdata->proc_dir_touchpanel_entry) {
        if (vdata->proc_dir_debuginfo_entry) {
            cts_info("Remove proc dir '"PROC_DEBUG_INFO_DIR_PATH"'");
            remove_proc_subtree(PROC_DEBUG_INFO_DIR_NAME,
                vdata->proc_dir_touchpanel_entry);
            vdata->proc_dir_debuginfo_entry = NULL;
        }

        cts_info("Remove proc dir '"PROC_TOUCHPANEL_DIR_PATH"'");
        remove_proc_subtree(PROC_TOUCHPANEL_DIR_NAME, NULL);
        vdata->proc_dir_touchpanel_entry = NULL;
    }

    free_baseline_test_data_mem(vdata);

#ifdef CONFIG_CTS_GESTURE_TEST
    free_black_screen_test_data_mem(vdata);
#endif

    if (vdata->rawdata) {
        kfree(vdata->rawdata);
        vdata->rawdata = NULL;
    }
    if (vdata->delta) {
        kfree(vdata->delta);
        vdata->delta = NULL;
    }

    kfree(cts_data->vendor_data);
    cts_data->vendor_data = NULL;

    return 0;
}
