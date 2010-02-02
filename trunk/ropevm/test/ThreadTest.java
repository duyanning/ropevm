
class ThreadTest extends Thread{
	public void run(){
		System.out.println("From the thread");
	}

	public static void main(String[] args){
		ThreadTest tt = new ThreadTest();
		tt.start();
	}
}

