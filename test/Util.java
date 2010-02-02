
/* Some handy facilities for Jupiter programs */

//package jupiter;

import java.lang.*;

public class Util{
	public native static void printInt(int i);
	public native static void printIntAndDouble(int i, double d);
	public native static void printString(String s);
	public static final int ANY_LINE = -1;
	public native static void fileTrap(String fileName, int lineNumber);
	public native static void functionTrap(String fileName, int lineNumber);
	public native static void emptyLoop(int iterCount);
}

