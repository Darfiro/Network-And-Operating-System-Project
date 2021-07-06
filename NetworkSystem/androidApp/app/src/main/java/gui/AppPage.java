package gui;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;

import com.example.linuxmonitor.R;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Map;

import connection.HTTPClient;
import logic.Container;
import logic.ErrorMessages;
import logic.MemInfo;

public class AppPage extends Activity
{
    private Button updateBtn;
    private HTTPClient client;
    private ListView timeline;
    private ArrayList<String> listItems = new ArrayList<>();
    private Map<String, MemInfo> info;
    private ArrayAdapter<String> adapter;
    private AppPage thisApp;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.app_page);
        Intent intent = getIntent();
        Bundle bd = intent.getExtras();
        client = new HTTPClient(bd.getString("address"), bd.getInt("port"), null, this);
        updateBtn = findViewById(R.id.update);
        updateBtn.setOnClickListener(buttonSendOnClickListener);

        timeline = findViewById(R.id.timeline);

        info = Container.getTimeline();
        listItems.addAll(info.keySet());
        Collections.sort(listItems);

        adapter = new ArrayAdapter<>(this,
                android.R.layout.simple_list_item_1, android.R.id.text1, listItems);
        timeline.setAdapter(adapter);
        timeline.setOnItemClickListener((adapterView, view, position, l) -> {
            String value = adapter.getItem(position);
            Intent intentApp = new Intent(AppPage.this, Details.class);
            intentApp.putExtra("date", value);
            startActivity(intentApp);
        });
        thisApp = this;
    }

    public void backFromClient()
    {
        String error = ErrorMessages.getMessage();
        if (error.equals(""))
        {
            client = new HTTPClient(client.getAddress(), client.getPort(), null, thisApp);
            ErrorMessages.setMessage("");
            info = Container.getTimeline();
            listItems.clear();
            listItems.addAll(info.keySet());
            Collections.sort(listItems);
            adapter.notifyDataSetChanged();
        }
    }

    Button.OnClickListener buttonSendOnClickListener = new Button.OnClickListener()
    {
        @Override
        public void onClick(View arg0)
        {
            client.execute("");
        }
    };
}
