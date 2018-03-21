package ru.startandroid.androidclient;

/**
 * Created by Home on 17.03.2018.
 */

public class PMessage1 extends PMessage{
   private int len;
   private int len2;
    private String data;
    public PMessage1(){}
   public void SetLenMessage(int l)
   {
       this.len=l;
   }
    public void SetLenData(int l)
    {
        this.len2=l;
    }
   public void SetData(String d)
   {
       this.data=d;
   }

   public int GetLenMessage()
   {
       return this.len;
   }
    public int GetLenData()
    {
        return this.len2;
    }
   public String GetData()
   {
       return this.data;
   }
}
