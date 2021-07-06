package gui;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;

import android.content.Intent;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.app.Activity;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import androidx.annotation.RequiresApi;

import com.example.linuxmonitor.R;

import org.json.JSONException;

import connection.HTTPClient;
import logic.ErrorMessages;
import logic.Parser;

public class MainActivity extends Activity
{
    private TextView errorText;
    private EditText editAddress, editPort;
    private Button buttonConnect;
    private HTTPClient client;
    private MainActivity thisActivity;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        editAddress = findViewById(R.id.address);
        editPort = findViewById(R.id.port);
        errorText = findViewById(R.id.error);

        buttonConnect = findViewById(R.id.connect);
        buttonConnect.setOnClickListener(buttonSendOnClickListener);
        thisActivity = this;
    }

    Button.OnClickListener buttonSendOnClickListener = new Button.OnClickListener()
    {
        @Override
        public void onClick(View arg0)
        {
            if (editAddress.getText().toString().equals("") || editPort.getText().toString().equals(""))
                errorText.setText("Ошибка: введите адрес и порт");
            else
            {
                errorText.setText("");
                client = new HTTPClient(editAddress.getText().toString(), Integer.parseInt(editPort.getText().toString()), thisActivity, null);

                client.execute("");
                errorText.setText("Выполняется подключение...");
            }
        }
    };

    public void backFromClient()
    {
        errorText.setText("");
        String error = ErrorMessages.getMessage();
        if (!error.equals(""))
        {
            errorText.setText(error);
            ErrorMessages.setMessage("");
        }
        else
        {
            Intent intentApp = new Intent(MainActivity.this, AppPage.class);
            intentApp.putExtra("address", client.getAddress());
            intentApp.putExtra("port", client.getPort());
            startActivity(intentApp);
        }
    }
}

