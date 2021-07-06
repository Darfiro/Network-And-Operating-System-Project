package logic;

public class ErrorMessages
{
    private static String message = "";
    public static void setMessage(String error) { message = error; }
    public static String getMessage() {return message; }
}
