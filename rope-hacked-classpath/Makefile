rope-hacked-classpath.so : debug/rope-hacked-classpath.so
	rm -f rope-hacked-classpath.so
	cp debug/rope-hacked-classpath.so .

debug/rope-hacked-classpath.so : Math.cpp Math.h
	epm b

Math.h : Math.java
	javac -g -cp .:$(ROPECLASSPATH) Math.java
	rm Math.h
	javah -jni Math
