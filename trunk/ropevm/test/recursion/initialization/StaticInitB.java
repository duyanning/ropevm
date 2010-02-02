class StaticInitB {
	static { System.out.println("Initializing B"); }
	StaticInitB() { System.out.println("Constructing B"); }
	static StaticInitA a = new StaticInitA();
	static { System.out.println("Done initializing B"); }
}

