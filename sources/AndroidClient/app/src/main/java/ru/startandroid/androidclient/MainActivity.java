package ru.startandroid.androidclient;

import android.os.AsyncTask;
import android.os.Looper;
import android.security.keystore.KeyProperties;
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
import java.io.UnsupportedEncodingException;
import java.net.InetAddress;
import java.net.Socket;
import java.security.AlgorithmParameters;
import java.security.Key;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.SecureRandom;
import java.security.spec.AlgorithmParameterSpec;
import java.util.BitSet;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }
    byte[] cryptdata=null;
    String serIpAddress;       // адрес сервера
    String username;
    String password;
    String msg;                 // Сообщение
    String code;

    EditText ipEdit, unEdit;
    TextInputLayout pwEdit;
    Button authorization;
    public void MyonClick(View view)
    {
        String incorrectData="";
        ipEdit=(EditText)findViewById(R.id.editText5);
        serIpAddress = ipEdit.getText().toString();

        unEdit=(EditText)findViewById(R.id.editText6);
        username=unEdit.getText().toString();

        pwEdit=(TextInputLayout)findViewById(R.id.textInputLayout);
        password=pwEdit.getEditText().getText().toString();

        int i=0,num,count;
        boolean flag=false;
        Toast msgToast;
        if (serIpAddress.isEmpty())
        {
           incorrectData="Введите ip адрес";
        }
        Pattern p=Pattern.compile("([0-9]{1,3}\\.){3}[0-9]{1,3}");
        String [] splits=serIpAddress.split("\\.");
        count=splits.length-1;
        while(!flag&&i<splits.length)
        {
            num=Integer.parseInt(splits[i]);
            if(num>256)
                flag=true;
            ++i;
        }
        Matcher m=p.matcher(serIpAddress);

        if(!m.matches()&&!flag)
            incorrectData="Неверный формат ip адреса";

        Pattern p1=Pattern.compile("((.*[\\(\\;\\:\\[\\]\\'\\(\\)\\/\\,\\+\\*\\?\\<\\>\\)]+))");
        Matcher m1=p1.matcher(username);

        if(incorrectData.isEmpty())
        {
            if (username.isEmpty())
                incorrectData="Введите username";
            if(username.length()>20)
                incorrectData="Слишком длинное имя";
            if(m1.lookingAt())
                incorrectData="Неверный формат логина";

        }

        m1=p1.matcher(password);
       if(incorrectData.isEmpty()) {
           if (password.isEmpty())
               incorrectData="Введите password";
           if(password.length()>128)
               incorrectData="Слишком длинный пароль";
           if(m1.lookingAt())
               incorrectData="Неверный формат пароль";
       }
       msg=username+"&"+password;
        if(!incorrectData.isEmpty())
        {
            msgToast = Toast.makeText(this, incorrectData, Toast.LENGTH_LONG);
            msgToast.show();
            return;
        }


        cryptdata=CryptingData(msg);
        String beforeCryp=String.valueOf(msg.length());
        SenderThread sender = new SenderThread(); // объект представляющий поток отправки сообщений
        sender.execute(msg,serIpAddress,beforeCryp);
    }

    public byte[] CryptingData(String text)
    {
        // Original text
        String theTestText = text;

        Toast msgToast;

        SecretKeySpec sks = null;
        try {

            sks = new SecretKeySpec("passworddrowssap".getBytes(), "AES");
        } catch (Exception e) {
            msgToast = Toast.makeText(this, e.getMessage(), Toast.LENGTH_SHORT);
            msgToast.show();
        }

        // Encode the original data with AES
        byte[] encodedBytes = null;
        try {

            Cipher c = Cipher.getInstance("AES");
            c.init(Cipher.ENCRYPT_MODE, sks);
            encodedBytes = c.doFinal(theTestText.getBytes("UTF8"));
        } catch (Exception e) {
            msgToast = Toast.makeText(this, e.getMessage(), Toast.LENGTH_SHORT);
            msgToast.show();
        }
        msgToast = Toast.makeText(this, "[ENCODED]:\n" +
              Base64.encodeToString(encodedBytes, Base64.DEFAULT), Toast.LENGTH_SHORT);
        msgToast.show();
        return  encodedBytes;
    }

    class SenderThread extends AsyncTask<String, String, Void>
    {
        private int port = 27015;
        @Override
        protected Void doInBackground(String... params) {
            Toast msgToast=null;
            String outtext=null;
            String outipAddress=null;
            String tmp=null;
            int beforeCr=0;
            if(params.length==3) {
                outtext = params[0];
                outipAddress=params[1];
                beforeCr=Integer.parseInt(params[2]);
            }
            try {
                int i=1;
                String str;
                PMessage pMessage=new PMessage();
                PMessage1 pMessage1=new PMessage1();
                // ip адрес сервера
                InetAddress ipAddress = InetAddress.getByName(outipAddress);
                // Создаем сокет

                Socket socket = new Socket(ipAddress, port);
                // Получаем потоки ввод/вывода
                OutputStream outputStream = socket.getOutputStream();
                InputStream inputStream= socket.getInputStream();
                DataOutputStream out = new DataOutputStream(outputStream);
                DataInputStream in=new DataInputStream(inputStream);

                boolean finish=false;
                do {
                    byte[] outMsg = null;
                    switch (i) {
                        case 1:
                            str = "1";
                            outMsg = str.getBytes("UTF8");
                            out.write(outMsg);
                            break;

                        case 2:
                            str="4";
                            if(outtext.length()<10)
                                str += "00";
                            if(outtext.length()<100&&outtext.length()>=10)
                                str+="0";
                            str+=outtext.length();
                            if(beforeCr<10)
                                str+="00";
                            if(beforeCr<100&&beforeCr>=10)
                                str+="0";
                            str+=beforeCr+outtext;
                            outMsg = str.getBytes("UTF8");
                            out.write(outMsg);
                            break;
                    }
                    String answer;
                    byte[] buffer = new byte[1024 * 4];
                    int count;
                    count = in.read(buffer, 0, buffer.length);
                    if (count > 0) {
                            SetClassFileds(count,buffer,pMessage,pMessage1);

                        switch (pMessage.GetID()) {
                            case 2:
                                answer = pMessage1.GetData();
                                publishProgress(answer);
                                finish=true;
                                break;
                            case 3:
                                i = 2;
                                answer = "Sending credentials";
                                publishProgress(answer);
                                break;
                            case 5:
                                answer = "Incorrect login or password.\n Try again";
                                publishProgress(answer);
                                break;
                            case 6:
                                answer = "Login is successful";
                                finish=true;
                                publishProgress(answer);
                                break;
                        }

                    }
                }while(!finish);
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

            Toast  msgToast = Toast.makeText(getApplicationContext(), m[0], Toast.LENGTH_LONG);
            msgToast.show();
        }

        private void SetClassFileds(int size,byte[] text, PMessage pMes,PMessage1 pMes1)
        {
            if(size>7)
            {
                pMes1.SetLenMessage(Integer.parseInt(new String(text, 1, 3)));
                pMes1.SetLenData(Integer.parseInt(new String(text,4,3)));
                if(size==7+pMes1.GetLenMessage())
                    pMes1.SetData(new String(text, 7, pMes1.GetLenMessage()));
                else
                    pMes1.SetData("При приеме сообщения возникли какие-то ошибки");
            }

            pMes.SetID(Integer.parseInt(new String(text,0,1)));
        }
    }

}


