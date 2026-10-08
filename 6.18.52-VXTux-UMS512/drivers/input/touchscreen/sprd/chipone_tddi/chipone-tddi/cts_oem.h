#ifndef CTS_OEM_H
#define CTS_OEM_H

#include "cts_strerror.h"

struct chipone_ts_data;

/* Following options override device tree settings */
#define OEM_OF_DEF_PROPVAL_TEST_RESET_PIN   true
#define OEM_OF_DEF_PROPVAL_TEST_INT_PIN     false
#define OEM_OF_DEF_PROPVAL_TEST_RAWDATA     true
#define OEM_OF_DEF_PROPVAL_TEST_NOISE       true
#define OEM_OF_DEF_PROPVAL_TEST_OPEN        true
#define OEM_OF_DEF_PROPVAL_TEST_SHORT       true
#define OEM_OF_DEF_PROPVAL_TEST_COMP_CAP    true
#ifdef  CONFIG_CTS_GESTURE_TEST
#define OEM_OF_DEF_PROPVAL_TEST_GESTURE_RAWDATA    true
#define OEM_OF_DEF_PROPVAL_TEST_GESTURE_LP_RAWDATA true
#define OEM_OF_DEF_PROPVAL_TEST_GESTURE_NOISE      true
#define OEM_OF_DEF_PROPVAL_TEST_GESTURE_LP_NOISE   true
#endif

/* Default settings if device tree NOT exist */
#define OEM_OF_DEF_PROPVAL_RAWDATA_FRAMES   1
#define OEM_OF_DEF_PROPVAL_RAWDATA_MIN      1235
#define OEM_OF_DEF_PROPVAL_RAWDATA_MAX      2835
#define OEM_OF_DEF_PROPVAL_NOISE_FRAMES     16
#define OEM_OF_DEF_PROPVAL_NOISE_MAX        75
#define OEM_OF_DEF_PROPVAL_OPEN_MIN         2500
#define OEM_OF_DEF_PROPVAL_SHORT_MIN        350
#define OEM_OF_DEF_PROPVAL_COMP_CAP_MIN     1
#define OEM_OF_DEF_PROPVAL_COMP_CAP_MAX     126


#ifdef CONFIG_CTS_GESTURE_TEST
#define OEM_OF_DEF_PROPVAL_GESTURE_RAWDATA_FRAMES     3
#define OEM_OF_DEF_PROPVAL_GESTURE_RAWDATA_MIN        1094
#define OEM_OF_DEF_PROPVAL_GESTURE_RAWDATA_MAX        3084
#define OEM_OF_DEF_PROPVAL_GESTURE_LP_RAWDATA_FRAMES  3
#define OEM_OF_DEF_PROPVAL_GESTURE_LP_RAWDATA_MIN     1070
#define OEM_OF_DEF_PROPVAL_GESTURE_LP_RAWDATA_MAX     3104
#define OEM_OF_DEF_PROPVAL_GESTURE_NOISE_FRAMES       3
#define OEM_OF_DEF_PROPVAL_GESTURE_NOISE_MAX          145
#define OEM_OF_DEF_PROPVAL_GESTURE_LP_NOISE_FRAMES    3
#define OEM_OF_DEF_PROPVAL_GESTURE_LP_NOISE_MAX       55
#endif

#define AUO_OEM_OF_DEF_PROPVAL_RAWDATA_FRAMES   1
#define AUO_OEM_OF_DEF_PROPVAL_RAWDATA_MIN      1339
#define AUO_OEM_OF_DEF_PROPVAL_RAWDATA_MAX      3159
#define AUO_OEM_OF_DEF_PROPVAL_NOISE_FRAMES     16
#define AUO_OEM_OF_DEF_PROPVAL_NOISE_MAX        76
#define AUO_OEM_OF_DEF_PROPVAL_OPEN_MIN         2700
#define AUO_OEM_OF_DEF_PROPVAL_SHORT_MIN        350
#define AUO_OEM_OF_DEF_PROPVAL_COMP_CAP_MIN     1
#define AUO_OEM_OF_DEF_PROPVAL_COMP_CAP_MAX     126


