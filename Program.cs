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
            byte[] rxBuf = new byte[516];
            int rxCount;
            Console.WriteLine("Starting Client");
            Client c = new Client("135.64.102.52", 5069);
            c.sendTftpPacket(Client.OP_CODE.RRQ, "./server.log", null);
            rxCount = c.recvTftpPacket(ref rxBuf);
            Console.WriteLine("Press Any Key To Exit ...");
            Console.ReadKey(true);
        }
    }
}
