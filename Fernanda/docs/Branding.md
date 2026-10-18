# Branding

Fernanda's icon, colors, and banner, and how the icon files are made. For the app's own look (its style and palette roles), see "The style" in [Architecture.md](Architecture.md).

## The icon

A ring, drawn in `resources/icons/designs/Fernanda.svg`.

- **Canvas**: 640 × 640
- **Outer circle**: radius 288, centered at (320, 320)
- **Inner circle** (the hole): radius 160, centered at (320, 320)

Only the ring on the canvas is exported. The first icon, a flat orange ring (`#FF7900`), is kept in the SVG off the canvas, locked, as the template and a reminder of the plain first design.

### Fill

A linear gradient from the ring's top-left edge to its bottom-right edge, from (116.35, 116.35) to (523.65, 523.65), where a 45° line meets the outer circle. The colors are old Fernanda's "pastels", spaced as its color bar spaced them: short ends, and a long blend from sky blue to periwinkle across the middle.

| Offset | Color |
|---|---|
| 0 | `#7CE1F9` |
| 25% | `#3BB0F3` |
| 75% | `#9194F2` |
| 100% | `#F9B3F9` |

### Fill options

The same four colors can be spaced three ways. [FillOptions.png](../resources/icons/designs/FillOptions.png) shows each with the Deep periwinkle and Dark gradient strokes, at 16, 32, 64, and 130 px on light and dark.

| Layout | Offsets | How it reads |
|---|---|---|
| **Old bar** (in use) | 0, 25%, 75%, 100% | Sky blue fades into periwinkle across the whole middle, as old Fernanda's color bar did |
| Even | 0, 33%, 67%, 100% | Each color gets an equal share; the most varied of the three |
| Held periwinkle | 0, 25%, 50%, 75%, 100%, with `#9194F2` at both 50% and 75% | A flat periwinkle band across the lower right; the most purple of the three |

At 16–32 px all three average out to nearly the same blue-violet; the difference shows from 64 px up.

### Stroke

The Dark gradient below, 12.8 wide, on both edges of the ring: each color is a darker version of the fill color at the same offset, so the stroke reads as the ring's own edge rather than an outline. Its stops and end points are the fill's.

| Offset | Color |
|---|---|
| 0 | `#2A9AC8` |
| 25% | `#1A7FC4` |
| 75% | `#5A5EC8` |
| 100% | `#C46AC8` |

A stroke is centered on its edge, so it also makes the ring slightly wider than its circles: outer edge about 294, hole about 154.

### Stroke options

The options tried, at the same width. [StrokeOptions.png](../resources/icons/designs/StrokeOptions.png) shows each at 16, 32, 64, and 140 px on light and dark, over the held-periwinkle fill.

| Option | Stroke | How it reads |
|---|---|---|
| Pale | `#DCE7FF` | A rim on dark; almost gone on white |
| White 60% | `#FFFFFF` at 60% opacity | Like Pale, a little cleaner on dark |
| Deep periwinkle | `#5A5EC8` | Shows on both; one flat color around the ring, which reads as drawn on where it meets the cyan |
| **Dark gradient** (in use) | Below | Shows on both; follows the fill, so it reads as the ring's own edge |
| Light gradient | Below | A rim on dark; soft on white |
| Navy 35% | `#141E50` at 35% opacity | Crisp on white; gone on dark |

A gradient stroke has a color for each fill color, at the fill's offsets, along the fill's line from (116.35, 116.35) to (523.65, 523.65). If the stroke's stops or end points differ from the fill's, its colors change in different places and the edge stops tracking the fill.

| Fill | Dark gradient | Light gradient |
|---|---|---|
| `#7CE1F9` | `#2A9AC8` | `#C8F3FF` |
| `#3BB0F3` | `#1A7FC4` | `#A8DCFF` |
| `#9194F2` | `#5A5EC8` | `#CFD0FF` |
| `#F9B3F9` | `#C46AC8` | `#FFE0FF` |

With the held-periwinkle fill, a gradient stroke holds its third color across 50% and 75% too.

A stroke lighter than the background disappears on white, and one darker than the background disappears on dark. The Dark gradient is darker than the fill and lighter than a dark background, so it shows on both; its third color is the Deep periwinkle.

### Making the icon files

Drag `Fernanda.svg` onto `resources/icons/designs/DragForIconsFromSVG.bat`. It writes every PNG size, `Fernanda.ico`, and `Fernanda.icns` to an `out` folder beside it; copy them into `resources/icons/`.

- ImageMagick renders the PNGs and the `.ico`. The script installs it with winget if it's missing.
- ImageMagick can't write `.icns`, so `MakeIcns.ps1` packs the PNGs into one. An `.icns` holds PNGs unchanged, so it needs nothing else installed.

## The banner

GitHub's social preview, `resources/social/banner/GitHubSocialBanner.jpg`, made from `GitHubSocialBanner.kra` in Krita. It's uploaded by hand under the repo's Settings → Social preview; GitHub doesn't read it from the repo.

- **Size**: 1280 × 640
- **Background**: `BG4.png`, a dark slate gradient from `#293642` at the top-left to `#181C27` at the bottom-right. `BG1`–`BG3` are the alternatives kept in the file
- **Typeface**: Literata, for both the name and the tagline
- **Name**: `#FAF6EE`
- **Tagline**: `#ECE5D8`
- **Icon**: `Fernanda-2048.png`, scaled

Everything is centered horizontally, with the ring at the middle of the canvas.
