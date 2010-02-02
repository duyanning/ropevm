
/* Patrick Doyle  Sep 2001 */

class Benchmark{

	protected int timerResolution=1/*
		The number of milliseconds between System.currentTimeMillis() updates.
	*/;

	protected void action(int iterCount){
		/* Override this with whatever action you would like to measure.
		   Note that iterCount is an int rather than a long, to allow
		   it to be used as a loop counter without an undesirable
		   performance hit.
		*/
		while(--iterCount > 0);
	}

	public float run(int millis){
		/* Calls action a few times with the approriate iteration count
		   to make it run for approximately the given total number of
		   milliseconds.
		   Returns the number of iterations executed per millisecond.
		   Note that the overhead introduced by this code doesn't matter,
		   because it only executes a few times (ie. less than ten or so).
		   Thus its effect on the execution time is negligible because
		   most of the time is spent inside the action method.
		*/
		long iterCount = 0, itersElapsed = 0;
		long startTime = System.currentTimeMillis();
		long timeElapsed, timeLeft;
		while(true){
			/* Estimate how many iterations to run */
			timeElapsed = System.currentTimeMillis() - startTime;
			timeLeft = millis - timeElapsed;
			if(timeLeft <= 0){
				break;
			}else{
				/* Basically,
					iterCount = itersElapsed * timeLeft/timeElapsed
				   but with extra care for some corner cases.  Tries to
				   make sure that iterCount will err on the side of being
				   too small, so we don't overrun our time limit.
				*/
				iterCount =
					  itersElapsed
					* (timeLeft * 9)
					/ ((timeElapsed + timerResolution) * 10)
				;
			}
			/* Clip the iteration count to be a positive int */
			if(iterCount <= 0){
				iterCount = 1;
			}else if(iterCount > Integer.MAX_VALUE){
				iterCount = Integer.MAX_VALUE;
			}
			/* Call the action */
			System.out.println("action(" + iterCount + ")");
			action((int)iterCount);
			itersElapsed += iterCount;
		}
		return (float)itersElapsed / millis;
	}

	public void run(String[] args){
		/* A standard way to run a benchmark.  Call this from the
		   program's main method, and pass in the program arguments.
		   If an argument is present, it is taken to be the number
		   of milliseconds that the benchmark should take.  Otherwise,
		   it runs for some suitably small amount of time.
		*/
		try{
			int millis = 1000;
			if(args.length > 0)
				millis = Integer.parseInt(args[0]);
			System.err.println((long)(run(millis) * 1000) + " per second");
		}catch(NumberFormatException e){
			System.err.println(
				"Argument should be the duration of the test, in milliseconds"
			);
		}
	}

	public static void main(String[] args){
		/* An example of how the main method might look.  (Subclasses,
		   of course, should instantiate themselves.)
		*/
		new Benchmark().run(args);
	}

}

