MKSHELL=/bin/ksh
B=build

REGIONSA=A01 A02 A03 A04 A05 A06 A07 A08 A09 A10 A11
PBMA=${REGIONSA:%=$B/region%.pbm}
TEXA=${REGIONSA:%=$B/region%.tex}
SCADA=${REGIONSA:%=$B/region%.scad}

all:V: 

tex:V: $TEXA

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

$B/%-thresh.pbm:D: $B/%-blur5.jpg threshold
	./threshold 0.79 $stem-blur5.jpg > $target

$B/%-despeck.pbm:D: $B/%-thresh.pbm despeckle
	set -o pipefail
	./despeckle 20000 $B/$stem-thresh.pbm |
        unblackedges > $target

#regions-%:V: $B/monsters%-despeck.pbm $B/regions-monsters%.txt
#	./regions $prereq

$PBMA:D: $B/monstersA-despeck.pbm $B/regions-monstersA.txt regions metricsA
	./regions $B/monstersA-despeck.pbm $B/regions-monstersA.txt

#$B/%.json: $B/%.pbm transmogrify
#	./transmogrify $B/$stem.pbm

^$B/(.*([A-Z]).*)'\.'json'$':R: $B/'\1'.pbm transmogrify metrics'\2'
	./transmogrify $B/$stem1.pbm

$B/%.tex:D: $B/%.json ungeojson
	ungeojson -tikz $B/$stem.json > $target

$B/%.scad:D: $B/%.json ungeojson
	ungeojson -scae $B/$stem.json > $target

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

$B/%-numbered.jpg: $B/regions-%.txt %.jpg number-regions
	./number-regions $B/regions-$stem.txt $stem.jpg $target

