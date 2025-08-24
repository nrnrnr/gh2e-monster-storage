MKSHELL=/bin/ksh
B=build

all:V: tray.pdf

REGIONSA=A01 A02 A03 A04 A05 A06 A07 A08 A09 A10 A11
REGIONSB=B01 B02 B03 B04 B05 B06 B07
REGIONSC=C01 C02 C03 C04 C05 C06 C07 C08 C09 C10 C11
REGIONSD=D01 D02 D03 D04 D05 D06 D07 D08 D09 D10 D11

REGIONSW=W01 W02 W03 W04 W05 W06 W07 W08 W09 W10 W11
REGIONSX=X01 X02 X03 X04 X05 X06 X07 X08 X09 X10 X11
REGIONSY=Y01 Y02 Y03 Y04 Y05 Y06 Y07 Y08 Y09 Y10 Y11
REGIONSZ=Z01 Z02 Z03 Z04 Z05 Z06 Z07

<|./pbmrules W X Y Z

TEX=${REGIONSW:%=$B/region%.tex} \
    ${REGIONSX:%=$B/region%.tex} \
    ${REGIONSY:%=$B/region%.tex} \
    ${REGIONSZ:%=$B/region%.tex} \

SCAD=${TEX:%.tex=%.scad}

THRESHA=0.79
THRESHB=0.76
THRESHC=0.725
THRESHD=0.755

THRESHW=0.89
THRESHX=0.90
THRESHY=0.88
THRESHZ=0.90

tex:V: outlines.tex

scad:V: $SCAD

tray.pdf:D: tray.tex outlines.tex
	latex-batch tray

label-sed:D: labels-to-sed labels.txt
	./labels-to-sed labels.txt

outlines.tex:D: $TEX label-sed link-named-regions
	./link-named-regions labels.txt
	set -o pipefail
        cat named/*.tex | ./label-sed > $target

outlines.scad:D: $SCAD label-sed
	set -o pipefail
        cat named/*.scad | ./label-sed > $target

outlines.scad:D: $SCAD 
        cat named/*.scad > $target

tray.scad:D: tray.tex tikz-to-scad
	tikz-to-scad tray.tex > $target

metrics%:V: $B/dpi%.float $B/dpi%.int $B/ppm%.int

$B/dpi%.float:D: ruler150%.jpg
	lua5.1 -e "local width = $(identify -format "%w" $prereq)
	           print(width / (150.0 / 25.4))" > $target

$B/dpi%.int:D: dpi%.float
	lua5.1 -e "local width = $(cat $prereq)
	           print(math.floor(width + 0.5))" > $target

$B/ppm%.int:D: ruler150%.jpg # pixels per meter
	lua5.1 -e "local width = $(identify -format "%w" $prereq)
	           print(math.floor(1000 * width / 150.0))" > $target

$B/monsters%-thresh.pbm:D: $B/monsters%-blur5.jpg threshold
	T=$(eval 'echo $THRESH'"$stem")
	./threshold "$T" $B/monsters${stem}-blur5.jpg > $target

$B/%-despeck.pbm:D: $B/%-thresh.pbm despeckle
	set -o pipefail
	./despeckle 160000 $B/$stem-thresh.pbm |
        unblackedges > $target


^$B/(.*([A-Z]).*)'\.'json'$':R: $B/'\1'.pbm transmogrify metrics'\2'
	./transmogrify $B/$stem1.pbm

$B/%.tex:D: $B/%.json ungeojson
	ungeojson -tikz $B/$stem.json > $target

$B/%.scad:D: $B/%.json ungeojson
	ungeojson -scad $B/$stem.json > $target

blur-%:V: $B/monsters%-blur5.jpg

$B/%-blur5.jpg: %.jpg
	convert $stem.jpg -gaussian-blur 0x5 $target

$B/%-blur7.jpg: %.jpg
	convert $stem.jpg -gaussian-blur 0x7 $target

find-regions: find-regions.c
	gcc -I/usr/include/netpbm -Wall -O2 -o $target $prereq -lnetpbm

$B/regions-%.txt: $B/%-despeck.pbm find-regions
	./find-regions $B/$stem-despeck.pbm $target

$B/%-coords.txt: $B/%-despeck.pbm find-regions
	./find-regions $stem-despeck.pbm $target

number-%:V: $B/monsters%-numbered.jpg

$B/%-numbered.jpg: $B/regions-%.txt %.jpg number-regions
	./number-regions $B/regions-$stem.txt $stem.jpg $target

