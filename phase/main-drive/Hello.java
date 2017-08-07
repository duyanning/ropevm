class Obj {
	public Obj(String name) {
		m_name = name;
	}
	public void touch(String who) {
        System.out.println(who + " -> " + m_name);
	}
	public void touch(Obj who) {
        System.out.println(who.getName() + " -> " + m_name);
	}
	public void touch(String who, Obj next) { // who touch me(this)? I(this) touch next.
        System.out.println(who + " -> " + m_name);
        next.touch(m_name);
	}
	public void touch(Obj who, Obj next) {
        System.out.println(who.getName() + " -> " + m_name);
        next.touch(m_name);
	}
	// public void send(Obj o, String msg) {
	// 	System.out.println(m_name + " sends to " + o.getName() + ": " + msg);
	// 	o.receive(msg);
	// }
	// public void receive(String msg) {
	// 	System.out.println(this.getName() + " received: " + msg);
	// }

	public String getName() {
		return m_name;
	}
	private String m_name;
}

class Hello
{
    public static void main(String args[])
    {
		Obj a = new Obj("a");
		Obj b = new Obj("b");
		Obj c = new Obj("c");
		Obj d = new Obj("d");
        RopeVMBackdoor.register_obj(Hello.class, "main");
        RopeVMBackdoor.register_obj(a, "a");
        //RopeVMBackdoor.register_obj();
        RopeVMBackdoor.register_obj(b, "b");
        RopeVMBackdoor.register_obj(c, "c");
        RopeVMBackdoor.register_obj(d, "d");

        a.touch("main", b); 
        c.touch("main", d);
        b.touch("main", a); 
        d.touch("main", c);

        a.touch("main", c); 
        b.touch("main", d);
        c.touch("main", a); 
        d.touch("main", b);
        
    }
}
