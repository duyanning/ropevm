public class FactRec {
      public static void main(String args[]) {
          if (args.length > 0)
              System.out.println(Factorial.factorialRecursive(Integer.parseInt(args[0])));
      }
}