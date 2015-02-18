using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Net;
using System.Net.Sockets;
using System.IO;


namespace FTClientCode
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        public void button1_Click(object sender, EventArgs e)
        {
            FileDialog fDg = new OpenFileDialog();
            if (fDg.ShowDialog() == DialogResult.OK)
            {
                FTClientCode.SendFile(fDg.FileName);
            }
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            label3.Text = FTClientCode.curMsg;
            label1.Text = FTClientCode.ack;
        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }

        private void label1_Click(object sender, EventArgs e)
        {
            
        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {
            if (!Int32.TryParse(AmountOfFiles.Text, out iloscPlikow.liczbaPlikow))
            {
                Environment.Exit(1);
            }
        }

        private void PliczbePlikow_Click(object sender, EventArgs e)
        {
            
        }

        private void textBox1_TextChanged_1(object sender, EventArgs e)
        {

        }

        private void button2_Click(object sender, EventArgs e)
        {
            iloscPlikow.k_archiwum = d_archiwum.Text;
        }

    }
    public class iloscPlikow
    {
        public static int liczbaPlikow = 0;
        public static int counter = 0;
        public static string k_archiwum = "dane haslo archiwum";
    }
   
    class FTClientCode
    {
        public static string curMsg = "Uruchomiony(idle)";
        public static string ack = "Nie otrzyma³em potwierdzenia";
        public static void SendFile(string fileName)
        {     
                try
                {
                    IPAddress[] ipAddress = Dns.GetHostAddresses("10.0.2.15");
                    IPEndPoint ipEnd = new IPEndPoint(ipAddress[0], 9981);
                    Socket clientSock = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.IP);

                    string filePath = "";
                 //for (int i = 1; i <= iloscPlikow.liczbaPlikow; i++)
                 //{
                    fileName = fileName.Replace("\\", "/");
                    while (fileName.IndexOf("/") > -1)
                    {
                        filePath += fileName.Substring(0, fileName.IndexOf("/") + 1);
                        fileName = fileName.Substring(fileName.IndexOf("/") + 1);
                    }

                    byte[] fileNameByte = Encoding.ASCII.GetBytes(fileName);

                    if (fileNameByte.Length > 850 * 1024)
                    {
                        curMsg = "Rozmiar pliku przekracza 850kb, dokonaj retransmisji przy mniejszej wielkosci pliku.";
                        return;
                    }

                    curMsg = "Buferuje ...";
                    //System.Threading.Thread.Sleep(2000);
                    byte[] fileData = File.ReadAllBytes(filePath + fileName);
                    byte[] clientData = new byte[4 + fileNameByte.Length + fileData.Length];
                    byte[] fileNameLen = BitConverter.GetBytes(fileNameByte.Length);

                    fileNameLen.CopyTo(clientData, 0);
                    fileNameByte.CopyTo(clientData, 4);
                    fileData.CopyTo(clientData, 4 + fileNameByte.Length);

                        curMsg = "Laczenie z serwerem ...";
                        //System.Threading.Thread.Sleep(2000);
                        clientSock.Connect(ipEnd); //tylko raz na jedno polaczenie
                        byte[] lp = BitConverter.GetBytes(iloscPlikow.liczbaPlikow); //konwersja liczby plikow z int do byte[], aby wyslac gniazdem
                        if (BitConverter.IsLittleEndian)
                            Array.Reverse(lp);
                        //byte[] result = lp; 

                        if (iloscPlikow.counter == 0) { clientSock.Send(lp); iloscPlikow.counter = iloscPlikow.counter + 1; } //tylko raz na polaczenie
                        curMsg = "Wyslano do serwera liczbe plikow, wysylam rozmiar pliku";

                        NetworkStream stream = new NetworkStream(clientSock); //<---------------- wysy³am rozmiar pliku w bajtach
                        BinaryWriter writer = new BinaryWriter(stream);
                        {
                           int rozmiar = clientData.Length;
                            writer.Write(rozmiar);
                        }

                        ASCIIEncoding asen = new ASCIIEncoding();
                        clientSock.Send(asen.GetBytes(iloscPlikow.k_archiwum));

                        curMsg = "Wysylanie pliku...";
                        //System.Threading.Thread.Sleep(2000);
                        clientSock.Send(clientData);

                        byte[] RecData = new byte[4];
                        ack = "Odbierania statusu od serwera ...";
                        //System.Threading.Thread.Sleep(2000);
                        clientSock.Receive(RecData);
                        string odSerwera = ASCIIEncoding.ASCII.GetString(RecData);
                        ack = "SERWER MOWI: " + odSerwera;
                        filePath = "";
                        Array.Clear(fileNameByte, 0, fileNameByte.Length);
                        Array.Clear(fileData, 0, fileData.Length);
                        Array.Clear(clientData, 0, clientData.Length);
                        Array.Clear(fileNameLen, 0, fileNameLen.Length);
                        Array.Clear(RecData, 0, RecData.Length);
                    //}
                        //iloscPlikow.liczbaPlikow = iloscPlikow.liczbaPlikow - 1;
                       //if (iloscPlikow.liczbaPlikow != 0) Application.Run(new Form1()); <----- iloscPlikow transmisji w zaleznosci od ilosci plikow
                    curMsg = "Rozlaczanie...";
                    //System.Threading.Thread.Sleep(2000); 
                    clientSock.Close();
                    curMsg = "Plik przes³any pomyœlnie.";

                    //curMsg = "rozmiar " + clientData.Length;  //<<<<<<-------------- zwraca prawidlowo liczbe bajtow ktore przesylam.
                }
                catch (Exception ex)
                {
                    if (ex.Message == "No connection could be made because the target machine actively refused it")
                        curMsg = "Nie udalo sie wyslac pliku poniewaz serwer nie jest uruchomiony.";
                    else
                        curMsg = "Blad podczas wysylania pliku." + ex.Message;
                }
            }
        }
    }
