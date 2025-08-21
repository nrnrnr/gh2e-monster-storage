
#!/usr/bin/env python3
"""
find_subimage.py
----------------
Locate a (semi-)matching smaller image (with transparency) inside a larger scene image,
robust to different photographs (lighting/rotation/scale/partial occlusion).

Approach
========
- Use a *feature-based* matcher (SIFT by default). This is far more robust than raw
  template matching when the photos differ.
- Respect transparency in the template: keypoints are detected only where alpha > 0.
- Estimate a homography via RANSAC from good feature matches.
- If successful, we project the template's corners to the scene and output that quadrilateral.
- If feature matching fails (e.g., very textureless image), we optionally fall back to
  masked template matching (best if scale/rotation are similar).

Outputs
=======
- PNGs with an overlay showing the projected template quadrilateral (and inlier matches if requested).
- A CSV file summarizing results for each (scene, template) pair.
- Optional JSON with full details.

Usage
=====
Basic (a single scene and a directory of templates):
    python3 find_subimage.py --scene path/to/scene.jpg --templates_dir path/to/templates --out outdir

Batch (multiple scenes and templates):
    python3 find_subimage.py --scenes_dir path/to/scenes --templates_dir path/to/templates --out outdir

Switching detectors (SIFT|ORB|AKAZE):
    python3 find_subimage.py --scene scene.jpg --templates_dir T --detector ORB --out outdir

Show debug visualizations:
    python3 find_subimage.py --scene scene.jpg --templates_dir T --draw-matches --out outdir

Dependencies
============
- Python 3.9+
- opencv-python>=4.5
- numpy
- pillow (only used for some I/O conveniences)

"""
import argparse
import csv
import json
import sys
from pathlib import Path
from typing import Tuple, Optional, Dict, Any, List

import cv2
import numpy as np

# --------------------------- Utilities ---------------------------

def imread_any(path: Path, flags=cv2.IMREAD_UNCHANGED) -> np.ndarray:
    img = cv2.imread(str(path), flags)
    if img is None:
        raise FileNotFoundError(f"Could not read image: {path}")
    return img

def to_gray(img: np.ndarray) -> np.ndarray:
    if img.ndim == 2:
        return img
    if img.shape[2] == 4:  # BGRA
        bgr = cv2.cvtColor(img, cv2.COLOR_BGRA2BGR)
        return cv2.cvtColor(bgr, cv2.COLOR_BGR2GRAY)
    return cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

def alpha_mask_from_png(img_rgba: np.ndarray, thresh: int = 10) -> Optional[np.ndarray]:
    if img_rgba.ndim == 3 and img_rgba.shape[2] == 4:
        alpha = img_rgba[:, :, 3]
        mask = (alpha > thresh).astype(np.uint8) * 255
        return mask
    return None

def create_detector(name: str, nfeatures: int = 4000):
    name = name.upper()
    if name == "SIFT":
        return cv2.SIFT_create(nfeatures=nfeatures)
    if name == "ORB":
        # ORB is faster, binary descriptors
        return cv2.ORB_create(nfeatures=nfeatures, scoreType=cv2.ORB_HARRIS_SCORE)
    if name == "AKAZE":
        return cv2.AKAZE_create()
    raise ValueError(f"Unknown detector: {name}")

def norm_for_detector(name: str):
    name = name.upper()
    if name in ("SIFT", "AKAZE"):
        return cv2.NORM_L2
    if name == "ORB":
        return cv2.NORM_HAMMING
    raise ValueError(f"Unknown detector: {name}")

def ratio_test(matches_knn, ratio=0.75):
    good = []
    for m in matches_knn:
        if len(m) == 2:
            if m[0].distance < ratio * m[1].distance:
                good.append(m[0])
    return good

def points_from_matches(kp1, kp2, matches):
    src = np.float32([kp1[m.queryIdx].pt for m in matches]).reshape(-1, 1, 2)
    dst = np.float32([kp2[m.trainIdx].pt for m in matches]).reshape(-1, 1, 2)
    return src, dst

def polygon_from_homography(w: int, h: int, H: np.ndarray) -> np.ndarray:
    corners = np.float32([[0,0], [w,0], [w,h], [0,h]]).reshape(-1,1,2)
    poly = cv2.perspectiveTransform(corners, H).reshape(-1,2)
    return poly  # shape (4,2)

def draw_polygon(img_bgr: np.ndarray, poly: np.ndarray, color=(0,255,0), thickness=3):
    poly_i = poly.astype(int)
    cv2.polylines(img_bgr, [poly_i], isClosed=True, color=color, thickness=thickness)

