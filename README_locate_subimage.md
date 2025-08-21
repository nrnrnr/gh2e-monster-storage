
# Image-in-Image Locator (with Transparency & Approximate Match)

This tool finds where a smaller image (PNG with transparency recommended) appears inside a larger image,
even when the two images come from different photos (angle/scale/lighting changes).

## How it works
- **Feature matching (SIFT by default)** between the template (masked by alpha) and the scene.
- **RANSAC homography** to get a robust geometric alignment.
- **Polygon output** giving the projected quadrilateral where the template lands in the scene.
- Optional **fallback** to masked template matching for near-exact cases.

## Install
```bash
python -m venv venv
. venv/bin/activate
pip install -r requirements.txt
```

## Run: single scene, many templates
```bash
python find_subimage.py   --scene path/to/scene.jpg   --templates_dir path/to/templates   --out outdir   --draw-matches
```

## Run: many scenes, many templates
```bash
python find_subimage.py   --scenes_dir path/to/scenes   --templates_dir path/to/templates   --out outdir
```

## Notes
- **Transparency respected**: keys points are only detected where alpha > 0 in your template.
- **When to use fallback**: if the two photos are near-identical scale/rotation and features fail, the fallback might still produce a helpful bounding box via correlation.
- **Detectors**: SIFT (robust; default), ORB (faster), AKAZE (middle-ground).

## Output
- `results.csv` with one row per (scene, template) including the found polygon and match stats.
- Optional `*_vis.png` files showing the match.
- Optional `results.json` with full detail.

## Tips for better matches
- Crop your template tight around the object (remove background from the template if possible).
- Higher-resolution images often yield more/better keypoints.
- If SIFT is too slow, try `--detector ORB`.
- Tune `--ratio` (default 0.75) and `--ransac` (default 3.0 px) if matches are noisy.
