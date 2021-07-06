package logic;

import android.os.Build;

import androidx.annotation.RequiresApi;
import java.util.HashMap;
import java.util.Map;

public class Container
{
    private static Map<String, MemInfo> timeline;

    public static void clear()
    {
        timeline = new HashMap<>();
    }

    @RequiresApi(api = Build.VERSION_CODES.O)
    public static void addMoment(String date, MemInfo mem)
    {
        if (timeline == null)
            timeline = new HashMap<>();

        timeline.put(date, mem);
    }

    public static Map<String, MemInfo> getTimeline() { return timeline; }

    public static MemInfo getMoment(String dateTime) { return timeline.get(dateTime); }

}
