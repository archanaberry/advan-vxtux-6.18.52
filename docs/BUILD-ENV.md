# Build environment tanpa root (codespace / mesin bersih)

Diperoleh kembali 2026-10-23 setelah codespace kehilangan seluruh toolchain
cross: `dpkg -l | grep aarch64` = 0, tidak ada `/usr/bin/aarch64*`, padahal
`.config` pohon masih menyebut "built by aarch64-linux-gnu-gcc 11.4.0".
Semua langkah di bawah **tanpa sudo** dan tidak menyentuh sistem.

## 1. Toolchain cross

```bash
mkdir -p $HOME/tc && cd $HOME/tc
curl -fSL -o tc.tar.xz \
  https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-linux-gnu.tar.xz
tar xf tc.tar.xz          # direktori: arm-gnu-toolchain-13.2.Rel1-... (R besar)
export PATH=$HOME/tc/arm-gnu-toolchain-13.2.Rel1-x86_64-aarch64-none-linux-gnu/bin:$PATH
```

## 2. Dependensi host yang tidak ada di image

`flex`, `bc`, `m4`, `libelf-dev` — diambil sebagai `.deb` lalu diekstrak,
bukan `apt install`:

```bash
mkdir -p $HOME/apt/lists/partial $HOME/apt/cache/archives/partial $HOME/deb $HOME/sysroot
apt-get -o Dir::State::Lists=$HOME/apt/lists -o Dir::Cache=$HOME/apt/cache update
cd $HOME/deb && apt-get -o Dir::State::Lists=$HOME/apt/lists \
  -o Dir::Cache=$HOME/apt/cache download flex bc m4 libelf-dev
for f in $HOME/deb/*.deb; do dpkg -x "$f" $HOME/sysroot; done
export PATH=$HOME/sysroot/usr/bin:$PATH
```

Kalau `apt-get download` menjawab `E: Unable to locate package bc` (daftar
paket kosong dan `update` tidak bisa menulis `/var/lib/apt`), ambil `.deb`-nya
langsung dari arsip Ubuntu sesuai rilis image — cara ini yang akhirnya dipakai
untuk `bc` di Ubuntu 24.04 (**noble, amd64**):

```bash
curl -sSL -o $HOME/bc.deb \
  https://archive.ubuntu.com/ubuntu/pool/main/b/bc/bc_1.07.1-3build1_amd64.deb
dpkg-deb -x $HOME/bc.deb $HOME/bcpkg
mkdir -p $HOME/bc/bin && ln -sf $HOME/bcpkg/usr/bin/bc $HOME/bc/bin/bc
ls $HOME/bc/bin/bc   # libreadline8 sudah ada di image, tak perlu tambahan
```

Tanpa `bc`, `make` berhenti di `Kbuild:24: include/generated/timeconst.h`
(`/bin/sh: 1: bc: not found`) — bukan error driver.

## 3. Pohon sumber yang bersih

Pohon di `/workspaces/...` berisi artefak milik **root**, sehingga
`make O=` menolak dengan *"The source tree is not clean"* dan
`include/generated/*` tidak bisa ditulis ulang. Clone baru milik user adalah
jalan keluarnya:

```bash
git clone --depth 1 --single-branch -b test1 /workspaces/advan-vxtux-6.18.52 $HOME/src
cd $HOME/src/6.18.52-VXTux-UMS512
chmod -R +x scripts/       # bit executable hilang saat commit dari Windows
```

Kalau clone-nya **sudah** ada tapi tercemar artefak root, jangan clone ulang
(2 GB per pohon): bikin *worktree* dari commit yang mau diuji. Objek git
dipakai bersama (hardlink), jadi cuma butuh ~1,4 GB untuk isi pohon dan
selesai dalam ~34 s:

```bash
cd /workspaces/advan-vxtux-6.18.52
git fetch --depth 1 https://github.com/<user>/advan-vxtux-6.18.52 test1
git worktree add --detach /workspaces/b2 $(git rev-parse FETCH_HEAD)
```

Dua jebakan yang sudah kena di codespace:

| Gejala | Sebab | Penanganan |
|---|---|---|
| `ls include/asm-generic/vmlinux.lds.h` hilang, padahal `git status` bersih | entri ber-*skip-worktree* (checkout parsial) | `git restore --source=HEAD --worktree -- 6.18.52-VXTux-UMS512` (1 perintah, ~8000 file kembali) |
| `fopen: Permission denied` di `include/generated/autoconf.h`, `unlink: Permission denied` | sisa build milik **root** (2391 entri; `include/config` dan `include/generated` dimiliki root) | tidak bisa `chmod`/`rm` tanpa root → pakai worktree baru milik user |

