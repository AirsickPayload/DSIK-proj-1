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


namespace FTServerCode
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
            FTServerCode.receivedPath = "";
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (FTServerCode.receivedPath.Length > 0)
                backgroundWorker1.RunWorkerAsync();
            else
                MessageBox.Show("Podaj lokalizacje w kotrej zapisac plik: ");
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            label5.Text = FTServerCode.receivedPath;
            label3.Text = FTServerCode.curMsg;
        }

        FTServerCode obj = new FTServerCode();
        private void backgroundWorker1_DoWork(object sender, DoWorkEventArgs e)
        {

            obj.StartServer();
        }

        private void button2_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog fd = new FolderBrowserDialog();
            if (fd.ShowDialog() == DialogResult.OK)
            {
                FTServerCode.receivedPath = fd.SelectedPath;
            }
        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }

        private void button3_Click(object sender, EventArgs e)
        {

        }

    }
    //FILE TRANSFER USING C#.NET SOCKET - SERVER
    public class global
    {
        public static int temp = 0;
        public static int l_plikow = 0;
    }
    class FTServerCode
    {
        IPEndPoint ipEnd;
        Socket sock;
        public FTServerCode()
        {
            ipEnd = new IPEndPoint(IPAddress.Any, 5656);
            sock = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.IP);
            sock.Bind(ipEnd);
        }
        public static string receivedPath;
        public static string curMsg = "Zatrzymane";
        public void StartServer()
        {
            try
            {
                curMsg = "Uruchamiam...";
                sock.Listen(100);
                curMsg = "Uruchomiony, czekam na odbior pliku.";
                Socket clientSock = sock.Accept();
                if (global.temp == 0)
                {
                    byte[] lp = new byte[sizeof(int)];
                    clientSock.Receive(lp);

                    if (BitConverter.IsLittleEndian) //konwersja byte to int
                        Array.Reverse(lp);
                    global.l_plikow = BitConverter.ToInt32(lp, 0);
                    //for (int i = 1; i <= l_plikow; i++)
                    //{
                }
                curMsg = "liczba plikow: " + global.l_plikow;
                System.Threading.Thread.Sleep(2000);

                int rozmiar = 0;
                NetworkStream stream = new NetworkStream(clientSock); //<--------- czytaj rozmiar pliku ktory nadejdzie
                BinaryReader reader = new BinaryReader(stream);
                {
                    rozmiar = reader.ReadInt32();
                }
                curMsg = "odebralem: "+rozmiar+" bitow";
                System.Threading.Thread.Sleep(2000);

                byte[] archiwum = new byte[19]; //<------------ zapisuje ustawienia archiwum (musi byæ 19 znaków)
                curMsg = "Odbieram dane archiwum ...";
                clientSock.Receive(archiwum);
                string odKlienta = ASCIIEncoding.ASCII.GetString(archiwum);
                curMsg = "Konf. arch. klienta: " + odKlienta;
                System.Threading.Thread.Sleep(2000);

                byte[] clientData = new byte[1024 * 5000];

                int receivedBytesLen = clientSock.Receive(clientData);
                curMsg = "Odbieram dane...";
                //System.Threading.Thread.Sleep(2000);
                int fileNameLen = BitConverter.ToInt32(clientData, 0);
                string fileName = Encoding.ASCII.GetString(clientData, 4, fileNameLen);

                BinaryWriter bWrite = new BinaryWriter(File.Open(receivedPath + "/" + fileName, FileMode.Append)); ;
                bWrite.Write(clientData, 4 + fileNameLen, receivedBytesLen - 4 - fileNameLen);

                curMsg = "Zapisuje plik...";
                //System.Threading.Thread.Sleep(2000);
                ASCIIEncoding asen = new ASCIIEncoding();
                clientSock.Send(asen.GetBytes("ACK"));
                curMsg = "Plik obs³u¿ony";
                bWrite.Close();
                Array.Clear(clientData, 0, clientData.Length); //czyszcze tablice
                receivedBytesLen = 0;
                //}
                global.temp = 1;
                global.l_plikow = global.l_plikow - 1;
                if (global.l_plikow != 0) StartServer();
                clientSock.Close();
                curMsg = "Serwer zatrzymany";
            }
            catch (Exception ex)
            {
                curMsg = "Blad odbioru pliku." + ex.Message;
            }
        }
    }
}