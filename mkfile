MKSHELL=/bin/ksh
B=build

all:V: 

REGIONSA=A01 A02 A03 A04 A05 A06 A07 A08 A09 A10 A11
REGIONSB=B01 B02 B03 B04 B05 B06 B07
REGIONSC=C01 C02 C03 C04 C05 C06 C07 C08 C09 C10 C11
REGIONSD=D01 D02 D03 D04 D05 D06 D07 D08 D09 D10 D11

<|./pbmrules A B C D

TEX=${REGIONSA:%=$B/region%.tex} ${REGIONSB:%=$B/region%.tex} \
    ${REGIONSC:%=$B/region%.tex} ${REGIONSD:%=$B/region%.tex}


THRESHA=0.79
THRESHB=0.76
THRESHC=0.725
THRESHD=0.755

tex:V: $TEX

metrics%:V: dpi%.float dpi%.int ppm%.int

dpi%.float:D: ruler150%.jpg
	lua5.1 -e "local width = $(identify -format "%w" $prereq)
	           print(width / (150.0 / 25.4))" > $target

dpi%.int:D: dpi%.float
	lua5.1 -e "local width = $(cat $prereq)
	           print(math.floor(width + 0.5))" > $target

ppm%.int:D: ruler150%.jpg # pixels per meter
	lua5.1 -e "local width = $(identify -format "%w" $prereq)
	           print(math.floor(1000 * width / 150.0))" > $target

#$B/%-unblue.png: %.jpg unblue
#	unblue 200 5 $stem.jpg $target

#$B/%A-thresh.pbm:D: $B/%A-blur5.jpg threshold
#	./threshold 0.79 $B/${stem}A-blur5.jpg > $target
#
#$B/%B-thresh.pbm:D: $B/%B-blur5.jpg threshold
#	./threshold 0.76 $B/${stem}B-blur5.jpg > $target

$B/monsters%-thresh.pbm:D: $B/monsters%-blur5.jpg threshold
	T=$(eval 'echo $THRESH'"$stem")
	./threshold "$T" $B/monsters${stem}-blur5.jpg > $target

$B/%-despeck.pbm:D: $B/%-thresh.pbm despeckle
	set -o pipefail
	./despeckle 20000 $B/$stem-thresh.pbm |
        unblackedges > $target

#regions-%:V: $B/monsters%-despeck.pbm $B/regions-monsters%.txt
#	./regions $prereq



#$B/%.json: $B/%.pbm transmogrify
#	./transmogrify $B/$stem.pbm

^$B/(.*([A-Z]).*)'\.'json'$':R: $B/'\1'.pbm transmogrify metrics'\2'
	./transmogrify $B/$stem1.pbm

$B/%.tex:D: $B/%.json ungeojson
	ungeojson -tikz $B/$stem.json > $target

$B/%.scad:D: $B/%.json ungeojson
	ungeojson -scae $B/$stem.json > $target

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

