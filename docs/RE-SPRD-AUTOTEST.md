# RE `core.ko` + `gpio.ko` — framework autotest

Sumber: `donors/stock_vendor_modules_all/core.ko` (sprd autotest core
driver) dan `gpio.ko` (sprd autotest gpio driver, `depends=core`).
Disassembly penuh dengan relokasi:
[core.objdump.txt](evidence-20261023/core.objdump.txt) dan
[gpio.objdump.txt](evidence-20261023/gpio.objdump.txt).

Port: `drivers/vxtux/soc/autotest/{sprd_autotest_core.c,sprd_autotest_gpio.c}`
(Kconfig `SPRD_AUTOTEST`, `SPRD_AUTOTEST_GPIO`).

## Core

| item | nilai |
|---|---|
| alokasi | `kmem_cache_alloc_trace(..., GFP_KERNEL, 0x460)` (1120 B) |
| list head | `+0x430` |
| mutex | `+0x440` |
| cdev | `+0x3a8` |
| device | offset 0 (`device_initialize(autotest)`) |
| nama | `dev_set_name(dev, "autotest%d", 0)` → `autotest0` |
| class | `.rodata+0x65` = `"autotest"` |
| devt | `MKDEV(MAJOR(devt), 0)` — region 10 minor |
| ekspor | `sprd_autotest_register_handler`, `sprd_autotest_unregister_handler` |

Handler (`.data` gpio.ko):

```
+0x00 const char *name
+0x08 u32 cmd
+0x10 struct list_head list
+0x20 int (*init)(handler *h, void *data)
+0x28 int (*test)(handler *h, unsigned long arg)
+0x30 void (*exit)(handler *h, void *data)
+0x38 void *data
```

- `register`: `!h || !h->test` → `-EINVAL`; cmd duplikat → cetak
  `"try to add dupicated autotest handler"` dan `-EEXIST`; panggil
  `h->init(h, h->data)` bila ada (error-nya diteruskan); `INIT_LIST_HEAD`;
  `list_add()` di bawah mutex.
- `unregister`: `h->exit(h, h->data)` bila ada, lalu `list_del()`.
- `ioctl`: `printk("autotest cmd = %x")`, cari handler dengan `cmd` sama,
  panggil `h->test(h, arg)`; tidak ketemu → `-EINVAL`.
- `open`/`release` hanya `return 0`.

## GPIO handler

`.data`: nama `"gpio"` (`.rodata+0x00`), `cmd = 0xc0044101`
(`_IOWR('A', 1, 4)`), test = `gpio_test`.

Argumen ioctl = satu `u32` di user:

```
bit  0..15  offset
bit 16      dir
bit 17      value (dipakai juga untuk menulis balik hasil input)
```

Alur `gpio_test`:

1. `get_user(req, arg)`; `printk("offset=%d, dir=%d, value=%d")`.
2. `gpiochip_find(NULL, match)` dengan `match: chip->ngpio == 256`
   (`ldrh [chip,#140] == 0x100`).
3. `gpio = chip->base + offset`; `desc = gpio_to_desc(gpio)`.
4. `gpio_request(gpio, "gpio")`; error selain `-EBUSY` → cetak
   `"gpio request failed."` dan keluar.
5. `dir`:
   - line adalah IRQ (`gpiochip_line_is_irq`) → `chip->set(chip, offset, value)`
   - selain itu → `gpiod_direction_output(desc, value)`
6. `!dir`: `gpiod_direction_input(desc)` lalu `gpiod_get_value(desc)`;
   hasil `!= 0` ditulis ke bit 17 argumen user (`put_user`).
7. Error akses user → `-EFAULT` (`"write to user failed."` bila setelah
   operasi).

## Catatan port (6.18)

- `gpiochip_find()` dan `gpio_request()` sudah dihapus. Penggantinya:
  `gpio_device_find(NULL, match)` → `gpio_device_get_chip()` +
  `gpiochip_request_own_desc(chip, offset, "gpio", GPIO_LOOKUP_FLAGS_DEFAULT,
  GPIOD_ASIS)`. Bila hasilnya `-EBUSY` (line sudah dipegang pihak lain),
  port memakai `gpio_device_get_desc()` seperti perilaku blob yang tetap
  melanjutkan.
- `class_create()` 6.18 hanya menerima nama (tanpa `owner`).
- Nama modul dipertahankan berbeda (`sprd_autotest_core`/`_gpio`) supaya
  tidak bertabrakan dengan `core`/`gpio` generik di build lain.
