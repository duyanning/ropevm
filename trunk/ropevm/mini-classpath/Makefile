all: mini-classpath.so Math.class Random.class MiniClassPathHelper.class


mini-classpath.so : debug/mini-classpath.so
	ln -fs debug/mini-classpath.so

debug/mini-classpath.so : Math.cpp Math.h
	epm b

Math.h : Math.class
	-rm Math.h
	javah -jni Math


%.class : %.java
	javac -g -cp .:$(ROPE_CLASSES) $<
