class Graph {
	public void init() {
		
	}

	public void adjust() {
		
	}
	
}

class Page {
	public void prepare() {
		
	}

	public void layout() {
		
	}
}

class Fixer {
	public void tune(Graph g, Page p) {
		g.init();
		p.prepare();
		g.adjust();
		p.layout();
	}
	
	public static void main(String[] args) {
		Fixer fixer = new Fixer();
		fixer.tune(new Graph(), new Page());
	}
}
