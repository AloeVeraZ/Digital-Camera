#include "photo_image.h"
#include "driver_sdmmc.h"
#include "esp_heap_caps.h"
#include "extra/libs/sjpg/tjpgd.h"
#include <algorithm>

static uint32_t le32(const uint8_t *p) {
  return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}

struct RasterTarget {
  fs::File *file;
  PhotoImage *photo;
  int source_w, source_h, width, height, crop_x, crop_y, crop_w, crop_h;
};

static bool prepare_target(RasterTarget &t, int max_w, int max_h, bool crop) {
  if (t.source_w <= 0 || t.source_h <= 0 || t.source_w > 4096 || t.source_h > 4096)
    return false;
  t.crop_x = t.crop_y = 0;
  t.crop_w = t.source_w; t.crop_h = t.source_h;
  if (crop) {
    t.crop_w = t.crop_h = std::min(t.source_w, t.source_h);
    t.crop_x = (t.source_w - t.crop_w) / 2;
    t.crop_y = (t.source_h - t.crop_h) / 2;
    t.width = max_w; t.height = max_h;
  } else {
    t.width = max_w;
    t.height = std::max(1, t.source_h * max_w / t.source_w);
    if (t.height > max_h) {
      t.height = max_h;
      t.width = std::max(1, t.source_w * max_h / t.source_h);
    }
  }
  size_t size = t.width * t.height * sizeof(lv_color_t);
  if (t.photo->capacity < size) {
    auto *pixels = static_cast<uint8_t *>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!pixels) pixels = static_cast<uint8_t *>(malloc(size));
    if (!pixels) return false;
    free(t.photo->pixels);
    t.photo->pixels = pixels;
    t.photo->capacity = size;
  }
  lv_img_cache_invalidate_src(&t.photo->image);
  t.photo->image = {};
  t.photo->image.header.w = t.width;
  t.photo->image.header.h = t.height;
  t.photo->image.header.cf = LV_IMG_CF_TRUE_COLOR;
  t.photo->image.data_size = size;
  t.photo->image.data = t.photo->pixels;
  return true;
}

static bool load_bmp(fs::File &file, PhotoImage &photo, int max_w, int max_h, bool crop) {
  uint8_t header[70];
  if (file.read(header, sizeof(header)) != sizeof(header)) return false;
  uint32_t offset = le32(header + 10), dib = le32(header + 14);
  int32_t width = static_cast<int32_t>(le32(header + 18));
  int32_t signed_h = static_cast<int32_t>(le32(header + 22));
  int bpp = header[28] | header[29] << 8;
  uint32_t compression = le32(header + 30);
  if (header[0] != 'B' || header[1] != 'M' || dib < 40 || width <= 0 ||
      signed_h == 0 || signed_h < -4096 || signed_h > 4096 || width > 4096 ||
      header[26] != 1 || header[27] != 0 || offset < 54 || offset > file.size()) return false;
  // Camera BMPs are RGB565. Ordinary 24-bit BMPs are also accepted.
  if (bpp == 16) {
    if (compression != 3 || offset < 66 || le32(header + 54) != 0xF800 ||
        le32(header + 58) != 0x07E0 || le32(header + 62) != 0x001F) return false;
  } else if (bpp != 24 || compression != 0) return false;
  int height = signed_h < 0 ? -signed_h : signed_h;
  size_t raw_stride = width * (bpp / 8), padded_stride = (raw_stride + 3) & ~size_t(3);
  size_t available = file.size() - offset;
  // Older camera firmware omitted BMP row padding. Keep those photos readable.
  size_t stride = available >= padded_stride * height ? padded_stride : raw_stride;
  if (available < stride * height) return false;
  RasterTarget t{&file, &photo, width, height};
  if (!prepare_target(t, max_w, max_h, crop)) return false;
  // Preserve the original gallery's sequential SD read path for camera photos.
  if (bpp == 16 && !crop && t.width == width && t.height == height) {
    if (!file.seek(offset)) return false;
    for (int source_y = 0; source_y < height; ++source_y) {
      int dest_y = signed_h > 0 ? height - source_y - 1 : source_y;
      uint8_t *out = photo.pixels + dest_y * raw_stride;
      if (file.read(out, raw_stride) != raw_stride) return false;
#if LV_COLOR_16_SWAP
      for (size_t i = 0; i < raw_stride; i += 2) std::swap(out[i], out[i+1]);
#endif
      if (stride > raw_stride && !file.seek(file.position() + stride - raw_stride)) return false;
    }
    return true;
  }
  auto *row = static_cast<uint8_t *>(malloc(raw_stride));
  if (!row) return false;
  bool ok = true;
  for (int y = 0; y < t.height; ++y) {
    int sy = t.crop_y + y * t.crop_h / t.height;
    if (signed_h > 0) sy = height - sy - 1;
    if (!file.seek(offset + sy * stride) || file.read(row, raw_stride) != raw_stride) { ok = false; break; }
    auto *out = reinterpret_cast<lv_color_t *>(photo.pixels) + y * t.width;
    for (int x = 0; x < t.width; ++x) {
      int sx = t.crop_x + x * t.crop_w / t.width;
      if (bpp == 16) {
        uint16_t pixel = row[sx * 2] | row[sx * 2 + 1] << 8;
#if LV_COLOR_16_SWAP
        pixel = (pixel >> 8) | (pixel << 8);
#endif
        out[x].full = pixel;
      } else out[x] = lv_color_make(row[sx*3+2], row[sx*3+1], row[sx*3]);
    }
  }
  free(row);
  return ok;
}

