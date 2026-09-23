import sys
from pathlib import Path

WIDTH = 84
HEIGHT = 64
LEVELS = 16

SCRIPT_DIR = Path(__file__).resolve().parent
PACKAGE_ROOT = SCRIPT_DIR.parent.parent
PHOTO_HEADER = PACKAGE_ROOT / "graphics" / "PhotoData.h"
PREVIEW_IMAGE = SCRIPT_DIR / "preview.png"

try:
    from PIL import Image, ImageOps, ImageDraw
except ImportError:
    sys.exit("Pillow is required:  pip3 install pillow")

def make_placeholder():

    img = Image.new("L", (WIDTH * 4, HEIGHT * 4), 0)
    d = ImageDraw.Draw(img)
    cx, cy = WIDTH * 2, HEIGHT * 2.55

    d.ellipse((cx - 90, cy - 55, cx + 90, cy + 75), fill=230)

    for dx, dy, r in ((-105, -95, 42), (-38, -128, 44), (38, -128, 44), (105, -95, 42)):
        d.ellipse((cx + dx - r, cy + dy - r, cx + dx + r, cy + dy + r), fill=230)
    return img.resize((WIDTH, HEIGHT), Image.LANCZOS)

def load_photo(path):
    img = Image.open(path)
    img = ImageOps.exif_transpose(img).convert("L")
    img = ImageOps.autocontrast(img, cutoff=1)

    img.thumbnail((WIDTH, HEIGHT), Image.LANCZOS)
    canvas = Image.new("L", (WIDTH, HEIGHT), 0)
    canvas.paste(img, ((WIDTH - img.width) // 2, (HEIGHT - img.height) // 2))
    return canvas

def dither_to_levels(img):

    px = [[float(img.getpixel((x, y))) for x in range(WIDTH)] for y in range(HEIGHT)]
    out = [[0] * WIDTH for _ in range(HEIGHT)]
    step = 255.0 / (LEVELS - 1)
    for y in range(HEIGHT):
        for x in range(WIDTH):
            old = px[y][x]
            new = round(old / step)
            new = max(0, min(LEVELS - 1, new))
            out[y][x] = new
            err = old - new * step
            if x + 1 < WIDTH:
                px[y][x + 1] += err * 7 / 16
            if y + 1 < HEIGHT:
                if x > 0:
                    px[y + 1][x - 1] += err * 3 / 16
                px[y + 1][x] += err * 5 / 16
                if x + 1 < WIDTH:
                    px[y + 1][x + 1] += err * 1 / 16
    return out

def emit_header(pixels, source_name):
    lines = []
    lines.append("#pragma once")
    lines.append("#include <stdint.h>")
    lines.append("")
    lines.append("static const int kPhotoWidth = %d;" % WIDTH)
    lines.append("static const int kPhotoHeight = %d;" % HEIGHT)
    lines.append("static const uint8_t kPhotoPixels[%d] = {" % (WIDTH * HEIGHT))
    for y in range(HEIGHT):
        row = ",".join(str(v) for v in pixels[y])
        lines.append("  " + row + ",")
    lines.append("};")
    PHOTO_HEADER.parent.mkdir(parents=True, exist_ok=True)
    with PHOTO_HEADER.open("w") as f:
        f.write("\n".join(lines) + "\n")
    print("Wrote %s (%d x %d)" % (PHOTO_HEADER, WIDTH, HEIGHT))

def emit_preview(pixels, path=PREVIEW_IMAGE):
    img = Image.new("L", (WIDTH, HEIGHT))
    for y in range(HEIGHT):
        for x in range(WIDTH):
            img.putpixel((x, y), int(pixels[y][x] * 255 / (LEVELS - 1)))
    img.resize((WIDTH * 4, HEIGHT * 4), Image.NEAREST).save(path)
    print("Wrote %s (4x preview of what the display will show)" % path)

if __name__ == "__main__":
    if len(sys.argv) > 1:
        src = sys.argv[1]
        img = load_photo(src)
    else:
        src = "(placeholder paw print)"
        img = make_placeholder()
    pixels = dither_to_levels(img)
    emit_header(pixels, src)
    emit_preview(pixels)
