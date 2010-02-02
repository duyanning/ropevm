public class TowerOfHanoi {
    public static void main(String args[]) {
        if (args.length > 0)
            new TowerOfHanoi(Integer.parseInt(args[0]), 'a', 'c', 'b');
    }

    public TowerOfHanoi(int n, char origin, char destination, char auxiliary) {
        if (n > 1)
            // move all disks except for the bottom one from origin to auxiliary,
            // using the destination
            new TowerOfHanoi(n - 1, origin, auxiliary, destination);
        System.out.println("Move a disc from peg " + origin + " to peg " + destination);
        if (n > 1)
            // move all disks except for the bottom one from auxiliary to destination,
            // using the origin
            new TowerOfHanoi(n - 1, auxiliary, destination, origin);
    }
}

        
        