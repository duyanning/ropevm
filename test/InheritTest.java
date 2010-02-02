
abstract class C1{
	public abstract void c1a();
	public abstract void c1b();
	public void c1z(){ Util.printInt(101); }
	public void c1y(){ Util.printInt(102); }
	public void c1x(){ Util.printInt(103); }
	public void c1w(){ Util.printInt(104); }
}

interface I1{
	public void i1a();
	public void i1b();
}

abstract class C2 extends C1 implements I1{
	public void c1a(){ Util.printInt(201); }
	public void c1z(){ Util.printInt(202); }
	public void c1x(){ Util.printInt(206); }
	public void i1a(){ Util.printInt(203); }
	public abstract void c2a();
	public void c2z(){ Util.printInt(204); }
	public void c2y(){ Util.printInt(205); }
}

interface I2{
	public void i2a();
	public void i1a(); // Name collision with I1
}

interface I3 extends I2{
	public void i3a();
}

class C3 extends C2 implements I3{
	public void c1b(){ Util.printInt(301); }
	public void c1z(){ Util.printInt(302); }
	public void c1y(){ Util.printInt(303); }
	public void i1b(){ Util.printInt(304); }
	public void i2a(){ Util.printInt(305); }
	public void i3a(){ Util.printInt(306); }
	public void c2a(){ Util.printInt(307); }
	public void c2z(){ Util.printInt(308); }
}

class InheritTest{
	public static void main(String args[]){
		C3 c3 = new C3();
		c3.c1a(); // 201
		c3.c1b(); // 301
		c3.c1z(); // 302
		c3.c1y(); // 303
		c3.c1x(); // 206
		c3.c1w(); // 104
		c3.i1a(); // 203
		c3.i1b(); // 304
		c3.c2a(); // 307
		c3.c2z(); // 308
		c3.c2y(); // 205
		c3.i2a(); // 305
		c3.i3a(); // 306

		I3 i3 = c3;
		i3.i1a(); // 203
		i3.i2a(); // 305
		i3.i3a(); // 306

		I2 i2 = c3;
		i2.i1a(); // 203
		i2.i2a(); // 305

		C2 c2 = c3;
		c2.c2a(); // 307
		c2.c2z(); // 308
		c2.c2y(); // 205

		I1 i1 = c3;
		i1.i1a(); // 203
		i1.i1b(); // 304

		C1 c1 = c3;
		c1.c1a(); // 201
		c1.c1b(); // 301
		c1.c1z(); // 302
		c1.c1y(); // 303
		c1.c1x(); // 206
		c1.c1w(); // 104
	}
}