#if LV_USE_SJPG
static size_t jpeg_read(JDEC *decoder, uint8_t *buffer, size_t length) {
  auto &t = *static_cast<RasterTarget *>(decoder->device);
  if (buffer) return t.file->read(buffer, length);
  size_t start = t.file->position();
  return t.file->seek(start + length) ? length : 0;
}

static int jpeg_write(JDEC *decoder, void *bitmap, JRECT *rect) {
  auto &t = *static_cast<RasterTarget *>(decoder->device);
  auto *rgb = static_cast<uint8_t *>(bitmap);
  for (int y = 0; y < t.height; ++y) {
    int sy = t.crop_y + y * t.crop_h / t.height;
    if (sy < rect->top || sy > rect->bottom) continue;
    auto *out = reinterpret_cast<lv_color_t *>(t.photo->pixels) + y * t.width;
    for (int x = 0; x < t.width; ++x) {
      int sx = t.crop_x + x * t.crop_w / t.width;
      if (sx < rect->left || sx > rect->right) continue;
      int index = ((sy - rect->top) * (rect->right - rect->left + 1) + sx - rect->left) * 3;
      out[x] = lv_color_make(rgb[index], rgb[index+1], rgb[index+2]);
    }
  }
  return 1;
}
#endif

bool photo_image_load(const char *path, PhotoImage &photo, int max_w, int max_h, bool crop) {
  fs::File file = SD_MMC.open(path, FILE_READ);
  if (!file || max_w <= 0 || max_h <= 0) return false;
  uint8_t signature[2];
  if (file.read(signature, 2) != 2 || !file.seek(0)) return false;
  if (signature[0] == 'B' && signature[1] == 'M')
    return load_bmp(file, photo, max_w, max_h, crop);
#if LV_USE_SJPG
  if (signature[0] == 0xFF && signature[1] == 0xD8) {
    auto *work = static_cast<uint8_t *>(malloc(4096));
    if (!work) return false;
    JDEC decoder{};
    RasterTarget t{&file, &photo};
    bool ok = jd_prepare(&decoder, jpeg_read, work, 4096, &t) == JDR_OK;
    if (ok) {
      t.source_w = decoder.width; t.source_h = decoder.height;
      ok = prepare_target(t, max_w, max_h, crop);
    }
    if (ok) {
      uint8_t scale = 0;
      while (scale < 3 && (t.source_w >> (scale+1)) >= t.width &&
             (t.source_h >> (scale+1)) >= t.height) ++scale;
      t.source_w >>= scale; t.source_h >>= scale;
      // Compute the sampling coordinates in the decoder's scaled image.
      t.crop_x >>= scale; t.crop_y >>= scale;
      t.crop_w >>= scale; t.crop_h >>= scale;
      ok = jd_decomp(&decoder, jpeg_write, scale) == JDR_OK;
    }
    free(work);
    return ok;
  }
#endif
  return false;
}

void photo_image_free(PhotoImage &photo) {
  lv_img_cache_invalidate_src(&photo.image);
  free(photo.pixels);
  photo = PhotoImage{};
}
