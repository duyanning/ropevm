public class StaticInitA {
	static { System.out.println("Initializing A"); }
	StaticInitA() { System.out.println("Constructing A"); }
	static StaticInitB b = new StaticInitB();
	static { System.out.println("Done initializing A"); }
}