def masked_template_match(scene_gray: np.ndarray, tmpl_gray: np.ndarray, mask: Optional[np.ndarray] = None):
    # Normalized cross-correlation with optional mask (OpenCV supports it for TM_CCORR_NORMED only).
    method = cv2.TM_CCORR_NORMED
    if mask is not None and mask.any():
        res = cv2.matchTemplate(scene_gray, tmpl_gray, method, mask=mask)
    else:
        res = cv2.matchTemplate(scene_gray, tmpl_gray, method)
    min_val, max_val, min_loc, max_loc = cv2.minMaxLoc(res)
    top_left = max_loc
    h, w = tmpl_gray.shape
    poly = np.array([[top_left[0], top_left[1]],
                     [top_left[0]+w, top_left[1]],
                     [top_left[0]+w, top_left[1]+h],
                     [top_left[0], top_left[1]+h]], dtype=np.float32)
    return {"score": float(max_val), "poly": poly}

# --------------------------- Core logic ---------------------------

def locate_one(scene_path: Path,
               tmpl_path: Path,
               detector_name: str = "SIFT",
               ratio: float = 0.75,
               min_matches: int = 8,
               ransac_reproj_thresh: float = 3.0,
               fallback_template: bool = True,
               draw_matches: bool = False) -> Dict[str, Any]:

    scene_raw = imread_any(scene_path, cv2.IMREAD_UNCHANGED)
    tmpl_raw  = imread_any(tmpl_path,  cv2.IMREAD_UNCHANGED)

    scene_gray = to_gray(scene_raw)
    tmpl_gray  = to_gray(tmpl_raw)
    tmpl_mask  = alpha_mask_from_png(tmpl_raw)

    detector = create_detector(detector_name)
    norm = norm_for_detector(detector_name)
    matcher = cv2.BFMatcher(norm)

    kp_t, des_t = detector.detectAndCompute(tmpl_gray, tmpl_mask)
    kp_s, des_s = detector.detectAndCompute(scene_gray, None)

    result: Dict[str, Any] = {
        "scene": str(scene_path),
        "template": str(tmpl_path),
        "detector": detector_name,
        "status": "failed",
    }

    if des_t is None or des_s is None or len(kp_t) == 0 or len(kp_s) == 0:
        result["reason"] = "no_descriptors"
        if fallback_template:
            mt = masked_template_match(scene_gray, tmpl_gray, tmpl_mask)
            result.update({
                "status": "template_match",
                "score": mt["score"],
                "poly": mt["poly"].tolist(),
            })
        return result

    matches_knn = matcher.knnMatch(des_t, des_s, k=2)
    good = ratio_test(matches_knn, ratio=ratio)

    result["num_kp_tmpl"] = len(kp_t)
    result["num_kp_scene"] = len(kp_s)
    result["num_matches_raw"] = len(matches_knn)
    result["num_matches_good"] = len(good)

    if len(good) < max(min_matches, 4):
        result["reason"] = f"not_enough_good_matches({len(good)})"
        if fallback_template:
            mt = masked_template_match(scene_gray, tmpl_gray, tmpl_mask)
            result.update({
                "status": "template_match",
                "score": mt["score"],
                "poly": mt["poly"].tolist(),
            })
        return result

    src_pts, dst_pts = points_from_matches(kp_t, kp_s, good)
    H, mask_inliers = cv2.findHomography(src_pts, dst_pts, cv2.RANSAC, ransac_reproj_thresh)
    if H is None or mask_inliers is None or int(mask_inliers.sum()) < 4:
        result["reason"] = "homography_failed"
        if fallback_template:
            mt = masked_template_match(scene_gray, tmpl_gray, tmpl_mask)
            result.update({
                "status": "template_match",
                "score": mt["score"],
                "poly": mt["poly"].tolist(),
            })
        return result

    # Compute polygon
    h_t, w_t = tmpl_gray.shape
    poly = polygon_from_homography(w_t, h_t, H)

    # Compute a crude reprojection error
    inliers = mask_inliers.ravel().astype(bool)
    reproj_err = None
    try:
        src_in = src_pts[inliers].reshape(-1,2)
        dst_in = dst_pts[inliers].reshape(-1,2)
        src_proj = cv2.perspectiveTransform(src_pts[inliers], H).reshape(-1,2)
        diffs = src_proj - dst_in
        reproj_err = float(np.sqrt((diffs**2).sum(axis=1)).mean()) if len(diffs) else None
    except Exception:
        pass

    result.update({
        "status": "homography",
        "poly": poly.tolist(),
        "inliers": int(mask_inliers.sum()),
        "reproj_error": reproj_err,
        "good_match_ratio": float(len(good) / max(1, len(matches_knn))),
    })

    # Optionally build a drawn visualization
    if draw_matches:
        scene_vis = scene_raw.copy()
        if scene_vis.ndim == 2:
            scene_vis = cv2.cvtColor(scene_vis, cv2.COLOR_GRAY2BGR)
        elif scene_vis.shape[2] == 4:
            scene_vis = cv2.cvtColor(scene_vis, cv2.COLOR_BGRA2BGR)

        draw_polygon(scene_vis, poly, color=(0, 255, 0), thickness=3)

        # Draw matches (only inliers for clarity)
        draw_params = dict(matchColor=(0, 255, 0),
                           singlePointColor=(255, 0, 0),
                           matchesMask=mask_inliers.ravel().tolist(),
                           flags=cv2.DrawMatchesFlags_NOT_DRAW_SINGLE_POINTS)
        match_img = cv2.drawMatches(tmpl_gray, kp_t, scene_vis, kp_s, good, None, **draw_params)
        result["vis_image"] = match_img

    return result