#ifdef CONFIG_CTS_GESTURE_TEST
#define AUO_OEM_OF_DEF_PROPVAL_GESTURE_RAWDATA_FRAMES     3
#define AUO_OEM_OF_DEF_PROPVAL_GESTURE_RAWDATA_MIN        1040
#define AUO_OEM_OF_DEF_PROPVAL_GESTURE_RAWDATA_MAX        2700
#define AUO_OEM_OF_DEF_PROPVAL_GESTURE_LP_RAWDATA_FRAMES  3
#define AUO_OEM_OF_DEF_PROPVAL_GESTURE_LP_RAWDATA_MIN     1040
#define AUO_OEM_OF_DEF_PROPVAL_GESTURE_LP_RAWDATA_MAX     2700
#define AUO_OEM_OF_DEF_PROPVAL_GESTURE_NOISE_FRAMES       3
#define AUO_OEM_OF_DEF_PROPVAL_GESTURE_NOISE_MAX          145
#define AUO_OEM_OF_DEF_PROPVAL_GESTURE_LP_NOISE_FRAMES    3
#define AUO_OEM_OF_DEF_PROPVAL_GESTURE_LP_NOISE_MAX       55
#endif

#define OEM_OF_PROPNAME_PREFIX                 "chipone," //modify

#define OEM_OF_PROPNAME_TEST_RESET_PIN      OEM_OF_PROPNAME_PREFIX"test-reset-pin"
#define OEM_OF_PROPNAME_TEST_INT_PIN        OEM_OF_PROPNAME_PREFIX"test-int-pin"
#define OEM_OF_PROPNAME_TEST_RAWDATA        OEM_OF_PROPNAME_PREFIX"test-rawdata"
#define OEM_OF_PROPNAME_RAWDATA_FRAMES      OEM_OF_PROPNAME_PREFIX"test-rawdata-frames"
#define OEM_OF_PROPNAME_RAWDATA_MIN         OEM_OF_PROPNAME_PREFIX"rawdata-min"
#define OEM_OF_PROPNAME_RAWDATA_MAX         OEM_OF_PROPNAME_PREFIX"rawdata-max"
#define OEM_OF_PROPNAME_TEST_NOISE          OEM_OF_PROPNAME_PREFIX"test-noise"
#define OEM_OF_PROPNAME_NOISE_FRAMES        OEM_OF_PROPNAME_PREFIX"test-noise-frames"
#define OEM_OF_PROPNAME_NOISE_MAX           OEM_OF_PROPNAME_PREFIX"noise-max"
#define OEM_OF_PROPNAME_TEST_OPEN           OEM_OF_PROPNAME_PREFIX"test-open"
#define OEM_OF_PROPNAME_OPEN_MIN            OEM_OF_PROPNAME_PREFIX"open-min"
#define OEM_OF_PROPNAME_TEST_SHORT          OEM_OF_PROPNAME_PREFIX"test-short"
#define OEM_OF_PROPNAME_SHORT_MIN           OEM_OF_PROPNAME_PREFIX"short-min"
#define OEM_OF_PROPNAME_TEST_COMP_CAP       OEM_OF_PROPNAME_PREFIX"test-compensate-cap"
#define OEM_OF_PROPNAME_COMP_CAP_MIN        OEM_OF_PROPNAME_PREFIX"compensate-cap-min"
#define OEM_OF_PROPNAME_COMP_CAP_MAX        OEM_OF_PROPNAME_PREFIX"compensate-cap-max"
#ifdef  CONFIG_CTS_GESTURE_TEST
#define OEM_OF_PROPNAME_TEST_GESTURE_RAWDATA       OEM_OF_PROPNAME_PREFIX"test-gesture-rawdata"
#define OEM_OF_PROPNAME_GESTURE_RAWDATA_FRAMES     OEM_OF_PROPNAME_PREFIX"test-gesture-rawdata-frames"
#define OEM_OF_PROPNAME_GESTURE_RAWDATA_MIN        OEM_OF_PROPNAME_PREFIX"gesture-rawdata-min"
#define OEM_OF_PROPNAME_GESTURE_RAWDATA_MAX        OEM_OF_PROPNAME_PREFIX"gesture-rawdata-max"
#define OEM_OF_PROPNAME_TEST_GESTURE_LP_RAWDATA    OEM_OF_PROPNAME_PREFIX"test-gesture-lp-rawdata"
#define OEM_OF_PROPNAME_GESTURE_LP_RAWDATA_FRAMES  OEM_OF_PROPNAME_PREFIX"test-gesture-lp-rawdata-frames"
#define OEM_OF_PROPNAME_GESTURE_LP_RAWDATA_MIN     OEM_OF_PROPNAME_PREFIX"gesture-lp-rawdata-min"
#define OEM_OF_PROPNAME_GESTURE_LP_RAWDATA_MAX     OEM_OF_PROPNAME_PREFIX"gesture-lp-rawdata-max"
#define OEM_OF_PROPNAME_TEST_GESTURE_NOISE         OEM_OF_PROPNAME_PREFIX"test-gesture-noise"
#define OEM_OF_PROPNAME_GESTURE_NOISE_FRAMES       OEM_OF_PROPNAME_PREFIX"test-gesture-noise-frames"
#define OEM_OF_PROPNAME_GESTURE_NOISE_MAX          OEM_OF_PROPNAME_PREFIX"gesture-noise-max"
#define OEM_OF_PROPNAME_TEST_GESTURE_LP_NOISE      OEM_OF_PROPNAME_PREFIX"test-gesture-lp-noise"
#define OEM_OF_PROPNAME_GESTURE_LP_NOISE_FRAMES    OEM_OF_PROPNAME_PREFIX"test-gesture-lp-noise-frames"
#define OEM_OF_PROPNAME_GESTURE_LP_NOISE_MAX       OEM_OF_PROPNAME_PREFIX"gesture-lp-noise-max"
#endif

