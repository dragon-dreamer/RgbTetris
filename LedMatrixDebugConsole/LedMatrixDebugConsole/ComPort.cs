// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO.Ports;
using System.Threading;

namespace LedMatrixDebugConsole
{
    public class ComPort : IDisposable
    {
        private SerialPort _port;
        private Thread _notificationThread;
        private AutoResetEvent _threadEvent;
        private volatile bool _running = false;
        private readonly List<IReceivedComPacketType> _packetTypes = new List<IReceivedComPacketType>();
        private ConcurrentQueue<byte> _receivedBytes = new ConcurrentQueue<byte>();

        public ComPort(string name, int baud, Parity parity, int dataBits, StopBits stopBits)
        {
            _port = new SerialPort(name, baud, parity, dataBits, stopBits);
            _port.DataReceived += ComDataReceived;
        }

        public void AddPacketType(IReceivedComPacketType packet)
        {
            if (_notificationThread != null && _notificationThread.IsAlive)
                throw new InvalidOperationException("Unable to add new packet types when com port is open");

            _packetTypes.Add(packet);
        }

        public void Start()
        {
            try
            {
                if (!_running)
                {
                    _running = true;
                    _threadEvent = new AutoResetEvent(false);
                    _notificationThread = new Thread(new ThreadStart(NotificationThread));
                    _notificationThread.Start();
                    _port.Open();
                }
            }
            catch(Exception)
            {
                _running = false;

                _threadEvent.Set();
                if (_notificationThread != null && _notificationThread.IsAlive)
                    _notificationThread.Join();

                _threadEvent.Dispose();
                _threadEvent = null;
                _receivedBytes = new ConcurrentQueue<byte>();

                throw;
            }
        }

        public void Write(IComPacketToSend packet)
        {
            var data = packet.GetRawData();
            _port.Write(data, 0, data.Length);
        }

        private void NotificationThread()
        {
            while (_running)
            {
                bool packetFound = false;
                int unknownPacketTypeCount = 0;

                byte data;
                if (_receivedBytes.TryPeek(out data))
                {
                    foreach (var packet in _packetTypes)
                    {
                        if (_receivedBytes.Count < packet.PacketLength)
                            continue;

                        if (packet.CheckPacketSignature(data))
                        {
                            byte[] packetData = new byte[packet.PacketLength];
                            for(int i = 0; i != packet.PacketLength; ++i)
                            {
                                while (!_receivedBytes.TryDequeue(out packetData[i]))
                                    continue;
                            }

                            packet.ParsePacket(packetData);
                            packetFound = true;
                            break;
                        }
                        else
                        {
                            ++unknownPacketTypeCount;
                        }
                    }

                    if (unknownPacketTypeCount == _packetTypes.Count)
                    {
                        //Remove one byte, because no packet types has recognized the signature
                        _receivedBytes.TryDequeue(out data);
                        _threadEvent.Set();
                    }
                }

                if (!packetFound)
                    _threadEvent.WaitOne();
            }
        }

        private void ComDataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            SerialPort port = (SerialPort)sender;

            int bytesAvailable = port.BytesToRead;
            byte[] recvBuf = new byte[bytesAvailable];

            try
            {
                port.Read(recvBuf, 0, bytesAvailable);
                for (int index = 0; index != bytesAvailable; ++index)
                    _receivedBytes.Enqueue(recvBuf[index]);

                _threadEvent.Set();
            }
            catch (TimeoutException)
            {
            }
        }

#region IDisposable
        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (_port != null)
                {
                    _port.Dispose();
                    _port = null;
                }

                if(_running)
                {
                    _running = false;
                    _threadEvent.Set();
                    if (_notificationThread.IsAlive)
                        _notificationThread.Join();
                }

                if(_threadEvent != null)
                {
                    _threadEvent.Dispose();
                    _threadEvent = null;
                }
            }
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
    }
#endregion
}
