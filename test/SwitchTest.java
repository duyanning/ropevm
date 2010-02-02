
class SwitchTest{
	public static void switchTest(int value){
		/* Currently, this first TableSwitch starts at an address
		   which is divisible by 4.  If you change the code, try
		   to leave at least one of the switch statements on such
		   an address, since that will test the interpreter's
		   handling of the zero-padding in the xxxSWITCH opcodes.
		*/
		System.out.println(value);
		/* Tableswitch */
		switch(value){
		case 2:
			System.out.println("Two");
			break;
		case 3:
			System.out.println("Three");
			break;
		default:
			System.out.println("Not two or three");
			break;
		}
		/* Lookupswitch can be implemented using a binary search,
		   and often off-by-one errors in binary searches only turn
		   up with either even or odd numbers of items, but not both.
		   Thus, we test both cases.
		*/
		/* Lookupswitch, even */
		switch(value){
		case 2:
			System.out.println("Two again");
			break;
		case 102:
			System.out.println("One hundred and two");
			break;
		default:
			System.out.println("Not 2 or 102");
			break;
		}
		/* Lookupswitch, odd */
		switch(value){
		case 3:
			System.out.println("Three again");
			break;
		case 103:
			System.out.println("One hundred and three");
			break;
		case -1000000003: /* Make sure multi-byte negative values are handled properly */
			System.out.println("Minus one billion and three");
			break;
		default:
			System.out.println("Not 3, 103, or -1000000003");
			break;
		}
	}

	public static void main(String[] args){
		/* Make sure the default case is handled cleanly for values
		   just outside the tableswitch's range:
		*/
		switchTest(1);
		switchTest(2);
		switchTest(3);
		switchTest(4);
		/* Test the lookupswitches */
		switchTest(100);
		switchTest(102);
		switchTest(103);
		switchTest(1000);
		switchTest(-1000000003);
	}
}

