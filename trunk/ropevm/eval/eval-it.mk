speedup.txt: $(wildcard *.java) cmd vmparams ../../debug/ropevm
	@eval-it.sh
