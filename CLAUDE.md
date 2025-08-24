We're processing images of board-game pieces.  An image, for example `monstersA.jpg`, is processed as follows:

 1. Create file `ruler150A.jpg` by cropping the original image to the width of the 150mm ruler in the image.  The ruler will be used to compute the number of pixels per millimeter in the image.  This number is different for each image.  A person will perform this step.

 2. Run the original image `monstersA.jpg` through a blur-threshold-despeckle pipeline to create a bitmap `monstersA-despect.pbm`.  This is handled by mkfile rules.  In the resulting bitmap each board-game piece is solid black.

 3. Find regions using the `find-region` program, which writes `regionsA.txt`, to hold the bounding box of each black region.
 
    (Optionally, the `regionsA.txt` file can be used to number each region.  `number-regions monstersA.txt` should create `monstersA-regions.jpg`, in which a region number is superimposed over the center of each region in the original photo.)

 4. Extract the regions using the `regions` script.  There is an example in the mkfile.

 5. Each black region will then be transmogrified using `transmogrify`.  This script embiggens the region by an amount in millimeters specified on the command line using the `-embiggen` option, or if the option is not given, defaults to `0.9`.
    The embiggened region is then cropped.  The embiggened region is then traced by `potrace`.  That trace is then used to extract tikz and openscad info.
 
**Important**: Any time we take a new image-processing step, let me look at samples before proceeding to the next step.

**Script conventions**: Scripts output data to stdout, error/status messages to stderr. This allows pipeline composition.


Test scripts
------------
All test scripts created by Claude go in the `tests` directory.
All test outputs go in the `testout` directory.
When we seem to be done with a test script, Claude should move it to `obsolete-tests`.

Details
-------
The start of a pipeline is a photograph, which has a base name (for now, `monsters`), a suffix A, B, C, and so on, and finally `.jpg`.


- all things built by scripts, like a numbered jpg, go in the build directory.
- Never modify monstersA.jpg, or any similar file with a different letter.




Translation of tikz to scad
---------------------------

Each tikz picture will correspond to a module; call them tray1 and tray2.
Each element of a picture will be rendered using the `well` module as 
shown in the command in well.scad.  The argument to `well` will be taken 
from file `counts.txt`; any monster with a count of 10 should have two 
instances, each of which to get a well of size 5.  A monster should have 
two instances if and only if it has a count of 10; if you see an 
inconsistency, the translation should halt with an error message.
Each of the following commands in tray.tex should translate to a single 
well: \fullmonster, \mone, \mtwo, \monster, \xfullmonster, \monsterx, and
 anything similar beginning with "tilt."  Each \monsterpair command 
should be translated into two wells.
