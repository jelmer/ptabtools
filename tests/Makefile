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
	lilypond-snapshot "$<"

all: $(patsubst %.ptb,%.info,$(PTB_TESTFILES))
clean: 
	rm -f *.info *.ly *.txt *.pdf
