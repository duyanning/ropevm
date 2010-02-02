
interface StaticInterface{
	public int one = 1;
	public Object foo = new Object();
}

class StaticInit implements StaticInterface{
	public static int two = one+1;
	public static int three;
	static{ three = one+two; }
	public static int four = three+one;
	public static int five;
	static{ five = four+one; }
	public final static int six=6;
}

