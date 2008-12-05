using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace tcpTftpClientDotNet
{
    class Program
    {
        static void Main(string[] args)
        {
            //byte[] rxBuf = new byte[516];
            //int rxCount;
            Console.WriteLine("Starting Client");
            Client c = new Client("192.168.20.66", 5069);
            //c.sendTftpPacket(Client.OP_CODE.RRQ, "./server.log", null,-1);
            //rxCount = c.recvTftpPacket(ref rxBuf);
            
            
            Console.WriteLine("Sending File");
            c.PutBinaryFile("..\\..\\Client.cs");
            Console.WriteLine("Receiving File");
            Console.WriteLine(c.GetFile("Client.cs"));
            c.Close();
            //Console.WriteLine("Press Any Key To Exit ...");
            //Console.ReadKey(true);
        }
    }
}
