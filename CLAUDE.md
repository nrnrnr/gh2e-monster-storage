We're doing image processing.  We'll take a photograph of board-game pieces and do the following:

 1. Create a bitmap from the image with background pixels black and foreground pixels white.  The background is actually white paper with light blue rules, so light blue in the appropriate range should be forced to white.

 2. Despeckle the bitmap by flipping small regions.  E.g., a contiguous group of white pixels that does not contain enough pixels becomes black, and vice versa.

 3. We will then take each black region and calculate its bounding box.  Number each region and superimpose the numbers on the original photo.  A person will then label each numbered image.

 4. Each black region will then be extracted as an image, and its coordinates will be applied to the original image as well to get the foreground part with transparent background.  These images will be written to files using the labels from step 3.

 5. Each black image will be converted to geometry using `potrace` with the geojson back end.  Then that info will be converted both to a tikz path and to 2D geometry for openscad.
 
The processing will be orchestrated by a mkfile, which uses Andrew Hume's mk, a tool very similar to GNU Make, but with variable syntax as in the shell.

As we develop the tools and the mkfile, you will update these instructions with any refinements we develop interactively.