#define CTS_PROC_TOUCHPANEL					"touchpanel"
#define OEM_SELFTEST_PROC_FILENAME          "baseline_test"
#define OEM_SELFTEST_BLACK_FILENAME          "black_screen_test"

#define OEM_TPTEST_REPORT                   "/sdcard/TpTestReport"
#define OEM_TEST_DATA_DIR                   "/sdcard/TpTestReport/chipone_tpdata_"
#define OEM_RAWDATA_TEST_DATA_FILEPATH      "/RawdataTest.csv"
#define OEM_NOISE_TEST_DATA_FILEPATH        "/NoiseTest.csv"
#define OEM_OPEN_TEST_DATA_FILEPATH         "/OpenTest.csv"
#define OEM_SHORT_TEST_DATA_FILEPATH        "/ShortTest.csv"
#define OEM_COMP_CAP_TEST_DATA_FILEPATH     "/CompCapTest.csv"

#ifdef CONFIG_CTS_GESTURE_TEST
#define OEM_BLACK_TEST_DATA_DIR               "/sdcard/TpTestReport"
#define GESTURE_RAWDATA_TEST_DATA_FILENAME    OEM_BLACK_TEST_DATA_DIR"/Gstr_Raw_Test.csv"
#define GESTURE_LP_RAWDATA_TEST_DATA_FILENAME OEM_BLACK_TEST_DATA_DIR"/Gstr_Lp_Raw_Test.csv"
#define GESTURE_NOISE_TEST_DATA_FILENAME      OEM_BLACK_TEST_DATA_DIR"/Gstr_Noise_Test.csv"
#define GESTURE_LP_NOISE_TEST_DATA_FILENAME   OEM_BLACK_TEST_DATA_DIR"/Gstr_Lp_Noise_Test.csv"

extern struct blacktest_time blacktest_time_ms;
#endif

/* /proc/touchpanel */
#define PROC_TOUCHPANEL_DIR_NAME    "touchpanel"
#define PROC_TOUCHPANEL_DIR_PATH    "/proc/"PROC_TOUCHPANEL_DIR_NAME

#define PROC_BASELINE_TEST_FILENAME "baseline_test"
#define PROC_BASELINE_TEST_FILEPATH \
    PROC_TOUCHPANEL_DIR_PATH"/"PROC_BASELINE_TEST_FILENAME

#define PROC_BLACK_SCREEN_TEST_FILENAME "black_screen_test"
#define PROC_BLACK_SCREEN_TEST_FILEPATH \
    PROC_TOUCHPANEL_DIR_PATH"/"PROC_BLACK_SCREEN_TEST_FILENAME

