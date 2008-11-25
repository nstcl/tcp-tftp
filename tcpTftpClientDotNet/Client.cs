using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net;
using System.Net.Sockets;
namespace tcpTftpClientDotNet
{
    class Client
    {
        Socket s;
        NetworkStream ns;
        public enum OP_CODE:short{RRQ=1,WRQ=2,CD=6,LIST=7,ACK=4,DATA=3,ERROR=5}
        public Client(string address, int port)
        {
            s = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            s.Bind(new IPEndPoint(IPAddress.Any, 5555));
            s.Connect(address, port);
            ns = new NetworkStream(s);
        }
        public void sendTftpPacket(OP_CODE op,string fname, byte[] data, short blockNumber)
        {
            byte[] buf = null;
            byte[] mode = null;
            byte[] txBuffer = null;
            byte[] b_op = null;

            if (fname.Length>0)
            {
                buf = new byte[fname.ToCharArray().Length + 1];
                for (int i = 0; i < buf.Length - 1; i++)
                {
                    buf[i] = (byte)ASCIIEncoding.Convert(Encoding.Unicode, Encoding.ASCII, BitConverter.GetBytes(fname[i])).GetValue(0);
                }
                buf[buf.Length - 1] = (byte)'\0';
                mode = new byte["octet\0".Length];
                for (int i = 0; i < mode.Length - 1; i++)
                {
                    mode[i] = (byte)ASCIIEncoding.Convert(Encoding.Unicode, Encoding.ASCII, BitConverter.GetBytes("octet\0"[i])).GetValue(0);
                }
            }
            switch (op)
            {
                case OP_CODE.RRQ:
                    txBuffer = new byte[buf.Length+2+mode.Length];
                    b_op = BitConverter.GetBytes(IPAddress.HostToNetworkOrder((short)OP_CODE.RRQ));
                    Buffer.BlockCopy(b_op, 0, txBuffer, 0, b_op.Length);
                    Buffer.BlockCopy(buf, 0, txBuffer, b_op.Length, buf.Length);
                    Buffer.BlockCopy(mode, 0, txBuffer, b_op.Length + buf.Length, mode.Length);
                    s.Send(txBuffer);
                    break;
                case OP_CODE.WRQ:
                    break;
                case OP_CODE.CD:
                    break;
                case OP_CODE.LIST:
                    break;
                case OP_CODE.ACK:
                    if (blockNumber>=0)
                    {
                        txBuffer = new byte[4];
                        b_op = BitConverter.GetBytes(IPAddress.HostToNetworkOrder((short)OP_CODE.ACK));
                        Buffer.BlockCopy(b_op, 0, txBuffer, 0, b_op.Length);
                        byte[] blockAck = BitConverter.GetBytes(IPAddress.HostToNetworkOrder(blockNumber));
                        Buffer.BlockCopy(blockAck, 0, txBuffer, b_op.Length, sizeof(short));
                    }
                    break;
                case OP_CODE.DATA:
                    break;
                case OP_CODE.ERROR:
                    break;
                default:
                    break;
            }
        }
        public int recvTftpPacket(ref byte[] buffer)
        {
            int count = s.Receive(buffer);
            return count;
        }

        public string GetFile(string filename)
        {
            byte[] rxBuf = new byte[516];
            int rxCount;
            string s = "";
            short blockNumber;
            ASCIIEncoding ae = new ASCIIEncoding();
            sendTftpPacket(Client.OP_CODE.RRQ, filename, null,-1);//send RRQ
            
            do
            {
                rxCount = recvTftpPacket(ref rxBuf);
                OP_CODE op = (OP_CODE)IPAddress.NetworkToHostOrder(BitConverter.ToInt16(rxBuf, 0));
                blockNumber = IPAddress.NetworkToHostOrder(BitConverter.ToInt16(rxBuf, 2));
                s += new string(ae.GetChars(rxBuf, sizeof(short) * 2, rxCount - 4 >= 4 ? rxCount - 4 : 0)); ;
                sendTftpPacket(Client.OP_CODE.ACK, string.Empty, null, blockNumber);
            } while (rxCount >= 516);
            //sendTftpPacket(Client.OP_CODE.ACK, string.Empty, null, blockNumber);//final ACK
            return s;
        }
        ~Client()
        {
            s.Close(1);
        }
    }
}
