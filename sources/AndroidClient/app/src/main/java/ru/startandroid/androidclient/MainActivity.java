package ru.startandroid.androidclient;

import android.os.AsyncTask;
import android.os.Looper;
import android.support.design.widget.TextInputLayout;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Base64;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.Socket;
import java.security.Key;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.crypto.Cipher;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }

    String serIpAddress;       // адрес сервера
    String username;
    String password;
    int port = 27015;           // порт
    String msg;                 // Сообщение
    final byte codeMsg = 1;     // Оправить сообщение
    final byte codeRotate = 2;  // Повернуть экран
    final byte codePoff = 3;    // Выключить компьютер
    byte codeCommand;
    EditText ipEdit, unEdit;
    TextInputLayout pwEdit;
    Button authorization;
    public void MyonClick(View view)
    {
        ipEdit=(EditText)findViewById(R.id.editText5);
        serIpAddress = ipEdit.getText().toString();

        if (serIpAddress.isEmpty())
        {
            Toast msgToast = Toast.makeText(this, "Введите ip адрес", Toast.LENGTH_SHORT);
            msgToast.show();
            return;
        }
        Pattern p=Pattern.compile("([0-9]{1,3}\\.){3}[0-9]{1,3}");
        Matcher m=p.matcher(serIpAddress);
        if(!m.matches())
        {
            Toast msgToast = Toast.makeText(this, "Некорректный ip адрес", Toast.LENGTH_SHORT);
            msgToast.show();
            return;
        }
        unEdit=(EditText)findViewById(R.id.editText6);
        username=unEdit.getText().toString();
        if(username.isEmpty())
        {
            Toast msgToast = Toast.makeText(getApplicationContext(), "Введите username", Toast.LENGTH_SHORT);
            msgToast.show();
            return;
        }

        pwEdit=(TextInputLayout)findViewById(R.id.textInputLayout);
        password=pwEdit.getEditText().getText().toString();
        if(password.isEmpty())
        {
            Toast msgToast = Toast.makeText(this, "Введите password", Toast.LENGTH_SHORT);
            msgToast.show();
            return;
        }

        msg=username+"&"+password+"\0";

        SenderThread sender = new SenderThread(); // объект представляющий поток отправки сообщений
        switch (view.getId()) // id кнопок
        {
            case R.id.button: // отправить сообщение
                if (!msg.isEmpty()) {
                    codeCommand = codeMsg;
                    sender.execute();
                }
                break;

        }
       // CripingData(msg);
    }

    public void CripingData(String text)
    {
        // Original text
        String theTestText = text;

        Toast msgToast;
        // Generate key pair for 1024-bit RSA encryption and decryption
        Key publicKey = null;
        Key privateKey = null;
        try {
            KeyPairGenerator kpg = KeyPairGenerator.getInstance("RSA");
            kpg.initialize(1024);
            KeyPair kp = kpg.genKeyPair();
            publicKey = kp.getPublic();
            privateKey = kp.getPrivate();
        } catch (Exception e) {
             msgToast = Toast.makeText(this, e.getMessage(), Toast.LENGTH_SHORT);
            msgToast.show();
        }

        // Encode the original data with RSA private key
        byte[] encodedBytes = null;
        try {
            Cipher c = Cipher.getInstance("RSA");
            c.init(Cipher.ENCRYPT_MODE, publicKey);
            encodedBytes = c.doFinal(theTestText.getBytes());
        } catch (Exception e) {
             msgToast = Toast.makeText(this, e.getMessage(), Toast.LENGTH_SHORT);
            msgToast.show();
        }
         msgToast = Toast.makeText(this, "[ENCODED]:\n" +
                Base64.encodeToString(encodedBytes, Base64.DEFAULT), Toast.LENGTH_SHORT);
        msgToast.show();

        // Decode the encoded data with RSA public key
        byte[] decodedBytes = null;
        try {
            Cipher c = Cipher.getInstance("RSA");
            c.init(Cipher.DECRYPT_MODE, privateKey);
            decodedBytes = c.doFinal(encodedBytes);
        } catch (Exception e) {
             msgToast = Toast.makeText(this, e.getMessage(), Toast.LENGTH_SHORT);
            msgToast.show();
        }
        msgToast = Toast.makeText(this, "[DECODED]:\n" + new String(decodedBytes), Toast.LENGTH_SHORT);
        msgToast.show();

    }

    class SenderThread extends AsyncTask<Void, String, Void>
    {
        @Override
        protected Void doInBackground(Void... params) {
            Toast msgToast=null;
            try {
                // ip адрес сервера
                InetAddress ipAddress = InetAddress.getByName(serIpAddress);
                // Создаем сокет

                    Socket socket = new Socket(ipAddress, port);
                // Получаем потоки ввод/вывода
                OutputStream outputStream = socket.getOutputStream();
                InputStream inputStream= socket.getInputStream();
                DataOutputStream out = new DataOutputStream(outputStream);
                DataInputStream in=new DataInputStream(inputStream);
                switch (codeCommand) { // В зависимости от кода команды посылаем сообщения
                    case codeMsg:	// Сообщение
                        // Устанавливаем кодировку символов UTF-8
                        byte[] outMsg = msg.getBytes("UTF8");
                        out.write(outMsg);
                        break;
                }

                String answer;
                byte[] buffer = new byte[1024*4];
                int count;
                do {
                    count = in.read(buffer, 0, buffer.length);
                    if (count > 0) {
                        System.out.println(new String(buffer, 0, count));
                        answer = new String(buffer, 0, count);
                        publishProgress(answer);

                    }
                }while(count>0);
                socket.close();
            }
            catch (Exception ex)
            {
                publishProgress(ex.getMessage());
                return null;
            }
            return null;
        }

        @Override
        protected void onProgressUpdate(String ... m) {
            super.onProgressUpdate(m);
            Toast msgToast = Toast.makeText( getApplicationContext(), m[0], Toast.LENGTH_SHORT);
            msgToast.show();
        }
    }

}