#define PROC_COORDINATE_FILENAME "coordinate"
#define PROC_COORDINATE_FILEPATH    \
    PROC_TOUCHPANEL_DIR_PATH"/"PROC_COORDINATE_FILENAME

#define PROC_DEBUG_LEVEL_FILENAME "debug_level"
#define PROC_DEBUG_LEVEL_FILEPATH   \
    PROC_TOUCHPANEL_DIR_PATH"/"PROC_DEBUG_LEVEL_FILENAME

#define PROC_DOUBLE_TAP_ENABLE_FILENAME "double_tap_enable"
#define PROC_DOUBLE_TAP_ENABLE_FILEPATH \
    PROC_TOUCHPANEL_DIR_PATH"/"PROC_DOUBLE_TAP_ENABLE_FILENAME

#define PROC_GAME_SWITCH_ENABLE_FILENAME "game_switch_enable"
#define PROC_GAME_SWITCH_ENABLE_FILEPATH    \
    PROC_TOUCHPANEL_DIR_PATH"/"PROC_GAME_SWITCH_ENABLE_FILENAME

#define PROC_INCELL_PANEL_FILENAME "incell_panel"
#define PROC_INCELL_PANEL_FILEPATH    \
    PROC_TOUCHPANEL_DIR_PATH"/"PROC_INCELL_PANEL_FILENAME

#define PROC_IRQ_DEPTH_FILENAME "irq_depth"
#define PROC_IRQ_DEPTH_FILEPATH    \
    PROC_TOUCHPANEL_DIR_PATH"/"PROC_IRQ_DEPTH_FILENAME

#define PROC_OPPO_REGISTER_INFO_FILENAME "oppo_register_info"
#define PROC_OPPO_REGISTER_INFO_FILEPATH    \
    PROC_TOUCHPANEL_DIR_PATH"/"PROC_OPPO_REGISTER_INFO_FILENAME

#define PROC_OPPO_TP_DIRECTION_FILENAME "oppo_tp_direction"
#define PROC_OPPO_TP_DIRECTION_FILEPATH    \
    PROC_TOUCHPANEL_DIR_PATH"/"PROC_OPPO_TP_DIRECTION_FILENAME

#define PROC_OPPO_TP_LIMIT_ENABLE_FILENAME "oppo_tp_limit_enable"
#define PROC_OPPO_TP_LIMIT_ENABLE_FILEPATH    \
    PROC_TOUCHPANEL_DIR_PATH"/"PROC_OPPO_TP_LIMIT_ENABLE_FILENAME

#define PROC_TP_FW_UPDATE_FILENAME "tp_fw_update"
#define PROC_TP_FW_UPDATE_FILEPATH    \
    PROC_TOUCHPANEL_DIR_PATH"/"PROC_TP_FW_UPDATE_FILENAME

/* /proc/debug_info */
#define PROC_DEBUG_INFO_DIR_NAME    "debug_info"
#define PROC_DEBUG_INFO_DIR_PATH    \
    PROC_TOUCHPANEL_DIR_PATH"/"PROC_DEBUG_INFO_DIR_NAME

#define PROC_BASELINE_FILENAME  "baseline"
#define PROC_BASELINE_FILEPATH    \
    PROC_DEBUG_INFO_DIR_PATH"/"PROC_BASELINE_FILENAME

#define PROC_DATA_LIMIT_FILENAME  "data_limit"
#define PROC_DATA_LIMIT_FILEPATH    \
    PROC_DEBUG_INFO_DIR_PATH"/"PROC_DATA_LIMIT_FILENAME

#define PROC_DELTA_FILENAME  "delta"
#define PROC_DELTA_FILEPATH    \
    PROC_DEBUG_INFO_DIR_PATH"/"PROC_DELTA_FILENAME

#define PROC_MAIN_REGISTER_FILENAME  "main_register"
#define PROC_MAIN_REGISTER_FILEPATH    \
    PROC_DEBUG_INFO_DIR_PATH"/"PROC_MAIN_REGISTER_FILENAME


struct cts_oem_data {
    struct proc_dir_entry *proc_dir_touchpanel_entry;
    struct proc_dir_entry *proc_dir_debuginfo_entry;
	struct proc_dir_entry *selftest_proc_entry;
	struct proc_dir_entry * black_selftest_proc_entry;
    bool test_config_from_dt_has_parsed;
	bool black_test_config_from_dt_has_parsed;


