all: mini-classpath.so Math.class


mini-classpath.so : debug/mini-classpath.so
	ln -fs debug/mini-classpath.so

debug/mini-classpath.so : Math.cpp Math.h
	epm b

Math.h : Math.java
	javac -g -cp .:$(ROPE_CLASSES) Math.java
	-rm Math.h
	javah -jni Math


Math.class : Math.java
	javac -g -cp .:$(ROPE_CLASSES) Math.java
