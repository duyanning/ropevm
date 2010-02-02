public class PowerRec {
      public static void main(String args[]) {
          if (args.length > 1)
              System.out.println(Power.powerRecursive(Integer.parseInt(args[0]),
                                                      Integer.parseInt(args[1])));
      }
}
