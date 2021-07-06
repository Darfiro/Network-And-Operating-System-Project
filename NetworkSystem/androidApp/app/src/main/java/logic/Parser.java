package logic;

import android.os.Build;

import androidx.annotation.RequiresApi;

import org.json.JSONException;
import org.json.JSONObject;
import java.util.regex.Pattern;

public class Parser
{
    @RequiresApi(api = Build.VERSION_CODES.O)
    public static void parse(String message) throws JSONException
    {
        message = message.substring(53);
        String[] messages = message.split(Pattern.quote("}}"));
        Container.clear();

        for (String st: messages)
        {
            if (st.length() > 27)
            {
                JSONObject reader = new JSONObject(st + "}}");
                String dateTime = reader.getString("time");
                JSONObject mem = reader.getJSONObject("memoryUsage");
                MemInfo info = new MemInfo(mem.getLong("MemTotal"), mem.getLong("MemFree"));
                for (int i = 0; i < 5; i++)
                {
                    JSONObject process = mem.getJSONObject("process" + i);
                    long pid = process.getLong("pid");
                    String comm = process.getString("comm");
                    double percent = process.getDouble("mem");
                    Process pr = new Process(pid, comm, percent);
                    info.addProcess(pr);
                }
                Container.addMoment(dateTime, info);
            }
        }
    }
}
