result/speedups.txt: $(addsuffix /speedup.txt, $(DIRS))
	@rm -f result/speedups.txt
	@for i in $(DIRS); do																				\
		cd $$i;																							\
		awk '{ printf "%s\t%f\n" , prog , $$1 ; }' prog="$$i" speedup.txt >> ../result/speedups.txt;	\
		cd ..;																							\
	done
	@for i in $(?D); do								\
		cd $$i;										\
		rm -rf ../result/$$i-profile.txt;				\
		cp profile.txt ../result/$$i-profile.txt;	\
		cd ..;										\
	done