def save_visualization(result: Dict[str, Any], outdir: Path, basename: str):
    if "vis_image" in result and isinstance(result["vis_image"], np.ndarray):
        out_path = outdir / f"{basename}_vis.png"
        cv2.imwrite(str(out_path), result["vis_image"])

def main():
    ap = argparse.ArgumentParser(description="Locate a template (with transparency) inside a larger scene image.")
    g_in = ap.add_mutually_exclusive_group(required=True)
    g_in.add_argument("--scene", type=Path, help="Path to one scene image")
    g_in.add_argument("--scenes_dir", type=Path, help="Directory of scene images")

    ap.add_argument("--templates_dir", type=Path, required=True, help="Directory of template images (PNGs with alpha preferred)")
    ap.add_argument("--out", type=Path, required=True, help="Output directory")
    ap.add_argument("--detector", choices=["SIFT", "ORB", "AKAZE"], default="SIFT")
    ap.add_argument("--ratio", type=float, default=0.75, help="Lowe ratio test threshold")
    ap.add_argument("--min-matches", type=int, default=8, help="Minimum good matches before attempting homography")
    ap.add_argument("--ransac", type=float, default=3.0, help="RANSAC reprojection threshold (pixels)")
    ap.add_argument("--no-fallback", action="store_true", help="Disable fallback masked template matching")
    ap.add_argument("--draw-matches", action="store_true", help="Save a visualization with matches and polygon overlay")
    ap.add_argument("--json", action="store_true", help="Also emit a JSON file with detailed results")
    ap.add_argument("--exts", nargs="+", default=[".png", ".jpg", ".jpeg", ".tif", ".tiff", ".bmp", ".webp"], help="Image extensions to consider")
    args = ap.parse_args()

    outdir = args.out
    outdir.mkdir(parents=True, exist_ok=True)

    # Gather inputs
    if args.scene:
        scenes = [args.scene]
    else:
        scenes = [p for p in args.scenes_dir.iterdir() if p.suffix.lower() in set(e.lower() for e in args.exts)]
        scenes.sort()

    templates = [p for p in args.templates_dir.iterdir() if p.suffix.lower() in set(e.lower() for e in args.exts)]
    templates.sort()

    if not scenes:
        print("No scenes found.", file=sys.stderr)
        sys.exit(2)
    if not templates:
        print("No templates found.", file=sys.stderr)
        sys.exit(2)

    csv_path = outdir / "results.csv"
    with open(csv_path, "w", newline="") as fcsv:
        writer = csv.writer(fcsv)
        writer.writerow([
            "scene","template","status","detector",
            "num_kp_scene","num_kp_tmpl","num_matches_raw","num_matches_good",
            "inliers","reproj_error","good_match_ratio","poly","score"
        ])

        all_results: List[Dict[str, Any]] = []
        for scene in scenes:
            for tmpl in templates:
                base = f"{scene.stem}__{tmpl.stem}"
                try:
                    res = locate_one(
                        scene, tmpl,
                        detector_name=args.detector,
                        ratio=args.ratio,
                        min_matches=args.min_matches,
                        ransac_reproj_thresh=args.ransac,
                        fallback_template=not args.no_fallback,
                        draw_matches=args.draw_matches
                    )
                except Exception as e:
                    res = {
                        "scene": str(scene),
                        "template": str(tmpl),
                        "status": "error",
                        "detector": args.detector,
                        "error": repr(e)
                    }

                # Save visualization if present
                save_visualization(res, outdir, base)

                # CSV row
                writer.writerow([
                    res.get("scene"),
                    res.get("template"),
                    res.get("status"),
                    res.get("detector"),
                    res.get("num_kp_scene"),
                    res.get("num_kp_tmpl"),
                    res.get("num_matches_raw"),
                    res.get("num_matches_good"),
                    res.get("inliers"),
                    res.get("reproj_error"),
                    res.get("good_match_ratio"),
                    json.dumps(res.get("poly")) if res.get("poly") is not None else None,
                    res.get("score"),
                ])
                all_results.append(res)

    if args.json:
        with open(outdir / "results.json", "w") as fj:
            json.dump(all_results, fj, indent=2)

    print(f"Done. Wrote: {csv_path}")
    if args.draw_matches:
        print("Visualizations saved as *_vis.png alongside the CSV.")

if __name__ == "__main__":
    main()
