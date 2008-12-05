using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.IO;

namespace tcpTftpClientDotNet
{
    class Client
    {
        Socket s;
        NetworkStream ns;
        public enum OP_CODE:short{RRQ=1,WRQ=2,CD=6,LIST=7,ACK=4,DATA=3,ERROR=5, CLOSE=8}
        public Client(string address, int port)
        {
            s = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            s.Bind(new IPEndPoint(IPAddress.Any, new Random().Next(1024,65535)));
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
                    txBuffer = new byte[buf.Length + 2 + mode.Length];
                    b_op = BitConverter.GetBytes(IPAddress.HostToNetworkOrder((short)OP_CODE.WRQ));
                    Buffer.BlockCopy(b_op, 0, txBuffer, 0, b_op.Length);
                    Buffer.BlockCopy(buf, 0, txBuffer, b_op.Length, buf.Length);
                    Buffer.BlockCopy(mode, 0, txBuffer, b_op.Length + buf.Length, mode.Length);
                    s.Send(txBuffer);
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
                        s.Send(txBuffer);
                    }
                    break;
                case OP_CODE.CLOSE:

                        txBuffer = new byte[4];
                        b_op = BitConverter.GetBytes(IPAddress.HostToNetworkOrder((short)OP_CODE.ACK));
                        Buffer.BlockCopy(b_op, 0, txBuffer, 0, b_op.Length);
                        //byte[] closeBuf = BitConverter.GetBytes(IPAddress.HostToNetworkOrder(blockNumber));
                        //Buffer.BlockCopy(closeBuf, 0, txBuffer, b_op.Length, sizeof(short));
                        txBuffer[2] = 0x55; txBuffer[3] = 0x55;
                        s.Send(txBuffer);
                    break;
                case OP_CODE.DATA:
                    txBuffer = new byte[4+data.Length];
                    b_op = BitConverter.GetBytes(IPAddress.HostToNetworkOrder((short)OP_CODE.DATA));
                    Buffer.BlockCopy(b_op, 0, txBuffer, 0, b_op.Length);
                    Buffer.BlockCopy(BitConverter.GetBytes(IPAddress.HostToNetworkOrder(blockNumber)), 0, txBuffer, 2, 2);
                    Buffer.BlockCopy(data, 0, txBuffer, 4, data.Length);
                    s.Send(txBuffer);
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

        public int PutFile(string filename)
        {
            byte[] txBuf = new byte[516];
            int txCount;
            string s = System.IO.File.ReadAllText(filename);
            System.IO.StringReader sr = new System.IO.StringReader(s);
            short blockNumber;
            ASCIIEncoding ae = new ASCIIEncoding();
            sendTftpPacket(Client.OP_CODE.WRQ, filename, null, -1);//send WRQ
            char[] chunk = new char[512];
            int dataLength = sr.ReadBlock(chunk,0,512);
            do
            {
                txCount = recvTftpPacket(ref txBuf);
                OP_CODE op = (OP_CODE)IPAddress.NetworkToHostOrder(BitConverter.ToInt16(txBuf, 0));
                System.Diagnostics.Debug.Assert(op==OP_CODE.ACK);
                blockNumber = IPAddress.NetworkToHostOrder(BitConverter.ToInt16(txBuf, 2));
                //s += new string(ae.GetChars(rxBuf, sizeof(short) * 2, rxCount - 4 >= 4 ? rxCount - 4 : 0)); 
                byte[] data = new byte[dataLength];
                int chrCount,byteCount;
                bool completed;
                ae.GetEncoder().Convert(chunk,0,dataLength,data,0,data.Length,true,out chrCount,out byteCount,out completed);

                sendTftpPacket(Client.OP_CODE.DATA, string.Empty, data, ++blockNumber);
            } while ((dataLength=sr.ReadBlock(chunk, 0, 512)) == 512);
            //sendTftpPacket(Client.OP_CODE.ACK, string.Empty, null, blockNumber);//final ACK
            return txCount;
        }

        public int PutBinaryFile(string filename)
        {
            byte[] txBuf = new byte[516];
            byte[] data;
            int txCount;
            FileStream fs = new FileStream(filename, FileMode.Open);
            BinaryReader br = new BinaryReader(fs);
            short blockNumber;
            ASCIIEncoding ae = new ASCIIEncoding();
            sendTftpPacket(Client.OP_CODE.WRQ, Path.GetFileName(filename), null, -1);//send WRQ
            //char[] chunk = new char[512];
            //int dataLength = sr.ReadBlock(chunk, 0, 512);
            do
            {
                txCount = recvTftpPacket(ref txBuf);
                OP_CODE op = (OP_CODE)IPAddress.NetworkToHostOrder(BitConverter.ToInt16(txBuf, 0));
                System.Diagnostics.Debug.Assert(op == OP_CODE.ACK);
                blockNumber = IPAddress.NetworkToHostOrder(BitConverter.ToInt16(txBuf, 2));
                //s += new string(ae.GetChars(rxBuf, sizeof(short) * 2, rxCount - 4 >= 4 ? rxCount - 4 : 0)); 
                data = br.ReadBytes(512);
                //int chrCount, byteCount;
                //bool completed;
                //ae.GetEncoder().Convert(chunk, 0, dataLength, data, 0, data.Length, true, out chrCount, out byteCount, out completed);

                sendTftpPacket(Client.OP_CODE.DATA, string.Empty, data, ++blockNumber);
            } while (data.Length == 512);
            //sendTftpPacket(Client.OP_CODE.ACK, string.Empty, null, blockNumber);//final ACK
            return txCount;
        }

        ~Client()
        {
            s.Close(1);
        }
    }
}