    void *rawdata;

    void *delta;

    int   double_tap_enable;
    char  coordinate[256];
    int   coordinate_len;
    int   debug_level;

    bool game_switch_enable;

    u32 oppo_tp_direction;
    u32 oppo_tp_limit_enable;
    /* Baseline test(Screen ON) parameter */
    bool test_reset_pin;
    int  reset_pin_test_result;

    bool test_int_pin;
    int  int_pin_test_result;
    bool test_rawdata;
    u32  rawdata_test_frames;
    int  rawdata_test_result;
    u16 *rawdata_test_data;
    int  rawdata_test_data_buff_size;
    int  rawdata_test_data_wr_size;
    int  rawdata_min;
    int  rawdata_max;

    bool test_noise;
    u32  noise_test_frames;
    int  noise_test_result;
    u16 *noise_test_data;
    int  noise_test_data_buff_size;
    int  noise_test_data_wr_size;
    int  noise_max;

    bool test_open;
    int  open_test_result;
    u16 *open_test_data;
    int  open_test_data_buff_size;
    int  open_test_data_wr_size;
    int  open_min;

    bool test_short;
    int  short_test_result;
    u16 *short_test_data;
    int  short_test_data_buff_size;
    int  short_test_data_wr_size;
    int  short_min;

    bool test_comp_cap;
    int  comp_cap_test_result;
    u8  *comp_cap_test_data;
    int  comp_cap_test_data_buff_size;
    int  comp_cap_test_data_wr_size;
    int  comp_cap_min;
    int  comp_cap_max;

#ifdef CONFIG_CTS_GESTURE_TEST
		bool test_gesture_rawdata;
		u32  gesture_rawdata_test_frames;
		int  gesture_rawdata_test_result;
		int  gesture_rawdata_min;
		int  gesture_rawdata_max;
		u16 *gesture_rawdata_test_data;
		int  gesture_rawdata_test_data_buff_size;
		int  gesture_rawdata_test_data_wr_size;

		bool test_gesture_lp_rawdata;
		u32  gesture_lp_rawdata_test_frames;
		int  gesture_lp_rawdata_test_result;
		int  gesture_lp_rawdata_min;
		int  gesture_lp_rawdata_max;
		u16 *gesture_lp_rawdata_test_data;
		int  gesture_lp_rawdata_test_data_buff_size;
		int  gesture_lp_rawdata_test_data_wr_size;

		bool test_gesture_noise;
		u32  gesture_noise_test_frames;
		int  gesture_noise_test_result;
		int  gesture_noise_max;
		u16 *gesture_noise_test_data;
		int  gesture_noise_test_data_buff_size;
		int  gesture_noise_test_data_wr_size;

		bool test_gesture_lp_noise;
		u32  gesture_lp_noise_test_frames;
		int  gesture_lp_noise_test_result;
		int  gesture_lp_noise_max;
		u16 *gesture_lp_noise_test_data;
		int  gesture_lp_noise_test_data_buff_size;
		int  gesture_lp_noise_test_data_wr_size;
		int  blackscreen_test_reault;
#endif

    /* USB */
    bool usb_notifier_registered;
    struct notifier_block usb_notifier;
    struct work_struct set_usb_state_work;
    bool usb_state;

    struct chipone_ts_data *cts_data;

};

#ifdef CONFIG_CTS_GESTURE_TEST
int cts_dev_mkdir(char *name, umode_t mode);
int parse_black_selftest_dt(struct cts_oem_data *oem_data, struct device_node *np);
int auo_parse_black_selftest_dt(struct cts_oem_data *oem_data, struct device_node *np);

//static void do_black_selftest(struct cts_device *cts_dev, struct cts_oem_data *oem_data);
int prepare_black_test(struct cts_device *cts_dev);
int alloc_black_selftest_data_mem(struct cts_oem_data *oem_data, int nodes);
int dump_tsdata_to_csv_file(const char *filepath, int flags,
    const u16 *data, int frames, int rows, int cols);
#endif

extern int cts_oem_init(struct chipone_ts_data *cts_data);
extern int cts_oem_deinit(struct chipone_ts_data *cts_data);

#endif /* CTS_VENDOR_H */

