public class MiniClassPathHelper {
    public static void preloadClasses()
    {
        try {
            Class.forName("Math");
            Class.forName("Random");
        }
        catch (ClassNotFoundException e) {
            System.out.println("forName cannot find: " + e);
        }
    }
}