PTB_TESTFILES = $(wildcard *.ptb)

%.info: %.ptb ../ptbinfo
	../ptbinfo "$<" > "$@"

%.ly: %.ptb ../ptb2ly
	../ptb2ly "$<"

%.txt: %.ptb ../ptb2ascii
	../ptb2ascii "$<"

%.ly: %.gp4 ../gp2ly
	../gp2ly "$<"

%.ly: %.gp3 ../gp2ly
	../gp2ly "$<"

%.pdf: %.ly
	lilypond-snapshot --pdf "$*"

%.png: %.ly
	lilypond-snapshot --png "$*"

%.xml: %.ptb ../ptb2xml
	../ptb2xml "$<"

%.music.xml: %.ptb ../ptb2xml ../ptbxml2musicxml.xsl
	../ptb2xml -m "$<" -o "$@"

%.ptb.2: %.ptb ../ptb2ptb
	../ptb2ptb "$<"

all: ly ptb info txt xml musicxml 
info: $(patsubst %.ptb,%.info,$(PTB_TESTFILES))
txt: $(patsubst %.ptb,%.txt,$(PTB_TESTFILES))
xml: $(patsubst %.ptb,%.xml,$(PTB_TESTFILES))
musicxml: $(patsubst %.ptb,%.music.xml,$(PTB_TESTFILES))
ptb: $(patsubst %.ptb,%.ptb.2,$(PTB_TESTFILES))
pdf: $(patsubst %.ptb,%.pdf,$(PTB_TESTFILES))
ly: $(patsubst %.ptb,%.ly,$(PTB_TESTFILES))
clean: 
	rm -f *.info *.ly *.txt *.pdf *.xml *.ptb.2
