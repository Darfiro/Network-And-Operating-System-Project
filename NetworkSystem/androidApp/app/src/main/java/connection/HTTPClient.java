package connection;

import android.os.AsyncTask;
import android.os.Build;

import androidx.annotation.RequiresApi;

import org.json.JSONException;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.lang.reflect.Method;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;

import gui.AppPage;
import gui.MainActivity;
import logic.ErrorMessages;
import logic.Parser;

public class HTTPClient extends AsyncTask<String, Void, String>
{
    private String address;
    private int port;
    private String httpGet = "GET /LinuxMonitor.json HTTP/1.1\nHost: ";
    private MainActivity parentMain;
    private AppPage parentApp;

    public String getAddress() { return this.address; }
    public int getPort() { return this.port; }

    public HTTPClient(){}

    public String getHostName(String defValue) {
        try {
            Method getString = Build.class.getDeclaredMethod("getString", String.class);
            getString.setAccessible(true);
            return getString.invoke(null, "net.hostname").toString();
        } catch (Exception ex) {
            return defValue;
        }
    }

    public HTTPClient(String address, int port, MainActivity parentMain, AppPage parentApp)
    {
        this.address = address;
        this.port = port;
        this.parentMain = parentMain;
        this.parentApp = parentApp;
        String hostName = getHostName("android-phone");
        this.httpGet += hostName;
    }

    private void sendRequest(Socket sock) throws IOException
    {
        DataOutputStream dataOutputStream = new DataOutputStream(sock.getOutputStream());
        byte[] buf = httpGet.getBytes("UTF-8");
        dataOutputStream.write(buf, 0, buf.length);
    }

    private String receiveData(Socket sock) throws IOException
    {
        DataInputStream dataInputStream = new DataInputStream(sock.getInputStream());
        String result = "no result";
        StringBuilder temp = new StringBuilder();
        String line = dataInputStream.readLine();
        while (line != null)
        {
            temp.append(line);
            line = dataInputStream.readLine();
        }
        if (!temp.toString().equals(""))
            result = temp.toString();
        dataInputStream.close();
        return result;
    }

    @Override
    protected String doInBackground(String... params)
    {
        // TODO Auto-generated method stub
        Socket socket = null;
        String result = "";
        try
        {
            socket = new Socket(address, port);
            sendRequest(socket);
            result = receiveData(socket);
            if (result.equals("no result"))
                ErrorMessages.setMessage("Ошибка: не удалось подключиться к серверу");
        }
        catch (UnknownHostException e)
        {
            System.out.println(e.getMessage());
            ErrorMessages.setMessage("Ошибка сервера: " + e.getLocalizedMessage());
        }
        catch (IOException e)
        {
            ErrorMessages.setMessage("Ошибка при передаче данных: " + e.getLocalizedMessage());
        }
        finally
        {
            if (socket != null)
            {
                try
                {
                    System.out.println("SOCKET CLOSED");
                    socket.close();
                }
                catch (IOException e)
                {
                    ErrorMessages.setMessage("Ошибка при закрытии соединения: " + e.getLocalizedMessage());
                }
            }
        }
        return result;
    }

    @RequiresApi(api = Build.VERSION_CODES.O)
    @Override
    protected void onPostExecute(String result)
    {
        try {
            Parser.parse(result);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        if (parentMain != null)
            parentMain.backFromClient();
        else if (parentApp != null)
            parentApp.backFromClient();
    }

    @Override
    protected void onPreExecute() {}

    @Override
    protected void onProgressUpdate(Void... values) {}
}