## 4. Build

```bash
export ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu-
cp .config .config.device
sed -i 's/\r$//' .config.device ../enhanced_config.cfg
scripts/kconfig/merge_config.sh -m -O . .config.device ../enhanced_config.cfg
make olddefconfig
make -j$(nproc) modules
```

## 5. Jebakan yang sudah kena

| Gejala | Sebab |
|---|---|
| `aarch64-linux-gnu-gcc: No such file` | toolchain dihapus dari image; pakai tarball di langkah 1 |
| `flex: not found` saat `olddefconfig` | kconfig butuh flex; parser `.c` tidak ikut di repo |
| `scripts/cc-version.sh: Permission denied` | bit executable hilang; `chmod -R +x scripts/` |
| `The source tree is not clean` (build `O=`) | artefak root di pohon `/workspaces`; pakai clone bersih |
| modpost: `"<simbol>" undefined` di build `M=` | `Module.symvers` belum ada; modul tunggal baru bisa di-modpost setelah `make modules` penuh |
| `make modules` sukses tapi `.ko` baru tidak muncul; subdir `modules.order` kosong | dua sebab yang pernah kena: **(a)** Makefile subdir berisi `\t` **literal** (backslash-t) — make membaca nama variabel jadi `obj-m\t+`, sehingga `obj-m` kosong dan submake ke direktori menghasilkan `{ :; } > modules.order`; periksa dengan `cat -A` (harus `^I`, bukan `\\t`). **(b)** `enhanced_config.cfg` ada di root repo di luar `$K`, jadi tidak ikut tersinkron `cs_quick.sh` dan `CONFIG_*` baru tak pernah sampai ke `.config` codespace — kini ikut dikirim + auto-merge; bila `.config` sudah di-update tapi modul tetap absen, hapus `modules.order` (root + subdir) lalu `make modules` lagi |

## 6. Hasil verifikasi 2026-10-23

- `make modules` → **rc=0**, **228 `.ko`** (227 sebelumnya + `phy-sprd-ums512.ko`).
- `drivers/phy/sprd/phy-sprd-ums512.ko` = **433.584 B**.
- Bukti: `docs/evidence-20261023/ko_list_tick13.txt` + `config_tick13.txt`.

## 7. Loop cepat: satu objek di codespace (~10 s)

Build kernel penuh di Actions memakan 15–17 menit per iterasi. Untuk menulis
driver cukup `prepare` + satu objek, dan itu bisa dijalankan langsung di
codespace (`gh cs ssh`, bukan runner). Terukur di worktree `/workspaces/b2`
(toolchain ARM 13.2 rel1, `-j6`, commit `ddb885362`):

| Tahap | Waktu |
|---|---:|
| `merge_config.sh -m` + `make olddefconfig` | **20 s** |
| `make drivers/vxtux/soc/debugstat/sprd_debugstat.o` (cold, sekaligus prepare) | **9 s** |
| objek hasil | 229.600 B, `rc=0` |

Skripnya [`donors/cs_quick.sh`](../donors/cs_quick.sh): mengirim file yang
berubah (tracked + untracked) lewat satu round-trip `tar`/`ssh`, lalu
meng-`make` target di codespace. Contoh:

```bash
donors/cs_quick.sh                                          # debugstat.o
donors/cs_quick.sh drivers/vxtux/soc/sip_svc/sprd_sip_svc.o
donors/cs_quick.sh drivers/vxtux/soc/debugstat/sprd_debugstat.ko   # BUTUH build penuh
```

Yang **tidak** bisa dipercepat begini: `.ko`. Modpost menuntut
`Module.symvers` dari vmlinux, jadi target `.ko` tetap menunggu
`make modules` penuh — di codespace itu ~15 menit dengan `-j8` (jalankan
`nohup` lalu poll log, jangan blokir sesi SSH):

```bash
nohup bash -c 'make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- \
  -j8 Image modules dtbs > /workspaces/b2/build.log 2>&1; \
  echo RC=$? >> /workspaces/b2/build.log' >/dev/null 2>&1 &
# tanpa ARCH=arm64 make langsung salah sasaran: "arch/x86/Makefile: No such file"
```
