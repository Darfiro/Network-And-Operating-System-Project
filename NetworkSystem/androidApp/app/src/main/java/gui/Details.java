package gui;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;

import androidx.annotation.RequiresApi;

import com.example.linuxmonitor.R;

import java.util.ArrayList;
import java.util.List;

import logic.Container;
import logic.MemInfo;
import logic.Process;

public class Details extends Activity
{
    private TextView moment, pid1, comm1, percent1, pid2, comm2, percent2, pid3, comm3, percent3, pid4, comm4, percent4,pid5, comm5, percent5;
    private EditText total, left;
    private MemInfo memory;
    private List<String> listItems = new ArrayList<>();
    private List<String> listItems2 = new ArrayList<>();
    private ArrayAdapter<String> adapter1, adapter2;

    /** Called when the activity is first created. */
    @SuppressLint("SetTextI18n")
    @RequiresApi(api = Build.VERSION_CODES.N)
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.details);
        Intent intent = getIntent();
        Bundle bd = intent.getExtras();
        String date = bd.getString("date");
        memory = Container.getMoment(date);

        moment = findViewById(R.id.moment);
        moment.setText(date);
        moment.setFocusable(false);
        moment.setFocusableInTouchMode(false);
        moment.setClickable(false);

        total = findViewById(R.id.total);
        total.setText(memory.getTotal() + " kB");
        total.setFocusable(false);
        total.setFocusableInTouchMode(false);
        total.setClickable(false);

        left = findViewById(R.id.left);
        left.setText(memory.getLeft() + " kB");
        left.setFocusable(false);
        left.setFocusableInTouchMode(false);
        left.setClickable(false);

        pid1 = findViewById(R.id.pid1);
        pid2 = findViewById(R.id.pid2);
        pid3 = findViewById(R.id.pid3);
        pid4 = findViewById(R.id.pid4);
        pid5 = findViewById(R.id.pid5);

        comm1 = findViewById(R.id.comm1);
        comm2 = findViewById(R.id.comm2);
        comm3 = findViewById(R.id.comm3);
        comm4 = findViewById(R.id.comm4);
        comm5 = findViewById(R.id.comm5);

        percent1 = findViewById(R.id.percent1);
        percent2 = findViewById(R.id.percent2);
        percent3 = findViewById(R.id.percent3);
        percent4 = findViewById(R.id.percent4);
        percent5 = findViewById(R.id.percent5);

        List<Process> prs = memory.getProcesses();

        pid1.setText(String.valueOf(prs.get(0).getPid()));
        comm1.setText(prs.get(0).getComm());
        percent1.setText(String.valueOf(prs.get(0).getPercentMem()));

        pid2.setText(String.valueOf(prs.get(1).getPid()));
        comm2.setText(prs.get(1).getComm());
        percent2.setText(String.valueOf(prs.get(1).getPercentMem()));

        pid3.setText(String.valueOf(prs.get(2).getPid()));
        comm3.setText(prs.get(2).getComm());
        percent3.setText(String.valueOf(prs.get(2).getPercentMem()));

        pid4.setText(String.valueOf(prs.get(3).getPid()));
        comm4.setText(prs.get(3).getComm());
        percent4.setText(String.valueOf(prs.get(3).getPercentMem()));

        pid5.setText(String.valueOf(prs.get(4).getPid()));
        comm5.setText(prs.get(4).getComm());
        percent5.setText(String.valueOf(prs.get(4).getPercentMem()));

    }

}